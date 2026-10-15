/* Соглашение о вызовах System V x86-64 на примерах. Этот файл не запускается:
 * run.sh компилирует его с -O2 -S и показывает ассемблер. */

long sum8(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
long next(long x);

/* Восемь аргументов: первые шесть уйдут в rdi, rsi, rdx, rcx, r8, r9,
 * седьмой и восьмой вызывающий положит на стек. */
long call_with_8_args(void) {
    return sum8(1, 2, 3, 4, 5, 6, 7, 8);
}

/* x нужен после вызова next(). Регистры аргументов caller-saved, next()
 * может их испортить, поэтому компилятор держит x в callee-saved регистре
 * (rbx) и обязан сохранить старое значение этого регистра в прологе. */
long keep_across_call(long x) {
    return next(x) + x;
}

/* Структура больше 16 байт возвращается через скрытый указатель:
 * вызывающий передаёт адрес буфера в rdi, функция возвращает его же в rax. */
struct big { long a, b, c; };
struct big make_big(long v) {
    struct big r = { v, v + 1, v + 2 };
    return r;
}

/* Структура из двух long (16 байт) возвращается в паре rax:rdx. */
struct pair { long a, b; };
struct pair make_pair(long v) {
    struct pair r = { v, v * 2 };
    return r;
}

/* Число с плавающей точкой возвращается в xmm0. */
double half(long v) {
    return v / 2.0;
}
