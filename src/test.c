#include <stdio.h>
#include <string.h>

#include "puzzle.h"

static int tests_run = 0;
static int tests_failed = 0;

static void expect_solution(const char *puzzle, const char *expected)
{
    char result[256];

    tests_run++;
    if (!solve_puzzle(puzzle, result, sizeof(result))) {
        printf("FAIL: решение не найдено: %s\n", puzzle);
        tests_failed++;
        return;
    }
    if (strcmp(result, expected) != 0) {
        printf("FAIL: %s\n  ожидалось: %s\n  получено:  %s\n",
               puzzle, expected, result);
        tests_failed++;
    }
}

static void expect_rejected(const char *puzzle)
{
    char result[256];

    tests_run++;
    if (solve_puzzle(puzzle, result, sizeof(result))) {
        printf("FAIL: некорректный ребус принят: %s -> %s\n", puzzle, result);
        tests_failed++;
    }
}

int main(void)
{
    char result[256];

    expect_solution("SEND + MORE = MONEY", "9567 + 1085 = 10652");
    expect_solution("BE + BE = MOO", "72 + 72 = 144");
    expect_solution("BIG + CAT = LION", "762 + 843 = 1605");
    expect_solution("AB - CD = BC", "41 - 29 = 12");
    expect_solution("AB * C = CD", "12 * 3 = 36");
    expect_solution("AB / B = CD", "32 / 2 = 16");
    expect_solution("ABC - D - E = FG", "102 - 3 - 4 = 95");
    expect_solution("A * B * C = DE", "1 * 3 * 8 = 24");
    expect_solution("ABC / D / E = F", "108 / 2 / 6 = 9");

    expect_rejected("A + BB = C");
    expect_rejected("A + B + C");
    expect_rejected("A + B = C = D");
    expect_rejected("A + B = C + D");
    expect_rejected("A + B - C = D");
    expect_rejected("A = B");
    expect_rejected("a + B = C");

    tests_run++;
    if (solve_puzzle(NULL, result, sizeof(result))) {
        printf("FAIL: NULL вместо строки принят\n");
        tests_failed++;
    }

    tests_run++;
    if (solve_puzzle("A + B = C", NULL, sizeof(result))) {
        printf("FAIL: NULL вместо буфера результата принят\n");
        tests_failed++;
    }

    tests_run++;
    if (solve_puzzle("A + B = C", result, 0)) {
        printf("FAIL: нулевой размер буфера принят\n");
        tests_failed++;
    }

    if (tests_failed) {
        printf("Провалено тестов: %d из %d\n", tests_failed, tests_run);
        return 1;
    }

    printf("Все тесты пройдены: %d\n", tests_run);
    return 0;
}
