#include "puzzle.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define MAX_WORDS 8       /* до 7 слагаемых + 1 результат */
#define MAX_WORD_LEN 16
#define MAX_LETTERS 26

typedef struct {
    char words[MAX_WORDS][MAX_WORD_LEN];
    int lengths[MAX_WORDS];
    int n_addends;      /* количество слагаемых */
    int result_len;
} Puzzle;

/* ---------- разбор строки ребуса ---------- */

static int parse_puzzle(const char *puzzle, Puzzle *p)
{
    int n_words = 0;
    int len = 0;
    const char *c;

    memset(p, 0, sizeof(*p));

    for (c = puzzle; ; c++) {
        if (isupper((unsigned char)*c)) {
            if (len >= MAX_WORD_LEN - 1 || n_words >= MAX_WORDS)
                return 0;
            p->words[n_words][len++] = *c;
        } else if (*c == '+' || *c == '=' || *c == '\0') {
            if (len == 0)
                return 0;
            p->words[n_words][len] = '\0';
            p->lengths[n_words] = len;
            n_words++;
            len = 0;
            if (*c == '\0')
                break;
        } else if (isspace((unsigned char)*c)) {
            continue;
        } else {
            return 0; /* недопустимый символ */
        }
    }

    /* последнее слово - результат, остальные - слагаемые */
    if (n_words < 3 || n_words > MAX_WORDS)
        return 0;

    p->n_addends = n_words - 1;
    p->result_len = p->lengths[n_words - 1];
    return 1;
}

static void format_result(const Puzzle *p, const int digit_of[MAX_LETTERS],
                           char *result, size_t result_size)
{
    char buf[256];
    size_t pos = 0;
    int i, j;

    for (i = 0; i < p->n_addends; i++) {
        for (j = 0; j < p->lengths[i]; j++)
            buf[pos++] = (char)('0' + digit_of[p->words[i][j] - 'A']);
        buf[pos++] = ' ';
        buf[pos++] = (i == p->n_addends - 1) ? '=' : '+';
        buf[pos++] = ' ';
    }
    for (j = 0; j < p->result_len; j++)
        buf[pos++] = (char)('0' + digit_of[p->words[p->n_addends][j] - 'A']);
    buf[pos] = '\0';

    strncpy(result, buf, result_size - 1);
    result[result_size - 1] = '\0';
}

/* ---------- сбор различных букв ребуса (в порядке первого появления) ---------- */

static int collect_letters(const Puzzle *p, char letters[MAX_LETTERS])
{
    int seen[MAX_LETTERS];
    int n = 0;
    int i, j;

    memset(seen, 0, sizeof(seen));

    for (i = 0; i <= p->n_addends; i++) {
        for (j = 0; j < p->lengths[i]; j++) {
            int idx = p->words[i][j] - 'A';
            if (!seen[idx]) {
                seen[idx] = 1;
                letters[n++] = (char)('A' + idx);
            }
        }
    }
    return n;
}

/* ---------- базовый алгоритм: полный перебор перестановок цифр ---------- */

static long long word_value(const char *w, int len, const int digit_of[MAX_LETTERS])
{
    long long v = 0;
    int i;
    for (i = 0; i < len; i++)
        v = v * 10 + digit_of[w[i] - 'A'];
    return v;
}

/* Проверка готового (полного) назначения цифр буквам:
 * арифметическое равенство и отсутствие ведущих нулей. */
static int check_assignment(const Puzzle *p, const int digit_of[MAX_LETTERS])
{
    long long sum = 0;
    int i;

    for (i = 0; i <= p->n_addends; i++) {
        if (digit_of[p->words[i][0] - 'A'] == 0 && p->lengths[i] > 1)
            return 0;
    }

    for (i = 0; i < p->n_addends; i++)
        sum += word_value(p->words[i], p->lengths[i], digit_of);

    return sum == word_value(p->words[p->n_addends], p->result_len, digit_of);
}

static int assign_letter(int idx, const char letters[MAX_LETTERS], int n_letters,
                          int digit_of[MAX_LETTERS], int used[10], const Puzzle *p)
{
    int d;

    if (idx == n_letters)
        return check_assignment(p, digit_of);

    for (d = 0; d <= 9; d++) {
        if (used[d])
            continue;
        used[d] = 1;
        digit_of[letters[idx] - 'A'] = d;

        if (assign_letter(idx + 1, letters, n_letters, digit_of, used, p))
            return 1;

        used[d] = 0;
        digit_of[letters[idx] - 'A'] = -1;
    }
    return 0;
}

int solve_puzzle(const char *puzzle, char *result, size_t result_size)
{
    Puzzle p;
    char letters[MAX_LETTERS];
    int n_letters;
    int digit_of[MAX_LETTERS];
    int used[10] = { 0 };
    int i;

    if (!parse_puzzle(puzzle, &p))
        return 0;

    n_letters = collect_letters(&p, letters);
    if (n_letters > 10)
        return 0; /* больше 10 различных букв - решения не существует */

    for (i = 0; i < MAX_LETTERS; i++)
        digit_of[i] = -1;

    if (!assign_letter(0, letters, n_letters, digit_of, used, &p))
        return 0;

    format_result(&p, digit_of, result, result_size);
    return 1;
}
