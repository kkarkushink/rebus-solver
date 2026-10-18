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
 *
 * Ниже - две машинно-независимые (не зависящие от конкретного процессора)
 * микрооптимизации этого же алгоритма, не меняющие его асимптотику:
 *  1) раскладка букв по разрядам (ColumnLayout) строится один раз перед
 *     поиском, а не пересчитывается заново при каждом вызове solve_column -
 *     во время перебора эта функция вызывается многократно для одного и
 *     того же разряда (при откате на предыдущих разрядах), и повторное
 *     сканирование слов было бы лишней работой;
 *  2) множество использованных цифр хранится как битовая маска (1 слово
 *     вместо массива из 10 элементов), проверка и установка - это одна
 *     битовая операция вместо обращения к массиву по индексу.
 */

typedef struct {
    char addend_letter[MAX_WORDS]; /* буква слагаемого в этом разряде, 0 - разряда нет */
    char result_letter;
} ColumnLayout;

static void build_columns(const Puzzle *p, ColumnLayout columns[MAX_WORD_LEN])
{
    int col, i, len;

    for (col = 0; col < p->result_len; col++) {
        for (i = 0; i < p->n_addends; i++) {
            len = p->lengths[i];
            columns[col].addend_letter[i] = (char)((col < len) ? p->words[i][len - 1 - col] : 0);
        }
        columns[col].result_letter = p->words[p->n_addends][p->result_len - 1 - col];
    }
}

static int solve_column(int col, int carry, int digit_of[MAX_LETTERS], unsigned used,
                         const int is_leading[MAX_LETTERS], const Puzzle *p,
                         const ColumnLayout columns[MAX_WORD_LEN]);

/* Все буквы разряда col уже получили цифры - проверяем равенство разряда
 * с учётом переноса и переходим к следующему разряду. */
static int check_column(int col, int carry, int digit_of[MAX_LETTERS], unsigned used,
                         const int is_leading[MAX_LETTERS], const Puzzle *p,
                         const ColumnLayout columns[MAX_WORD_LEN])
{
    const ColumnLayout *cl = &columns[col];
    int sum = carry;
    int i;

    for (i = 0; i < p->n_addends; i++)
        if (cl->addend_letter[i])
            sum += digit_of[cl->addend_letter[i] - 'A'];

    if (sum % 10 != digit_of[cl->result_letter - 'A'])
        return 0;

    return solve_column(col + 1, sum / 10, digit_of, used, is_leading, p, columns);
}

/* Поочерёдно назначает цифры новым буквам разряда col (new_letters[idx..n_new-1]),
 * а когда все они назначены - проверяет равенство разряда. */
static int assign_column_letters(const char new_letters[MAX_LETTERS], int n_new, int idx,
                                  int col, int carry, int digit_of[MAX_LETTERS], unsigned used,
                                  const int is_leading[MAX_LETTERS], const Puzzle *p,
                                  const ColumnLayout columns[MAX_WORD_LEN])
{
    int d;
    char letter;
    unsigned bit;

    if (idx == n_new)
        return check_column(col, carry, digit_of, used, is_leading, p, columns);

    letter = new_letters[idx];
    for (d = 0; d <= 9; d++) {
        bit = 1u << d;
        if (used & bit)
            continue;
        if (d == 0 && is_leading[letter - 'A'])
            continue;

        digit_of[letter - 'A'] = d;

        if (assign_column_letters(new_letters, n_new, idx + 1, col, carry, digit_of,
                                   used | bit, is_leading, p, columns))
            return 1;

        digit_of[letter - 'A'] = -1;
    }
    return 0;
}

/* Решение, начиная с разряда col (перенос carry из предыдущего разряда уже известен). */
static int solve_column(int col, int carry, int digit_of[MAX_LETTERS], unsigned used,
                         const int is_leading[MAX_LETTERS], const Puzzle *p,
                         const ColumnLayout columns[MAX_WORD_LEN])
{
    const ColumnLayout *cl;
    char new_letters[MAX_LETTERS];
    int n_new = 0;
    int i, k;
    char letter;

    if (col == p->result_len)
        return carry == 0; /* все разряды обработаны - лишнего переноса не должно остаться */

    cl = &columns[col];
    for (i = 0; i < p->n_addends; i++) {
        letter = cl->addend_letter[i];
        if (!letter || digit_of[letter - 'A'] != -1)
            continue;
        for (k = 0; k < n_new && new_letters[k] != letter; k++)
            ;
        if (k == n_new)
            new_letters[n_new++] = letter;
    }

    letter = cl->result_letter;
    if (digit_of[letter - 'A'] == -1) {
        for (k = 0; k < n_new && new_letters[k] != letter; k++)
            ;
        if (k == n_new)
            new_letters[n_new++] = letter;
    }

    return assign_column_letters(new_letters, n_new, 0, col, carry, digit_of, used, is_leading, p, columns);
}

int solve_puzzle(const char *puzzle, char *result, size_t result_size)
{
    Puzzle p;
    ColumnLayout columns[MAX_WORD_LEN];
    int digit_of[MAX_LETTERS];
    int is_leading[MAX_LETTERS];
    int i;

    if (!parse_puzzle(puzzle, &p))
        return 0;

    if (p.result_len < p.lengths[0])
        return 0; /* результат заведомо короче слагаемых - решения не существует */

    mark_leading(&p, is_leading);
    build_columns(&p, columns);

    for (i = 0; i < MAX_LETTERS; i++)
        digit_of[i] = -1;

    if (!solve_column(0, 0, digit_of, 0u, is_leading, &p, columns))
        return 0;

    format_result(&p, digit_of, result, result_size);
    return 1;
}
