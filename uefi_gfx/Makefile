# Нужны: gnu-efi (apt install gnu-efi), для запуска qemu-system-x86 + ovmf. Пути для Ubuntu/Debian.
ARCH   = x86_64
INC    = -I/usr/include/efi -I/usr/include/efi/$(ARCH)
CFLAGS = $(INC) -fpic -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone -Wall
LDFLAGS= -nostdlib -znocombreloc -T /usr/lib/elf_$(ARCH)_efi.lds -shared -Bsymbolic -L/usr/lib /usr/lib/crt0-efi-$(ARCH).o

all: gop_model gop_app.efi
gop_model: gop_model.c
	gcc -O2 -Wall -Wextra $< -o $@
gop_app.so: gop_app.o
	ld $(LDFLAGS) $^ -o $@ -lefi -lgnuefi
gop_app.efi: gop_app.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel* -j .rela* -j .reloc --target=efi-app-$(ARCH) $< $@
clean:
	rm -rf gop_model *.o *.so *.efi esp
