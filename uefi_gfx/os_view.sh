#!/bin/bash
# Что видит уже загруженная ОС: артефакты GOP на реальной машине (Linux).
echo "== Загружена через UEFI?"; [ -d /sys/firmware/efi ] && echo "да" || echo "нет (BIOS/WSL/контейнер): дальше смотреть нечего"
echo "== dmesg: efifb / simpledrm / sysfb"; dmesg 2>/dev/null | grep -Ei 'efifb|simpledrm|sysfb|simple-framebuffer' | head
echo "== Фреймбуфер, отданный загрузчиком (BOOTFB) в /proc/iomem"; grep -Ei 'BOOTFB|efifb' /proc/iomem 2>/dev/null
echo "== fbset / fb0"; ls -l /dev/fb0 2>&1; cat /sys/class/graphics/fb0/{name,virtual_size,stride,bits_per_pixel} 2>&1
echo "== BGRT (логотип прошивки, ACPI)"; ls -l /sys/firmware/acpi/bgrt/ 2>&1
echo "== Драйвер, забравший экран (после хендоффа)"; ls /sys/class/drm/ 2>&1 | head
