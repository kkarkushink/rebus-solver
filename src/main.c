#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "puzzle.h"

int main(int argc, char **argv)
{
    char puzzle[256];
    char result[256];
    int exit_code;

#ifdef _WIN32
    /* Исходники и строки программы хранятся в UTF-8. */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

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
        exit_code = 0;
    } else {
        printf("Решение не найдено.\n");
        exit_code = 1;
    }

    /* При запуске двойным щелчком даём прочитать результат. Если ребус
     * передан аргументом командной строки, лишняя пауза не нужна. */
    if (argc == 1) {
        printf("\nНажмите Enter для выхода...");
        fflush(stdout);
        getchar();
    }

    return exit_code;
}
