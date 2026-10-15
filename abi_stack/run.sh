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

echo; echo "##### 3а. Вызванная функция: аргументы 7 и 8 на стеке (gcc -O0)"
gcc -O0 -S -o - convention_callee.c | awk '/^sum8:/,/ret/' | grep -vE '^\s*\.(cfi)|^\.L|endbr64'

echo; echo "##### 3б. Возврат: 16-байтовая структура в rax:rdx, double в xmm0 (gcc -O2)"
gcc -O2 -S -o - convention.c | grep -vE '^\s*\.(cfi|p2align|size|type|globl)|^\.L|endbr64' | awk '/^(make_pair|half):/{p=1} p{print} p && /ret/{p=0}'

echo; echo "##### 4. Выключает ли -O2 указатель кадра"
for O in -O0 -O2; do
    printf '  gcc %s: ' "$O"
    gcc $O -Q --help=optimizers | grep -E '^\s*-fomit-frame-pointer' | tr -s ' '
done

echo; echo "##### 5. Раскрутка стека тремя способами: с указателем кадра (-O0)"
gcc -O0 -fno-omit-frame-pointer -rdynamic -Wall unwind.c -o build/unwind_fp && ./build/unwind_fp

echo; echo "##### 6. То же без указателя кадра (-O2 -fomit-frame-pointer)"
gcc -O2 -fomit-frame-pointer -rdynamic -Wall unwind.c -o build/unwind_nofp && ./build/unwind_nofp

echo; echo "##### 7. MSR из пользовательского режима"
gcc -O2 -Wall msr.c -o build/msr && ./build/msr
