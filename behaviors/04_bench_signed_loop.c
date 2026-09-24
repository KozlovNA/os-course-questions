#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N (1<<16)
#define REP 60000
static double now(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
__attribute__((noinline)) void sgn(float*a,const float*b,int n){ for(int i=0;i<=n;i++) a[i]=b[i]*2.f+1.f; }
__attribute__((noinline)) void uns(float*a,const float*b,unsigned n){ for(unsigned i=0;i<=n;i++) a[i]=b[i]*2.f+1.f; }
int main(){
  float*a=malloc((N+1)*4),*b=malloc((N+1)*4); for(int i=0;i<=N;i++){a[i]=0;b[i]=i;}
  double t=now(); for(int r=0;r<REP;r++) sgn(a,b,N-1); double ts=now()-t;
  t=now(); for(int r=0;r<REP;r++) uns(a,b,N-1); double tu=now()-t;
  printf("signed   i<=n: %.3f s\nunsigned i<=n: %.3f s  (x%.2f slower)\n",ts,tu,tu/ts); }
