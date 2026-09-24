// Ловушки inline asm и преимущества intrinsic: проверяется на -O0 и -O2
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// 1) Забыли сообщить компилятору, что asm пишет в память (нет "memory"/операнда "=m")
__attribute__((noinline)) int bad_store(void){
  int x=0; __asm__ volatile("movl $42, (%0)" :: "r"(&x));        // ОШИБКА: компилятор не знает про запись
  return x; }
__attribute__((noinline)) int good_store(void){
  int x=0; __asm__ volatile("movl $42, %0" : "=m"(x));            // ВЕРНО: операнд вывода
  return x; }
// 2) Нет early-clobber "&": приёмник может совпасть с входным регистром
__attribute__((noinline)) long bad_early(long a,long b){
  long out; __asm__("mov %1,%0\n\tadd %2,%0" : "=r"(out) : "r"(a),"r"(b)); return out; }   // ждём a+b
__attribute__((noinline)) long good_early(long a,long b){
  long out; __asm__("mov %1,%0\n\tadd %2,%0" : "=&r"(out) : "r"(a),"r"(b)); return out; }
// 3) Свертка констант: intrinsic/builtin понятен компилятору, asm — нет
__attribute__((noinline)) unsigned long const_builtin(void){ return __builtin_popcountll(0xFF00FFull); }
__attribute__((noinline)) unsigned long const_asm(void){
  unsigned long r; __asm__("popcnt %1,%0" : "=r"(r) : "r"(0xFF00FFul) : "cc"); return r; }
int main(void){
  printf("bad_store()  = %d  (ждём 42)\n", bad_store());
  printf("good_store() = %d\n", good_store());
  printf("bad_early(3,4)  = %ld (ждём 7)\n", bad_early(3,4));
  printf("good_early(3,4) = %ld\n", good_early(3,4));
  printf("const_builtin() = %lu, const_asm() = %lu\n", const_builtin(), const_asm());
  return 0; }
