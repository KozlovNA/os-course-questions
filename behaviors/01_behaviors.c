#include <stdio.h>
#include <limits.h>
#include <string.h>
// 1) UB: знаковое переполнение — компилятор "знает", что x+1 > x всегда
__attribute__((noinline)) int always_gt(int x){ return x + 1 > x; }
// 2) UB: разыменование NULL -> компилятор удаляет проверку после разыменования.
// ВНИМАНИЕ: при вызове с &a результат 5 при любых -O, вывод программы тут ничего не доказывает.
// Проверка исчезает только в машинном коде; см. 06_nullcheck_real.c и 07_check_deleted.c.
__attribute__((noinline)) int nullchk(int *p){ int v = *p; if(!p) return -1; return v; }
// 3) UB: strict aliasing
__attribute__((noinline)) int alias(volatile int *i, volatile float *f){ *i = 1; *f = 2.0f; return *i; }
// 4) unspecified/impl-defined
int main(void){
  printf("1) INT_MAX+1 > INT_MAX : %d\n", always_gt(INT_MAX));
  printf("%d", INT_MAX+1);
  int a=5; int r = nullchk(&a); printf("2) nullchk(&a)=%d\n", r);
  int i=0; float f=0; int *pi=&i; float *pf=(float*)&i; (void)f;
  // strict aliasing: -O0 даёт 1073741824 (записи мешают друг другу), -O2 даёт 1
  // (компилятор считает, что int* и float* не могут указывать на одну память)
  printf("3) alias: %d   [-O0 -> 1073741824, -O2 -> 1]\n", alias(pi,pf));
  // implementation-defined
  printf("4) sizeof(long)=%zu  sizeof(int)=%zu  char signed? %d  -8>>1=%d  (int)3000000000u=%d\n",
     sizeof(long), sizeof(int), (char)-1 < 0, -8>>1, (int)3000000000u);
  unsigned x=1; printf("   endianness: %s\n", *(unsigned char*)&x ? "little":"big");
  // unspecified: порядок вычисления аргументов
  int k=0; printf("5) unspecified order: %d %d\n", k++, k++);
  return 0; }
