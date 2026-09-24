// Почему пример с nullchk вообще что-то значит: дело не в возвращаемом значении,
// а в том, что проверка if(!p) ФИЗИЧЕСКИ ИСЧЕЗАЕТ из машинного кода.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

// ПЛОХО: разыменование ДО проверки. Компилятор рассуждает так:
//   "*p выполнено => если бы p был NULL, это UB => значит p != NULL => if(!p) всегда ложь => удаляю"
__attribute__((noinline)) int bad(int *p){
    int v = *p;              // <-- разыменовали
    if (!p) return -1;       // <-- эта проверка будет выброшена на -O2
    return v;
}
// ХОРОШО: проверка ДО разыменования. Удалять нечего.
__attribute__((noinline)) int good(int *p){
    if (!p) return -1;
    return *p;
}

static jmp_buf jb;
static void on_segv(int s){ (void)s; longjmp(jb, 1); }

static void try_call(const char *name, int (*f)(int*), int *p){
    if (setjmp(jb) == 0) printf("  %-6s(NULL) -> вернула %d\n", name, f(p));
    else                 printf("  %-6s(NULL) -> SIGSEGV (падение)\n", name);
}

int main(void){
    int a = 5;
    printf("== Со ВАЛИДНЫМ указателем оба варианта одинаковы (как вы и сказали):\n");
    printf("  bad(&a)=%d  good(&a)=%d\n", bad(&a), good(&a));

    printf("\n== А теперь передаём реальный NULL. Вот тут и видна разница:\n");
    signal(SIGSEGV, on_segv);
    try_call("bad",  bad,  NULL);
    try_call("good", good, NULL);
    return 0;
}
