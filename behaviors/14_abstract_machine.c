// Как стандарт вообще ОПРЕДЕЛЯЕТ поведение: через абстрактную машину,
// наблюдаемое поведение и правило "as-if" (C17 §5.1.2.3).
#include <stdio.h>
#include <time.h>

// Ничего наблюдаемого: результат не нужен -> компилятор вправе убрать всё целиком.
__attribute__((noinline)) void not_observable(void){
    int sum = 0;
    for (int i = 0; i < 1000000; i++) sum += i * i;    // миллион операций
    (void)sum;                                          // и никто их не увидит
}
// volatile-доступ ОБЪЯВЛЕН наблюдаемым -> убирать нельзя, как бы бесполезен он ни был.
__attribute__((noinline)) void observable(void){
    volatile int sink = 0;
    for (int i = 0; i < 1000000; i++) sink = i * i;
}
// Возвращаемое значение используется -> результат обязан совпасть с абстрактной машиной,
// но КАК он получен, стандарт не регламентирует. gcc 14 здесь цикл ОСТАВИЛ
// (замену на n(n+1)/2 правило as-if разрешает, но компилятор ею не воспользовался).
__attribute__((noinline)) long same_result(int n){
    long s = 0; for (int i = 0; i <= n; i++) s += i; return s;
}

static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){
    double t;
    t=now(); for(int k=0;k<200;k++) not_observable(); double t1=now()-t;
    t=now(); for(int k=0;k<200;k++) observable();     double t2=now()-t;
    printf("200 x миллион итераций:\n");
    printf("  без наблюдаемых эффектов : %.4f с\n", t1);
    printf("  с volatile (наблюдаемо)  : %.4f с\n", t2);
    printf("  первое быстрее в %.0f раз — цикла просто нет в коде\n\n", t2/(t1>1e-9?t1:1e-9));
    printf("same_result(1000000) = %ld  (результат обязан совпасть; способ — нет,\n", same_result(1000000));
    printf("                                   но gcc 14 цикл всё же оставил)\n");
    return 0;
}
