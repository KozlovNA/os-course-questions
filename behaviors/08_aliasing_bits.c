// Откуда берётся 1073741824: это биты float 2.0f, прочитанные как int.
#include <stdio.h>
#include <stdint.h>
#include <string.h>

__attribute__((noinline)) int alias(int *i, float *f){
    *i = 1;         // в 4 байта кладём целое 1
    *f = 2.0f;      // в ТЕ ЖЕ 4 байта кладём float 2.0
    return *i;      // читаем как int: что вернётся?
}

static void bits(const char *label, uint32_t v){
    printf("  %-11s = %10u = 0x%08X = ", label, v, v);
    for (int b = 31; b >= 0; b--){
        putchar((v >> b & 1) ? '1' : '0');
        if (b == 31 || b == 23) putchar(' ');   // границы знак | порядок | мантисса
    }
    putchar('\n');
}

int main(void){
    uint32_t two;  float f = 2.0f;  memcpy(&two, &f, 4);   // корректный способ посмотреть биты

    printf("Разбор числа 2.0f по IEEE-754 (single precision):\n");
    printf("  2.0 = 1.0 x 2^1  ->  знак=0, порядок=1+127=128, мантисса=0\n\n");
    bits("int   1", 1);
    bits("float 2.0f", two);
    printf("  %-13s   %10u\n", "^^^ оно же:", 1073741824u);
    printf("\n  0x40000000 = 2^30 = %u  <- вот и всё «огромное» число\n\n", 1u<<30);

    int i = 0;
    int r = alias(&i, (float*)&i);      // оба указателя на одну память
    printf("alias(&i, (float*)&i) вернула: %d\n", r);
    printf("  на -O0: %u — прочитаны реально лежащие в памяти биты 2.0f\n", two);
    printf("  на -O2: 1 — компилятор решил, что *f не мог задеть *i, и вернул записанное\n");

    // Самое наглядное: что при этом ЛЕЖИТ в самой переменной?
    printf("\nПосле вызова: функция вернула %d, а в памяти лежит %u\n", r, (unsigned)i);
    printf("  на -O2 это РАЗНЫЕ числа, хотя return читал именно эту переменную\n");
    return 0;
}
