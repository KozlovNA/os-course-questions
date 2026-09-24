// Переносимость: одна операция (rotate left + popcount + barrier) четырьмя способами,
// и диспетчеризация по возможностям CPU во время выполнения.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// --- 1. Переносимая идиома, которую компилятор сам сворачивает в одну инструкцию ---
static inline uint64_t rotl_portable(uint64_t x, int n){ return (x<<n) | (x>>(64-n)); }  // gcc/clang -> rolq
// --- 2. Intrinsic: компилируется только на x86 ---
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
static inline uint64_t rotl_intrin(uint64_t x, int n){ return _lrotl(x,n); }
#endif
// --- 3. Inline asm: привязан и к архитектуре, и к синтаксису ассемблера компилятора ---
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
static inline uint64_t rotl_asm(uint64_t x,int n){ __asm__("rolq %%cl, %0":"+r"(x):"c"(n):"cc"); return x; }
#endif

// --- Диспетчеризация во время выполнения: одна сборка, разный код на разных CPU ---
typedef uint64_t (*pc_t)(const uint64_t*,size_t);
__attribute__((target("default")))        static uint64_t pc_impl(const uint64_t*p,size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++){uint64_t x=p[i];while(x){x&=x-1;s++;}} return s; }
__attribute__((target("popcnt")))         static uint64_t pc_impl_popcnt(const uint64_t*p,size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=__builtin_popcountll(p[i]); return s; }
__attribute__((target("avx512vpopcntdq,avx512f"))) static uint64_t pc_impl_avx512(const uint64_t*p,size_t n){
  uint64_t s=0; for(size_t i=0;i<n;i++) s+=__builtin_popcountll(p[i]); return s; }

int main(void){
  __builtin_cpu_init();
  printf("== Что доступно на этом CPU (проверка во время выполнения)\n");
  printf("   popcnt=%d  avx2=%d  bmi2=%d  avx512f=%d  avx512vpopcntdq=%d\n",
    __builtin_cpu_supports("popcnt"),__builtin_cpu_supports("avx2"),
    __builtin_cpu_supports("bmi2"),__builtin_cpu_supports("avx512f"),
    __builtin_cpu_supports("avx512vpopcntdq"));
  pc_t chosen = __builtin_cpu_supports("avx512vpopcntdq") ? pc_impl_avx512
              : __builtin_cpu_supports("popcnt")          ? pc_impl_popcnt : pc_impl;
  const char* name = chosen==pc_impl_avx512?"avx512vpopcntdq":chosen==pc_impl_popcnt?"popcnt":"переносимый C";
  printf("   выбрана реализация: %s\n", name);

  uint64_t a[4096], x=12345; for(int i=0;i<4096;i++){x^=x<<13;x^=x>>7;x^=x<<17;a[i]=x;}
  printf("   результат: %lu\n", chosen(a,4096));

  printf("\n== Один и тот же rotl тремя способами (значения должны совпасть)\n");
  uint64_t v=0x0123456789ABCDEFull;
  printf("   портируемый C: 0x%016lX\n", rotl_portable(v,13));
#if defined(__x86_64__)
  printf("   intrinsic    : 0x%016lX\n", rotl_intrin(v,13));
  printf("   inline asm   : 0x%016lX\n", rotl_asm(v,13));
#endif
  return 0; }
