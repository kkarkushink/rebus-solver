#include "puzzle.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define MAX_WORDS 8       /* до 7 слагаемых + 1 результат */
#define MAX_WORD_LEN 16
#define MAX_LETTERS 26

typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV } Operator;

typedef struct {
    char words[MAX_WORDS][MAX_WORD_LEN];
    int lengths[MAX_WORDS];
    int n_addends;      /* количество операндов слева от "=" */
    int result_len;
    Operator op;
} Puzzle;

/* ---------- разбор строки ребуса ---------- */

static int op_char(char c, Operator *op)
{
    switch (c) {
    case '+': *op = OP_ADD; return 1;
    case '-': *op = OP_SUB; return 1;
    case '*': *op = OP_MUL; return 1;
    case '/': *op = OP_DIV; return 1;
    default: return 0;
    }
}

static int parse_puzzle(const char *puzzle, Puzzle *p)
{
    int n_words = 0;
    int len = 0;
    int op_seen = 0;
    int op_count = 0;
    int equals_seen = 0;
    int left_words = 0;
    const char *c;
    Operator op = OP_ADD, this_op;

    memset(p, 0, sizeof(*p));

    for (c = puzzle; ; c++) {
        if (isupper((unsigned char)*c)) {
            if (len >= MAX_WORD_LEN - 1 || n_words >= MAX_WORDS)
                return 0;
            p->words[n_words][len++] = *c;
        } else if (op_char(*c, &this_op) || *c == '=' || *c == '\0') {
            if (len == 0)
                return 0;
            p->words[n_words][len] = '\0';
            p->lengths[n_words] = len;
            n_words++;
            len = 0;

            if (*c != '=' && *c != '\0') {
                if (equals_seen)
                    return 0; /* после знака равенства должен быть только результат */
                /* знак действия между операндами - все знаки должны совпадать */
                if (op_seen && this_op != op)
                    return 0; /* смешение разных действий в одном ребусе не поддерживается */
                op = this_op;
                op_seen = 1;
                op_count++;
            } else if (*c == '=') {
                if (equals_seen)
                    return 0; /* в выражении должен быть ровно один знак равенства */
                equals_seen = 1;
                left_words = n_words;
            }
            if (*c == '\0')
                break;
        } else if (isspace((unsigned char)*c)) {
            continue;
        } else {
            return 0; /* недопустимый символ */
        }
    }

    if (!op_seen || !equals_seen || n_words < 3 || n_words > MAX_WORDS)
        return 0;

    /* Слева должно быть от 2 до 7 операндов, справа - ровно одно слово. */
    if (left_words < 2 || left_words > MAX_WORDS - 1 ||
        n_words != left_words + 1 || op_count != left_words - 1)
        return 0;

    p->op = op;
    p->n_addends = left_words;
    p->result_len = p->lengths[left_words];

    if (op != OP_ADD && p->n_addends != 2)
        return 0; /* вычитание, умножение и деление - только бинарные операции */

    return 1;
}

static const char *op_symbol(Operator op)
{
    switch (op) {
    case OP_SUB: return "-";
    case OP_MUL: return "*";
    case OP_DIV: return "/";
    default:     return "+";
    }
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
        if (i == p->n_addends - 1) {
            buf[pos++] = '=';
        } else {
            const char *sym = op_symbol(p->op);
            buf[pos++] = sym[0];
        }
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

/* Поразрядный перебор: после назначения цифр в очередном столбце сразу
 * проверяем сумму и перенос. Столбцы строятся один раз, а занятые цифры
 * хранятся в битовой маске. */

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

/* Для вычитания, умножения и деления используем обычный перебор по буквам.
 * Эти операции поддерживаются только для двух операндов. */

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

static long long word_value(const char *w, int len, const int digit_of[MAX_LETTERS])
{
    long long v = 0;
    int i;
    for (i = 0; i < len; i++)
        v = v * 10 + digit_of[w[i] - 'A'];
    return v;
}

static int check_binary_op(const Puzzle *p, const int digit_of[MAX_LETTERS])
{
    long long a = word_value(p->words[0], p->lengths[0], digit_of);
    long long b = word_value(p->words[1], p->lengths[1], digit_of);
    long long r = word_value(p->words[2], p->result_len, digit_of);

    switch (p->op) {
    case OP_SUB: return a - b == r;
    case OP_MUL: return a * b == r;
    case OP_DIV: return b != 0 && a == b * r;
    default:     return 0; /* сложение сюда не попадает - см. solve_puzzle */
    }
}

static int assign_letter_generic(int idx, const char letters[MAX_LETTERS], int n_letters,
                                  int digit_of[MAX_LETTERS], unsigned used,
                                  const int is_leading[MAX_LETTERS], const Puzzle *p)
{
    int d;
    char letter;
    unsigned bit;

    if (idx == n_letters)
        return check_binary_op(p, digit_of);

    letter = letters[idx];
    for (d = 0; d <= 9; d++) {
        bit = 1u << d;
        if (used & bit)
            continue;
        if (d == 0 && is_leading[letter - 'A'])
            continue;

        digit_of[letter - 'A'] = d;

        if (assign_letter_generic(idx + 1, letters, n_letters, digit_of, used | bit, is_leading, p))
            return 1;

        digit_of[letter - 'A'] = -1;
    }
    return 0;
}

static int solve_binary_op(const Puzzle *p, int digit_of[MAX_LETTERS], const int is_leading[MAX_LETTERS])
{
    char letters[MAX_LETTERS];
    int n_letters = collect_letters(p, letters);

    if (n_letters > 10)
        return 0;

    return assign_letter_generic(0, letters, n_letters, digit_of, 0u, is_leading, p);
}

int solve_puzzle(const char *puzzle, char *result, size_t result_size)
{
    Puzzle p;
    ColumnLayout columns[MAX_WORD_LEN];
    int digit_of[MAX_LETTERS];
    int is_leading[MAX_LETTERS];
    int i, max_addend_len = 0;

    if (!puzzle || !result || result_size == 0)
        return 0;

    if (!parse_puzzle(puzzle, &p))
        return 0;

    mark_leading(&p, is_leading);

    for (i = 0; i < MAX_LETTERS; i++)
        digit_of[i] = -1;

    if (p.op == OP_ADD) {
        for (i = 0; i < p.n_addends; i++)
            if (p.lengths[i] > max_addend_len)
                max_addend_len = p.lengths[i];
        if (p.result_len < max_addend_len)
            return 0; /* результат заведомо короче слагаемых - решения не существует */
        build_columns(&p, columns);
        if (!solve_column(0, 0, digit_of, 0u, is_leading, &p, columns))
            return 0;
    } else {
        if (!solve_binary_op(&p, digit_of, is_leading))
            return 0;
    }

    format_result(&p, digit_of, result, result_size);
    return 1;
}
