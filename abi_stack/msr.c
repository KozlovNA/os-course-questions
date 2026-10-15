/* MSR из пользовательского режима: что можно узнать и что произойдёт.
 * Сборка: gcc -O2 -Wall msr.c -o build/msr */
#include <cpuid.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    unsigned a = 0, b = 0, c = 0, d = 0;

    /* 1. Прежде чем трогать MSR, спрашивают CPUID, реализован ли он.
     *    IA32_EFER (C000_0080H) существует, если CPUID.80000001H:EDX[20] (NX)
     *    или EDX[29] (Long Mode) — так записано в таблице MSR в SDM, том 4. */
    __get_cpuid(0x80000001, &a, &b, &c, &d);
    printf("1. CPUID.80000001H:EDX: NX(bit 20) = %u, LM(bit 29) = %u -> IA32_EFER %s\n",
           (d >> 20) & 1, (d >> 29) & 1, ((d >> 20) & 1) || ((d >> 29) & 1) ? "есть" : "нет");

    /* 2. Работаем ли мы под гипервизором: CPUID.1:ECX[31], и тогда лист 40000000H
     *    содержит подпись гипервизора. */
    __get_cpuid(1, &a, &b, &c, &d);
    printf("2. CPUID.1:ECX[31] (hypervisor present) = %u", (c >> 31) & 1);
    if ((c >> 31) & 1) {
        char sig[13] = {0};
        __cpuid(0x40000000, a, b, c, d);
        memcpy(sig, &b, 4); memcpy(sig + 4, &c, 4); memcpy(sig + 8, &d, 4);
        printf(", подпись гипервизора: \"%s\"", sig);
    }
    printf("\n");

    /* 3. rdmsr в кольце 3: процессор выдаёт #GP, ядро превращает его в SIGSEGV. */
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        uint32_t lo, hi;
        __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
        printf("   rdmsr выполнился?! EFER = %#x%08x\n", hi, lo);
        _exit(0);
    }
    int st;
    waitpid(pid, &st, 0);
    printf("3. rdmsr(IA32_EFER) в пользовательском режиме -> %s\n",
           WIFSIGNALED(st) ? strsignal(WTERMSIG(st)) : "завершился нормально");

    /* 4. Linux даёт доступ к MSR из пользовательского режима через драйвер msr:
     *    pread(fd, &val, 8, номер_MSR) на /dev/cpu/N/msr, нужны права root. */
    int fd = open("/dev/cpu/0/msr", O_RDONLY);
    if (fd < 0)
        printf("4. /dev/cpu/0/msr: %s\n", strerror(errno));
    else {
        uint64_t v;
        ssize_t n = pread(fd, &v, 8, 0xC0000080);
        printf("4. /dev/cpu/0/msr: pread(EFER) -> %zd, EFER = %#lx\n", n, (unsigned long)v);
        close(fd);
    }
    return 0;
}
