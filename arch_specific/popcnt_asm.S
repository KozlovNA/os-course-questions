/* Способ 3: функции в отдельном .S-файле (x86-64, System V ABI, AT&T-синтаксис).
   rdi, rsi, rdx, rcx, r8, r9 - аргументы; rax - результат; rax, rcx, rdx, rsi, rdi, r8-r11 можно портить. */
    .text
    .globl  popcnt1                 /* uint64_t popcnt1(uint64_t x) */
    .type   popcnt1, @function
popcnt1:
    popcnt  %rdi, %rax
    ret

    .globl  popcnt_words            /* uint64_t popcnt_words(const uint64_t *p, size_t n) — наивно */
    .type   popcnt_words, @function
popcnt_words:
    xorl    %eax, %eax
    testq   %rsi, %rsi
    jz      2f
1:  popcnt  (%rdi), %rdx            /* у popcnt есть ложная зависимость по rdx на части CPU */
    addq    %rdx, %rax
    addq    $8, %rdi
    decq    %rsi
    jnz     1b
2:  ret

    .globl  popcnt_words_xor        /* то же, но с обнулением приёмника: рвёт ложную зависимость */
    .type   popcnt_words_xor, @function
popcnt_words_xor:
    xorl    %eax, %eax
    testq   %rsi, %rsi
    jz      2f
1:  xorl    %edx, %edx
    popcnt  (%rdi), %rdx
    addq    %rdx, %rax
    addq    $8, %rdi
    decq    %rsi
    jnz     1b
2:  ret
    .section .note.GNU-stack,"",@progbits
