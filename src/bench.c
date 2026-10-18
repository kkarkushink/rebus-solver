#include <stdio.h>
#include <time.h>

#include "puzzle.h"

typedef struct {
    const char *puzzle;
    int repeats;
} Benchmark;

/* Набор ребусов для сравнения быстродействия до/после оптимизаций.
 * BIG+CAT=LION имеет несколько решений, ELEVEN+...=THIRTY использует
 * все 10 различных букв - это самый тяжёлый случай для перебора. */
static const Benchmark benchmarks[] = {
    { "SEND + MORE = MONEY",                1000 },
    { "BE + BE = MOO",                      1000 },
    { "BIG + CAT = LION",                   1000 },
    { "ELEVEN + NINE + FIVE + FIVE = THIRTY", 50 },
};

#define N_BENCHMARKS (sizeof(benchmarks) / sizeof(benchmarks[0]))

int main(void)
{
    size_t i;
    char result[256];

    printf("%-42s %8s %12s %14s\n", "Ребус", "Повтор", "Время, мс", "Мкс/запуск");

    for (i = 0; i < N_BENCHMARKS; i++) {
        const Benchmark *b = &benchmarks[i];
        int ok;
        int r;
        clock_t start, end;
        double ms;

        start = clock();
        for (r = 0; r < b->repeats; r++)
            ok = solve_puzzle(b->puzzle, result, sizeof(result));
        end = clock();

        ms = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;

        printf("%-42s %8d %12.2f %14.2f  %s%s\n",
               b->puzzle, b->repeats, ms, 1000.0 * ms / b->repeats,
               ok ? "-> " : "РЕШЕНИЕ НЕ НАЙДЕНО ", ok ? result : "");
    }

    return 0;
}
