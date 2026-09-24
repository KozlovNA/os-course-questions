// Почему volatile не чинит strict aliasing и на какой стороне он вообще что-то меняет.
#include <stdio.h>
#include <string.h>

typedef float __attribute__((may_alias)) float_ma;   // расширение gcc: "этот тип может алиасить всё"

// A) исходный вариант
__attribute__((noinline)) int a_plain(int *i, float *f){ *i=1; *f=2.0f; return *i; }
// B) volatile на ЗАПИСИ (то, что вы попробовали)
__attribute__((noinline)) int b_vol_write(int *i, volatile float *f){ *i=1; *f=2.0f; return *i; }
// C) volatile на ЧТЕНИИ
__attribute__((noinline)) int c_vol_read(volatile int *i, float *f){ *i=1; *f=2.0f; return *i; }
// D) volatile на обеих
__attribute__((noinline)) int d_vol_both(volatile int *i, volatile float *f){ *i=1; *f=2.0f; return *i; }
// E) may_alias — сообщаем компилятору правду об алиасинге
__attribute__((noinline)) int e_may_alias(int *i, float_ma *f){ *i=1; *f=2.0f; return *i; }
// F) memcpy — корректный способ без всякого UB
__attribute__((noinline)) int f_memcpy(int *i, float *f){
    int one=1; memcpy(i,&one,4); float two=2.0f; memcpy(f,&two,4); int r; memcpy(&r,i,4); return r; }

int main(void){
    int st; float *pf;
    struct { const char *name; int v; } res[6];
    st=0; pf=(float*)&st; res[0]=(typeof(res[0])){"A) без volatile            ", a_plain(&st,pf)};
    st=0; pf=(float*)&st; res[1]=(typeof(res[0])){"B) volatile float *f  (ЗАПИСЬ)", b_vol_write(&st,pf)};
    st=0; pf=(float*)&st; res[2]=(typeof(res[0])){"C) volatile int   *i  (ЧТЕНИЕ)", c_vol_read(&st,pf)};
    st=0; pf=(float*)&st; res[3]=(typeof(res[0])){"D) volatile на обеих       ", d_vol_both(&st,pf)};
    st=0; pf=(float*)&st; res[4]=(typeof(res[0])){"E) may_alias               ", e_may_alias(&st,(float_ma*)pf)};
    st=0; pf=(float*)&st; res[5]=(typeof(res[0])){"F) memcpy (без UB)         ", f_memcpy(&st,pf)};
    for(int k=0;k<6;k++) printf("  %-32s -> %10d %s\n", res[k].name, res[k].v,
        res[k].v==1073741824 ? "(биты 2.0f: память перечитана)" : "(записанная 1: чтение свёрнуто)");
    return 0;
}
