// Чем UB отличается от implementation-defined.
// Тест 1: у implementation-defined ОДИН ответ, задокументированный и стабильный.
// Тест 2: у UB ответ может быть РАЗНЫМ для одного и того же выражения в ОДНОМ бинарнике.
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

// --- implementation-defined: сдвиг отрицательного вправо ---
__attribute__((noinline)) int shr_noinline(int x){ return x >> 1; }
static inline      int shr_inline  (int x){ return x >> 1; }

// --- UB: знаковое переполнение ---
__attribute__((noinline)) int ovf_noinline(int x){ return x + 1 > x; }
static inline      int ovf_inline  (int x){ return x + 1 > x; }

int main(int argc, char **argv){
    int m = (argc > 1) ? atoi(argv[1]) : INT_MAX;   // значение неизвестно компилятору
    int neg = (argc > 1) ? atoi(argv[1]) : -8;

    puts("=== implementation-defined: (-8) >> 1 ===");
    printf("  константа в исходнике : %d\n", -8 >> 1);
    printf("  через noinline-функцию: %d\n", shr_noinline(-8));
    printf("  встроенная версия     : %d\n", shr_inline(neg == -8 ? -8 : neg));
    puts("  все три совпали, и gcc это ДОКУМЕНТИРУЕТ: арифметический сдвиг");

    puts("\n=== UB: (INT_MAX + 1) > INT_MAX ===");
    printf("  константа в исходнике : %d\n", INT_MAX + 1 > INT_MAX);
    printf("  через noinline-функцию: %d\n", ovf_noinline(m));
    printf("  встроенная версия     : %d\n", ovf_inline(m));
    puts("  одно и то же выражение, один бинарник, РАЗНЫЕ ответы");
    return 0;
}
