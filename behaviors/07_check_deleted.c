// Удалённая проверка БЕЗ всякого падения: программа тихо возвращает неверный ответ.
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

// Классическая (неверная) проверка переполнения ПОСЛЕ факта.
// Компилятор: "знаковое переполнение — UB, значит x+100 всегда > x, значит if(...) всегда ложь".
__attribute__((noinline))
int add100_checked_after(int x){
    int r = x + 100;
    if (r < x) return -1;        // "переполнение" — на -O2 удаляется целиком
    return r;
}
// Верно: проверять ДО операции, не полагаясь на переполнение.
__attribute__((noinline))
int add100_checked_before(int x){
    if (x > INT_MAX - 100) return -1;
    return x + 100;
}
// Верно и проще: встроенная функция компилятора.
__attribute__((noinline))
int add100_builtin(int x){
    int r;
    if (__builtin_add_overflow(x, 100, &r)) return -1;
    return r;
}

int main(int argc, char **argv){
    int x = (argc > 1) ? atoi(argv[1]) : INT_MAX - 50;   // компилятор не знает значения
    printf("вход x = %d  (INT_MAX = %d, x+100 не помещается)\n\n", x, INT_MAX);
    printf("  проверка ПОСЛЕ  (r < x)              -> %d\n", add100_checked_after(x));
    printf("  проверка ДО     (x > INT_MAX-100)    -> %d\n", add100_checked_before(x));
    printf("  __builtin_add_overflow               -> %d\n", add100_builtin(x));
    printf("\n  ожидался -1 во всех трёх случаях\n");
    return 0;
}
