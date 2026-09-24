// Какая именно операция исчезает при strict aliasing: запись *f = 2.0f или чтение *i ?
#include <stdio.h>
#include <string.h>

__attribute__((noinline)) int alias(int *i, float *f){
    *i = 1;         // запись A
    *f = 2.0f;      // запись B  <- гипотеза: "её выкинули как ненужную"
    return *i;      // чтение C  <- на самом деле выкидывают ЭТО
}

int main(void){
    int storage = 0;
    int returned = alias(&storage, (float*)&storage);

    unsigned char bytes[4]; memcpy(bytes, &storage, 4);      // смотрим байты корректно
    unsigned raw; memcpy(&raw, &storage, 4);

    printf("функция вернула              : %d\n", returned);
    printf("что РЕАЛЬНО лежит в памяти   : %u  (байты: %02X %02X %02X %02X)\n",
           raw, bytes[0], bytes[1], bytes[2], bytes[3]);
    printf("\n");
    if (raw == 1073741824u)
        printf("  -> в памяти биты 2.0f, значит запись B ВЫПОЛНИЛАСЬ и НЕ была выкинута\n");
    else if (raw == 1u)
        printf("  -> в памяти 1, значит запись B действительно исчезла\n");
    if (returned == 1 && raw == 1073741824u)
        printf("  -> но вернулась 1, которой в памяти нет: исчезло ЧТЕНИЕ C\n");
    return 0;
}
