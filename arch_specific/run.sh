#!/bin/bash
# Все примеры и бенчмарки. Нужен gcc с поддержкой C23 (<stdbit.h>) и x86-64 CPU с popcnt.
cd "$(dirname "$0")" || exit 1
echo "##### 1. Бенчмарк: одна операция пятью способами (best-of-5, ~30 c)"
gcc -O2 -Wall popcnt_bench.c popcnt_asm.S -o popcnt_bench && ./popcnt_bench
echo; echo "##### 2. Что компилятор порождает для intrinsic и для inline asm"
gcc -O2 -S -o - popcnt_bench.c 2>/dev/null | awk '/^p_intrinsic:/,/\.size\tp_intrinsic/' | grep -vE '^\s*\.(cfi|p2align|size|LFE)'
gcc -O2 -S -o - popcnt_bench.c 2>/dev/null | awk '/^p_inline_asm:/,/\.size\tp_inline_asm/' | grep -vE '^\s*\.(cfi|p2align|size|LFE)'
echo; echo "##### 3. Ловушки inline asm (сравните -O0 и -O2)"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w pitfalls.c -o pit && ./pit; done
echo "== свёртка констант: builtin сворачивается, asm нет"
gcc -O2 -S -o - -w pitfalls.c | awk '/^(const_builtin|const_asm):/,/ret/' | grep -vE '^\s*\.(cfi|p2align)'
echo; echo "##### 4. Переносимость и диспетчеризация по CPU"
gcc -O2 -Wall portable.c -o portable && ./portable
