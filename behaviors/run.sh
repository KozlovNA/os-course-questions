#!/bin/bash
# Запуск всех примеров и бенчмарков. Нужен gcc.
cd "$(dirname "$0")" || exit 1
mkdir -p build
echo "##### 01: implementation-defined / unspecified"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 01_behaviors.c -o build/01 && build/01; done
echo; echo "##### 02: UB и оптимизатор"
for f in "-O0" "-O2" "-O2 -fwrapv"; do echo "== gcc $f"; gcc $f -w 02_ub_optimizer.c -o build/02 && build/02; done
echo "== UBSan"; gcc -O0 -g -fsanitize=undefined -w 02_ub_optimizer.c -o build/02s && build/02s 2>&1 | grep "runtime error"
echo "== asm (-O2): cmp и nullchk"; gcc -O2 -S -o - -w 02_ub_optimizer.c | awk '/^(cmp|nullchk):/,/ret/' | grep -vE '^\s*\.(cfi|p2align)'
echo; echo "##### 03: бенчмарк strict aliasing (~10 с)"
gcc -O2 -w 03_bench_aliasing.c -o build/03a && echo "-O2:                      $(build/03a | tail -1)"
gcc -O2 -fno-strict-aliasing -w 03_bench_aliasing.c -o build/03b && echo "-O2 -fno-strict-aliasing: $(build/03b | tail -1)"
echo; echo "##### 04: бенчмарк signed/unsigned i<=n (шумный)"
for f in "-O2 -ftree-vectorize" "-O2 -ftree-vectorize -fwrapv"; do echo "== gcc $f"; gcc $f -w 04_bench_signed_loop.c -o build/04 && build/04; done
echo; echo "##### 05: NULL — валидный указатель против NULL"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 05_nullcheck.c -o build/05 && build/05; done
echo; echo "##### 06: проверка исчезает из машинного кода (схема CVE-2009-1897)"
for O in -O0 -O2; do echo "== gcc $O, функция handle целиком:";
  gcc $O -S -o - -w 06_nullcheck_real.c | awk '/^handle:/,/^\.LFE/' | grep -vE '^\s*\.(cfi|p2align|LFE|size)'; done
echo; echo "##### 07: удалённая проверка -> НЕВЕРНЫЙ ОТВЕТ без падения"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 07_check_deleted.c -o build/07 && build/07; done
echo "== ассемблер add100_checked_after:"
for O in -O0 -O2; do echo "-- $O:"; gcc $O -S -o - -w 07_check_deleted.c | awk '/^add100_checked_after:/,/ret/' | grep -vE '^\s*\.(cfi|p2align)'; done
echo; echo "##### 08: откуда в strict aliasing берётся 1073741824"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 08_aliasing_bits.c -o build/08 && build/08; done
echo; echo "##### 09: почему volatile не чинит strict aliasing"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 09_volatile_aliasing.c -o build/09 && build/09; done
echo "== gcc -O2 -fno-strict-aliasing"; gcc -O2 -fno-strict-aliasing -w 09_volatile_aliasing.c -o build/09b && build/09b
echo "== ассемблер b_vol_write / c_vol_read / d_vol_both:"
for fn in b_vol_write c_vol_read d_vol_both; do gcc -O2 -S -o - -w 09_volatile_aliasing.c | awk "/^$fn:/,/^\.LFE/" | grep -vE '^\s*\.(cfi|p2align|LFE|size|LFB)'; done
echo; echo "##### 10: как получить type punning на -O2"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 10_punning_right.c -o build/10 && build/10; done
echo "== во что компилируется каждый способ (-O2):"
for fn in via_memcpy via_union via_char via_may_alias via_cast_UB; do printf "  %-14s: " $fn
  gcc -O2 -S -o - -w 10_punning_right.c | awk "/^$fn:/,/ret/" | grep -vE '\.(cfi|p2align)|^[a-z_]+:|^\.L|endbr64' | tr -s ' \t' ' ' | tr '\n' ';'; echo; done
gcc -O2 -S -o - -w 10_punning_right.c | awk '/^rmw_memcpy:/,/ret/' | grep -vE '^\s*\.(cfi|p2align)'
echo; echo "##### 11: что именно исчезает — запись или чтение"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 11_which_one_dies.c -o build/11 && build/11; done
gcc -O2 -S -o - -w 11_which_one_dies.c | awk '/^alias:/,/ret/' | grep -vE '^\s*\.(cfi|p2align)'
echo; echo "##### 12: UB против implementation-defined"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 12_ub_vs_impdef.c -o build/12 && build/12; done
echo; echo "##### 13: UB стирает код, а не подставляет значение"
for O in -O0 -O2; do echo "== gcc $O"; gcc $O -w 13_ub_not_a_value.c -o build/13 && build/13 | head -4; done
echo "== ассемблер f() (с UB) против g() (без UB):"
for fn in f g; do gcc -O2 -S -o - -w 13_ub_not_a_value.c | awk "/^$fn:/,/^\.LFE/" | grep -vE '\.(cfi|p2align|LFE|size|LFB)'; done
echo; echo "##### 14: абстрактная машина, наблюдаемое поведение, правило as-if"
gcc -O2 -w 14_abstract_machine.c -o build/14 && build/14
echo "== not_observable() на -O2:"; gcc -O2 -S -o - -w 14_abstract_machine.c | awk '/^not_observable:/,/ret/' | grep -vE '\.(cfi|p2align)'
