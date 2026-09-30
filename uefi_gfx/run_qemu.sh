#!/bin/bash
# Запускает gop_app.efi в QEMU с прошивкой OVMF, забирает вывод из последовательного
# порта и снимает скриншот экрана.
#   системные пакеты:  qemu-system-x86 + ovmf
#   локальные (без sudo): ./fetch_deps.sh, затем этот скрипт сам их подхватит
set -e
cd "$(dirname "$0")"

if [ -d deps/root ]; then                     # локально распакованные пакеты
    R=$PWD/deps/root
    export LD_LIBRARY_PATH=$R/usr/lib/x86_64-linux-gnu
    export QEMU_MODULE_DIR=$R/usr/lib/x86_64-linux-gnu/qemu
    QEMU=$R/usr/bin/qemu-system-x86_64
    DATA=$R/usr/share/qemu
    CODE=$R/usr/share/OVMF/OVMF_CODE_4M.fd
    VARS=$R/usr/share/OVMF/OVMF_VARS_4M.fd
else                                          # системные
    QEMU=$(command -v qemu-system-x86_64)
    DATA=/usr/share/qemu
    CODE=$(ls /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd /usr/share/ovmf/OVMF.fd 2>/dev/null | head -1)
    VARS=$(ls /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd 2>/dev/null | head -1)
fi
[ -x "$QEMU" ] || { echo "qemu-system-x86_64 не найден: поставьте qemu-system-x86 или запустите ./fetch_deps.sh"; exit 1; }
[ -f "$CODE" ] || { echo "прошивка OVMF не найдена: поставьте ovmf или запустите ./fetch_deps.sh"; exit 1; }

[ -f gop_app.efi ] || { echo "нет gop_app.efi, соберите: make (или make GNUEFI=\$PWD/deps/root/usr)"; exit 1; }

rm -rf esp serial.txt shot.ppm
mkdir -p esp/EFI/BOOT && cp gop_app.efi esp/EFI/BOOT/BOOTX64.EFI
cp "$VARS" vars.fd

echo "Запуск OVMF, ждём ~25 с (приложение останавливается на ожидании клавиши)..."
( sleep 25; echo "screendump $PWD/shot.ppm"; sleep 5; echo quit ) | \
"$QEMU" -machine q35 -m 512 -L "$DATA" -net none \
  -drive if=pflash,format=raw,unit=0,readonly=on,file="$CODE" \
  -drive if=pflash,format=raw,unit=1,file=vars.fd \
  -drive format=raw,file=fat:rw:esp \
  -vga std -display none -serial file:serial.txt -monitor stdio -no-reboot >/dev/null 2>&1 || true

echo; echo "=== вывод приложения (последовательный порт) ==="
tr -d '\r' < serial.txt | sed -n '/UGA:/,$p'
echo; echo "=== скриншот: $PWD/shot.ppm ($(du -h shot.ppm 2>/dev/null | cut -f1)) ==="
