#!/bin/bash
# Скачивает gnu-efi, OVMF, QEMU и seabios БЕЗ установки в систему (без sudo)
# и распаковывает их в ./deps. Так материалы и готовились.
set -e
cd "$(dirname "$0")"
mkdir -p deps/debs && cd deps/debs
apt-get download gnu-efi ovmf seabios \
                 qemu-system-x86 qemu-system-common qemu-system-data \
                 libaio1t64 libfdt1 libpmem1 libslirp0 liburing2 libndctl6 libdaxctl1
cd .. && for d in debs/*.deb; do dpkg-deb -x "$d" root/; done
cp root/usr/share/seabios/*.bin root/usr/share/qemu/ 2>/dev/null || true
echo
echo "Готово. Дальше:"
echo "  make GNUEFI=\$PWD/deps/root/usr"
echo "  ./run_qemu.sh"
