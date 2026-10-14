#!/bin/bash
# Эксперименты к REPORT.md: выравнивание стека и кучи, соглашение о вызовах.
# Нужен gcc и x86-64 Linux.
cd "$(dirname "$0")" || exit 1
mkdir -p build

echo "##### 1. Выравнивание стека и кучи"
gcc -O0 -Wall abi_stack.c probe.S -o build/abi_stack && ./build/abi_stack

echo; echo "##### 2. Как sse_victim обращается к стеку (gcc -O0)"
gcc -O0 -S -o - abi_stack.c 2>/dev/null | awk '/^sse_victim:/,/ret/' | grep -E 'movaps|sub|push|ret'

echo; echo "##### 3. Соглашение о вызовах (gcc -O2)"
gcc -O2 -S -o - convention.c | grep -vE '^\s*\.(cfi|p2align|size|type|globl|file|text|ident|section|LFB|LFE)|^\.L[A-Z]' \
    | awk '/^call_with_8_args:/{p=1} p{print} p && /^make_big:/{m=1} m && /ret/{exit}'

echo; echo "##### 4. Выключает ли -O2 указатель кадра"
for O in -O0 -O2; do
    printf '  gcc %s: ' "$O"
    gcc $O -Q --help=optimizers | grep -E '^\s*-fomit-frame-pointer' | tr -s ' '
done
