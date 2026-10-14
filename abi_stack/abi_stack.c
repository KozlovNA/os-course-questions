/* Проверка требований System V x86-64 ABI к стеку и куче.
 * Сборка: gcc -O0 -Wall abi_stack.c probe.S -o build/abi_stack
 * (-O0 нужен для части 2: так компилятор гарантированно кладёт __m128
 * на стек инструкцией movaps, которая требует выравнивания на 16.) */
#include <immintrin.h>
#include <signal.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

uintptr_t rsp_at_entry(void);
void call_misaligned(void (*fn)(void));

/* Функция, которой нужен выровненный стек: локальная __m128 сохраняется
 * в кадр через movaps. Компилятор считает, что ABI соблюдён, и не проверяет. */
__attribute__((noinline)) static void sse_victim(void) {
    __m128 v = _mm_set1_ps(1.0f);
    volatile __m128 local = v;
    (void)local;
}

/* Запускает fn в дочернем процессе и сообщает, чем он закончился. */
static void run_in_child(const char *what, void (*fn)(void)) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        fn();
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status))
        printf("  %-38s -> убит сигналом %s\n", what, strsignal(WTERMSIG(status)));
    else
        printf("  %-38s -> завершился нормально\n", what);
}

static void good_call(void) { sse_victim(); }
static void bad_call(void) { call_misaligned(sse_victim); }

int main(void) {
    printf("1. Выравнивание RSP на входе в функцию\n");
    for (int i = 0; i < 3; i++) {
        uintptr_t rsp = rsp_at_entry();
        printf("  вызов %d: RSP на входе = %#lx, RSP %% 16 = %lu  (до call: %% 16 = %lu)\n",
               i + 1, (unsigned long)rsp, (unsigned long)(rsp % 16),
               (unsigned long)((rsp + 8) % 16));
    }

    printf("\n2. Функция с __m128 на стеке\n");
    run_in_child("обычный вызов", good_call);
    run_in_child("вызов со стеком, сдвинутым на 8", bad_call);

    printf("\n3. Выравнивание кучи\n");
    printf("  alignof(max_align_t) = %zu\n", alignof(max_align_t));
    size_t sizes[] = {1, 3, 8, 24, 100, 4000};
    unsigned worst = 4096;
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        void *p = malloc(sizes[i]);
        uintptr_t a = (uintptr_t)p;
        unsigned align = a ? (unsigned)(a & -a) : 0;   /* наибольшая степень двойки, делящая адрес */
        if (align > 4096) align = 4096;
        if (align < worst) worst = align;
        printf("  malloc(%4zu) = %p, адрес кратен %u\n", sizes[i], p, align);
    }
    printf("  наименьшее выравнивание среди вызовов: %u\n", worst);
    void *big = aligned_alloc(64, 256);
    printf("  aligned_alloc(64, 256) = %p, %% 64 = %lu\n", big,
           (unsigned long)((uintptr_t)big % 64));
    return 0;
}
