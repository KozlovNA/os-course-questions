// Одна и та же операция (popcount) пятью способами. Сумма проверяется, затем замер.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <immintrin.h>
#include <stdbit.h>

uint64_t popcnt1(uint64_t);                                   // из popcnt_asm.S
uint64_t popcnt_words(const uint64_t *, size_t);
uint64_t popcnt_words_xor(const uint64_t *, size_t);

#define N   (1u<<15)      // 256 КБ: помещается в L2, замер вычислительный, а не по памяти
#define REP 3000
typedef uint64_t (*fn_t)(const uint64_t *, size_t);

// 0. Переносимый C (без знания об архитектуре)
__attribute__((noinline)) static uint64_t p_kernighan(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++){ uint64_t x=p[i]; while(x){ x&=x-1; s++; } } return s; }
__attribute__((noinline)) static uint64_t p_swar(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++){ uint64_t x=p[i];
    x=x-((x>>1)&0x5555555555555555ull); x=(x&0x3333333333333333ull)+((x>>2)&0x3333333333333333ull);
    x=(x+(x>>4))&0x0F0F0F0F0F0F0F0Full; s+=(x*0x0101010101010101ull)>>56; } return s; }
// 1. Встроенная функция компилятора БЕЗ разрешения popcnt: gcc вызывает __popcountdi2 из libgcc
__attribute__((noinline)) static uint64_t p_builtin_base(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=__builtin_popcountll(p[i]); return s; }
// 2. Она же с target("popcnt"): одна инструкция, встраивается
__attribute__((noinline,target("popcnt"))) static uint64_t p_builtin_hw(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=__builtin_popcountll(p[i]); return s; }
// 3. Стандартные средства: C23 <stdbit.h>
__attribute__((noinline,target("popcnt"))) static uint64_t p_stdbit(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=stdc_count_ones(p[i]); return s; }
// 4. Intrinsic (архитектурно-специфичный, читаемый, компилятор знает семантику)
__attribute__((noinline,target("popcnt"))) static uint64_t p_intrinsic(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=_mm_popcnt_u64(p[i]); return s; }
// 5. Inline asm: компилятор видит «чёрный ящик» с ограничениями
__attribute__((noinline)) static uint64_t p_inline_asm(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++){ uint64_t r; __asm__("popcnt %1, %0" : "=r"(r) : "rm"(p[i]) : "cc"); s+=r; } return s; }
// 6. Отдельный .S: вызов на каждое слово (нельзя встроить) и «оптом» на весь массив
__attribute__((noinline)) static uint64_t p_asm_call1(const uint64_t *p, size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=popcnt1(p[i]); return s; }

static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){
  uint64_t *a=malloc(N*8); uint64_t x=88172645463325252ull;
  for(size_t i=0;i<N;i++){ x^=x<<13; x^=x>>7; x^=x<<17; a[i]=x; }
  struct { const char *name; fn_t f; } v[]={
    {"C: цикл Кернигана (переносимо)", p_kernighan},
    {"C: SWAR-трюк (переносимо)", p_swar},
    {"__builtin_popcountll, без -mpopcnt", p_builtin_base},
    {"__builtin_popcountll + target(popcnt)", p_builtin_hw},
    {"C23 stdc_count_ones + target(popcnt)", p_stdbit},
    {"intrinsic _mm_popcnt_u64", p_intrinsic},
    {"inline asm 'popcnt' на слово", p_inline_asm},
    {".S: вызов popcnt1() на слово", p_asm_call1},
    {".S: popcnt_words() на весь массив (наивно)", popcnt_words},
    {".S: popcnt_words_xor() (xor перед popcnt)", popcnt_words_xor},
  };
  uint64_t ref=p_swar(a,N); double base=0;
  const int TRIALS=5;   // best-of-N: на ноутбуке разброс между прогонами в разы, берём минимум
  printf("%-44s %9s %8s %8s %s\n","способ","лучшее,с","худшее","Гслов/с","сумма");
  for(size_t k=0;k<sizeof v/sizeof*v;k++){
    double best=1e9,worst=0; uint64_t r=0;
    for(int tr=0;tr<TRIALS;tr++){ r=0; double t=now(); for(int i=0;i<REP;i++) r+=v[k].f(a,N); t=now()-t;
      if(t<best) best=t;
      if(t>worst) worst=t; }
    if(k==0) base=best;
    printf("%-44s %9.3f %8.3f %8.2f %s  x%.0f\n",v[k].name,best,worst,(double)N*REP/best/1e9, r==ref*REP?"OK":"НЕВЕРНО",base/best);
  }
  return 0; }
