// Как СМОТРЕТЬ НА ТЕ ЖЕ БАЙТЫ как на другой тип и получать это на -O2.
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// --- 1. memcpy: канонический способ. UB нет, компилятор его распознаёт. ---
__attribute__((noinline)) uint32_t via_memcpy(float f){
    uint32_t u; memcpy(&u, &f, sizeof u); return u;
}
// --- 2. union: разрешено в C (чтение не через записанный член). В C++ формально UB. ---
__attribute__((noinline)) uint32_t via_union(float f){
    union { float f; uint32_t u; } p; p.f = f; return p.u;
}
// --- 3. char*: символьные типы вправе алиасить что угодно ---
__attribute__((noinline)) uint32_t via_char(float f){
    uint32_t u; unsigned char *d=(unsigned char*)&u, *s=(unsigned char*)&f;
    for (size_t k=0;k<sizeof u;k++) d[k]=s[k];
    return u;
}
// --- 4. may_alias: расширение gcc/clang, непереносимо ---
typedef uint32_t __attribute__((may_alias)) u32_ma;
__attribute__((noinline)) uint32_t via_may_alias(float f){ return *(u32_ma*)&f; }
// --- 5. C23: memcpy остаётся основным; в C++20 есть std::bit_cast ---

// --- А вот так НЕЛЬЗЯ: то самое нарушение strict aliasing ---
__attribute__((noinline)) uint32_t via_cast_UB(float f){ return *(uint32_t*)&f; }

// Тот же исходный сценарий: две записи в одну память, чтение обратно
__attribute__((noinline)) int rmw_memcpy(void *p){
    int one = 1;    memcpy(p, &one, 4);
    float two=2.0f; memcpy(p, &two, 4);
    int r;          memcpy(&r, p, 4);
    return r;
}
int main(void){
    float f = 2.0f;
    printf("Читаем биты 2.0f (ожидаем 1073741824 = 0x40000000):\n");
    printf("  memcpy      -> %10u\n", via_memcpy(f));
    printf("  union       -> %10u\n", via_union(f));
    printf("  char*       -> %10u\n", via_char(f));
    printf("  may_alias   -> %10u\n", via_may_alias(f));
    printf("  *(uint32_t*)-> %10u   <- это UB, значение совпало случайно\n", via_cast_UB(f));
    int storage = 0;
    printf("\nЗапись int, потом float, потом чтение как int:\n");
    printf("  через memcpy-> %10d\n", rmw_memcpy(&storage));
    return 0;
}
