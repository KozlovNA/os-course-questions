#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
__attribute__((noinline)) int cmp(int x){ return x + 1 > x; }
__attribute__((noinline)) int nullchk(int *p){ int v = *p; if(!p) return -1; return v; }
__attribute__((noinline)) int cnt(int start){ int n=0; for(int i=start;i>=start;i++){ if(++n>100) break;} return n; }
int main(int argc,char**argv){
  int m = argc>1 ? atoi(argv[1]) : INT_MAX;   // значение неизвестно компилятору
  printf("INT_MAX+1 > INT_MAX  -> %d\n", cmp(m));
  printf("cnt (цикл i>=start; i++) -> %d итераций (100 = 'бесконечный' цикл)\n", cnt(m));
  int a=5; printf("nullchk(&a) -> %d\n", nullchk(&a));
  int s = 1; s <<= 31 + (argc>5);  printf("1<<31 = %d\n", s);
  return 0; }
