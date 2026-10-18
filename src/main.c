#include <stdio.h>
#include <string.h>

#include "puzzle.h"

int main(int argc, char **argv)
{
    char puzzle[256];
    char result[256];

    if (argc > 1) {
        /* Собираем все аргументы в одну строку ребуса. */
        puzzle[0] = '\0';
        for (int i = 1; i < argc; i++) {
            strcat(puzzle, argv[i]);
            if (i + 1 < argc)
                strcat(puzzle, " ");
        }
    } else {
        printf("Введите ребус (например, SEND + MORE = MONEY):\n");
        if (!fgets(puzzle, sizeof(puzzle), stdin))
            return 1;
        puzzle[strcspn(puzzle, "\r\n")] = '\0';
    }

    if (solve_puzzle(puzzle, result, sizeof(result))) {
        printf("%s\n", result);
        return 0;
    }

    printf("Решение не найдено.\n");
    return 1;
}
