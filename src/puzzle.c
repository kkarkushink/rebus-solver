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

/* ---------- буквы, которые не могут получить цифру 0 (первые буквы слов) ---------- */

static void mark_leading(const Puzzle *p, int is_leading[MAX_LETTERS])
{
    int i;
    memset(is_leading, 0, MAX_LETTERS * sizeof(int));
    for (i = 0; i <= p->n_addends; i++)
        if (p->lengths[i] > 1)
            is_leading[p->words[i][0] - 'A'] = 1;
}

/* ---------- оптимизированный алгоритм: разбор по столбцам (поразрядное сложение) ----------
 *
 * Вместо того чтобы назначать цифры всем буквам сразу и проверять
 * арифметическое равенство только в самом конце (как в предыдущих версиях),
 * ребус обрабатывается по разрядам - от младшего к старшему, как при сложении
 * столбиком. Буквы, встречающиеся в текущем разряде, получают цифры прямо
 * перед проверкой равенства этого разряда с переносом, поэтому неверная
 * ветка отбрасывается сразу на своём разряде, а не после полного назначения
 * всех букв ребуса. Это и есть главное алгоритмическое ускорение относительно
 * версий 1 и 2.
 */

/* Буква слова word_idx в разряде col (0 - младший разряд), если такой разряд
 * у слова есть; иначе функция возвращает 0, и слово в этом разряде не участвует. */
static int column_letter(const Puzzle *p, int word_idx, int col, char *out_letter)
{
    int len = p->lengths[word_idx];
    if (col >= len)
        return 0;
    *out_letter = p->words[word_idx][len - 1 - col];
    return 1;
}

static int solve_column(int col, int carry, int digit_of[MAX_LETTERS], int used[10],
                         const int is_leading[MAX_LETTERS], const Puzzle *p);

/* Все буквы разряда col уже получили цифры - проверяем равенство разряда
 * с учётом переноса и переходим к следующему разряду. */
static int check_column(int col, int carry, int digit_of[MAX_LETTERS], int used[10],
                         const int is_leading[MAX_LETTERS], const Puzzle *p)
{
    int sum = carry;
    int i;
    char letter;

    for (i = 0; i < p->n_addends; i++)
        if (column_letter(p, i, col, &letter))
            sum += digit_of[letter - 'A'];

    column_letter(p, p->n_addends, col, &letter); /* результат точно есть: col < result_len */
    if (sum % 10 != digit_of[letter - 'A'])
        return 0;

    return solve_column(col + 1, sum / 10, digit_of, used, is_leading, p);
}

/* Поочерёдно назначает цифры новым буквам разряда col (new_letters[idx..n_new-1]),
 * а когда все они назначены - проверяет равенство разряда. */
static int assign_column_letters(const char new_letters[MAX_LETTERS], int n_new, int idx,
                                  int col, int carry, int digit_of[MAX_LETTERS], int used[10],
                                  const int is_leading[MAX_LETTERS], const Puzzle *p)
{
    int d;
    char letter;

    if (idx == n_new)
        return check_column(col, carry, digit_of, used, is_leading, p);

    letter = new_letters[idx];
    for (d = 0; d <= 9; d++) {
        if (used[d])
            continue;
        if (d == 0 && is_leading[letter - 'A'])
            continue;

        used[d] = 1;
        digit_of[letter - 'A'] = d;

        if (assign_column_letters(new_letters, n_new, idx + 1, col, carry, digit_of, used, is_leading, p))
            return 1;

        used[d] = 0;
        digit_of[letter - 'A'] = -1;
    }
    return 0;
}

/* Решение, начиная с разряда col (перенос carry из предыдущего разряда уже известен). */
static int solve_column(int col, int carry, int digit_of[MAX_LETTERS], int used[10],
                         const int is_leading[MAX_LETTERS], const Puzzle *p)
{
    char new_letters[MAX_LETTERS];
    int n_new = 0;
    int i, k;
    char letter;

    if (col == p->result_len)
        return carry == 0; /* все разряды обработаны - лишнего переноса не должно остаться */

    for (i = 0; i < p->n_addends; i++) {
        if (!column_letter(p, i, col, &letter) || digit_of[letter - 'A'] != -1)
            continue;
        for (k = 0; k < n_new && new_letters[k] != letter; k++)
            ;
        if (k == n_new)
            new_letters[n_new++] = letter;
    }

    column_letter(p, p->n_addends, col, &letter);
    if (digit_of[letter - 'A'] == -1) {
        for (k = 0; k < n_new && new_letters[k] != letter; k++)
            ;
        if (k == n_new)
            new_letters[n_new++] = letter;
    }

    return assign_column_letters(new_letters, n_new, 0, col, carry, digit_of, used, is_leading, p);
}

int solve_puzzle(const char *puzzle, char *result, size_t result_size)
{
    Puzzle p;
    int digit_of[MAX_LETTERS];
    int is_leading[MAX_LETTERS];
    int used[10] = { 0 };
    int i;

    if (!parse_puzzle(puzzle, &p))
        return 0;

    if (p.result_len < p.lengths[0])
        return 0; /* результат заведомо короче слагаемых - решения не существует */

    mark_leading(&p, is_leading);

    for (i = 0; i < MAX_LETTERS; i++)
        digit_of[i] = -1;

    if (!solve_column(0, 0, digit_of, used, is_leading, &p))
        return 0;

    format_result(&p, digit_of, result, result_size);
    return 1;
}
