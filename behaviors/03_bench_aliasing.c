#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#define N (1<<16)
#define REP 60000
static double now(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
// 1. знаковый i (UB при переполнении) против беззнакового: индекс int / unsigned, шаг 2 -> векторизация/индукция
__attribute__((noinline)) void sgn(float*a,const float*b,int n){ for(int i=0;i<n;i+=2) a[i]=b[i]*2.f+1.f; }
__attribute__((noinline)) void uns(float*a,const float*b,unsigned n){ for(unsigned i=0;i<n;i+=2) a[i]=b[i]*2.f+1.f; }
// 2. strict aliasing: int* и float* не пересекаются
__attribute__((noinline)) void alias_strict(int*restrict_ok,float*f,int n){ for(int i=0;i<n;i++){ f[i]+=1.f; restrict_ok[0]+=1; } }
// 3. memcpy vs UB-приведение типа (type punning) — оба корректны через memcpy
int main(int argc,char**argv){
  float*a=malloc(N*4),*b=malloc(N*4); for(int i=0;i<N;i++){a[i]=0;b[i]=i;}
  double t=now(); for(int r=0;r<REP;r++) sgn(a,b,N); double ts=now()-t;
  t=now(); for(int r=0;r<REP;r++) uns(a,b,N); double tu=now()-t;
  printf("signed   int i (i+=2): %.3f s\n",ts);
  printf("unsigned    i (i+=2): %.3f s   (x%.2f)\n",tu,tu/ts);
  int c[1]={0}; float*f=calloc(N,4);
  t=now(); for(int r=0;r<REP;r++) alias_strict(c,f,N); double ta=now()-t;
  printf("strict-aliasing loop: %.3f s (%d)\n",ta,c[0]);
  return 0; }
