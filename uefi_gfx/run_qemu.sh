#!/bin/bash
# Запуск gop_app.efi в QEMU+OVMF. apt install qemu-system-x86 ovmf
cd "$(dirname "$0")" || exit 1
mkdir -p esp/EFI/BOOT && cp gop_app.efi esp/EFI/BOOT/BOOTX64.EFI
FW=$(ls /usr/share/OVMF/OVMF_CODE*.fd /usr/share/ovmf/OVMF.fd 2>/dev/null | head -1)
exec qemu-system-x86_64 -machine q35 -m 512 -bios "$FW" -vga std \
  -drive format=raw,file=fat:rw:esp "$@"      # добавьте -nographic для консоли; -vga virtio даёт другой pixel format
