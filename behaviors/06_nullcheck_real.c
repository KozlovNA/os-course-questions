// Настоящая опасность: разыменование УДАЛЯЕТСЯ как мёртвый код,
// но ВЫВЕДЕННЫЙ ИЗ НЕГО ФАКТ "p != NULL" остаётся, и проверка исчезает.
// Это схема CVE-2009-1897 (драйвер tun в ядре Linux).
#include <stdio.h>

struct cfg { int limit; };

// Значение s->limit не используется до проверки => компилятор выбросит саму загрузку.
// Но факт "раз s разыменовали, значит s != NULL" он уже записал и проверку удалит.
__attribute__((noinline))
int handle(struct cfg *s, int *out){
    int lim = s->limit;          // 1) разыменование; его результат пока не нужен
    if (!s) {                    // 2) на -O2 удалится: "s не может быть NULL"
        *out = 0;
        return -1;               //    штатный отказ, который мы рассчитывали получить
    }
    *out = lim * 2;              // 3) сюда попадём с s == NULL, хотя код это запрещал
    return 0;
}

int main(void){
    struct cfg c = { 21 };
    int out = -777;
    printf("handle(&c)  -> rc=%d out=%d\n", handle(&c, &out), out);
    out = -777;
    printf("handle(NULL): проверка должна вернуть rc=-1 out=0\n");
    int rc = handle(NULL, &out);
    printf("handle(NULL)-> rc=%d out=%d\n", rc, out);
    return 0;
}
