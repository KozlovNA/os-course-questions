/* Три способа восстановить стек вызовов в программе.
 * Сборка: gcc -O0 -fno-omit-frame-pointer -rdynamic unwind.c -o build/unwind_fp
 *         gcc -O2 -fomit-frame-pointer   -rdynamic unwind.c -o build/unwind_nofp
 * -rdynamic нужен, чтобы dladdr находил имена функций самой программы. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uintptr_t stack_lo, stack_hi;   /* границы стека текущего потока */
extern char __executable_start[], etext[];

static void stack_bounds(void) {
    pthread_attr_t a;
    void *addr;
    size_t size;
    pthread_getattr_np(pthread_self(), &a);
    pthread_attr_getstack(&a, &addr, &size);
    stack_lo = (uintptr_t)addr;
    stack_hi = stack_lo + size;
}

static const char *name_of(uintptr_t pc) {
    Dl_info info;
    if (dladdr((void *)pc, &info) && info.dli_sname) return info.dli_sname;
    return "?";
}

/* 1. Обход цепочки указателей кадра, как mon_backtrace в JOS, но с проверками:
 * указатель должен лежать в стеке потока, быть выровнен на 8 и строго расти. */
__attribute__((noinline)) static void walk_frame_pointers(void) {
    uintptr_t fp = (uintptr_t)__builtin_frame_address(0);
    printf("  обход по rbp:\n");
    for (int depth = 0; depth < 64; depth++) {
        if (fp == 0) { printf("    стоп: rbp = 0 (самый глубокий кадр помечен нулём)\n"); return; }
        if (fp < stack_lo || fp + 16 > stack_hi) { printf("    стоп: rbp = %#lx вне стека потока\n", (unsigned long)fp); return; }
        if (fp % 8) { printf("    стоп: rbp = %#lx не выровнен на 8\n", (unsigned long)fp); return; }
        uintptr_t *frame = (uintptr_t *)fp;
        uintptr_t ret = frame[1];
        printf("    rbp %#lx  rip %#lx  %s\n", (unsigned long)fp, (unsigned long)ret, name_of(ret));
        if (frame[0] != 0 && frame[0] <= fp) { printf("    стоп: следующий rbp не выше текущего (цепочка испорчена)\n"); return; }
        fp = frame[0];
    }
    printf("    стоп: превышена глубина\n");
}

/* 2. glibc backtrace(): раскрутка по таблицам .eh_frame (CFI) через libgcc,
 * указатель кадра не нужен. */
__attribute__((noinline)) static void walk_cfi(void) {
    void *pcs[32];
    int n = backtrace(pcs, 32);
    printf("  backtrace() по .eh_frame: %d кадров\n", n);
    for (int i = 0; i < n; i++)
        printf("    rip %p  %s\n", pcs[i], name_of((uintptr_t)pcs[i]));
}

/* Похоже ли значение на адрес возврата: лежит в коде программы и перед ним
 * стоит инструкция call (e8 rel32 или ff /2 с регистром или памятью). */
static int looks_like_return_address(uintptr_t v) {
    if (v < (uintptr_t)__executable_start + 8 || v >= (uintptr_t)etext) return 0;
    const unsigned char *p = (const unsigned char *)v;
    if (p[-5] == 0xe8) return 1;                                     /* call rel32 */
    if (p[-2] == 0xff && (p[-1] & 0x38) == 0x10) return 1;           /* call *%reg */
    if (p[-3] == 0xff && (p[-2] & 0x38) == 0x10) return 1;           /* call *disp8(%reg) */
    if (p[-6] == 0xff && (p[-5] & 0x38) == 0x10) return 1;           /* call *disp32(...) */
    return 0;
}

/* 3. Эвристика: просмотреть слова стека подряд и оставить похожие на адреса
 * возврата. Работает без указателя кадра и без таблиц, но даёт ложные срабатывания:
 * в стеке остаются старые адреса от уже завершившихся вызовов. */
__attribute__((noinline)) static void walk_scan(void) {
    uintptr_t sp = (uintptr_t)__builtin_frame_address(0);
    printf("  просмотр стека (только код программы, 512 слов):\n");
    for (int i = 0; i < 512 && sp + 8 * i < stack_hi; i++) {
        uintptr_t v = ((uintptr_t *)sp)[i];
        if (looks_like_return_address(v))
            printf("    [rsp+%3d] %#lx  %s\n", 8 * i, (unsigned long)v, name_of(v));
    }
}

/* Цепочка вызовов, которую будем восстанавливать: main → level1 → level2 → level3.
 * volatile и сложение после вызова мешают компилятору превратить вызовы в переходы. */
volatile int sink;
__attribute__((noinline)) int level3(int x) {
    walk_frame_pointers();
    walk_cfi();
    walk_scan();
    return x + sink;
}
__attribute__((noinline)) int level2(int x) { return level3(x + 1) + sink; }
__attribute__((noinline)) int level1(int x) { return level2(x + 1) + sink; }

int main(void) {
    stack_bounds();
    return level1(0) == 12345;
}
