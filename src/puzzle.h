#ifndef PUZZLE_H
#define PUZZLE_H

#include <stddef.h>

/*
 * Решает числовой ребус вида "SEND + MORE = MONEY".
 *
 * puzzle       - входная строка ребуса (слова из заглавных латинских букв,
 *                разделённые '+' и завершающиеся "= СЛОВО");
 * result       - буфер, в который записывается строка решения,
 *                например "9567 + 1085 = 10652";
 * result_size  - размер буфера result.
 *
 * Возвращает 1, если решение найдено и записано в result, иначе 0.
 */
int solve_puzzle(const char *puzzle, char *result, size_t result_size);

#endif /* PUZZLE_H */
