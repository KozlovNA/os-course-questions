# Сборка UEFI-приложения. Нужен gnu-efi.
#   системный:  make
#   локальный:  make GNUEFI=/путь/к/распакованному/корню   (см. fetch_deps.sh)
ARCH    = x86_64
GNUEFI ?= /usr
INC     = -I$(GNUEFI)/include/efi -I$(GNUEFI)/include/efi/$(ARCH)
CFLAGS  = $(INC) -fpic -ffreestanding -fno-stack-protector -fshort-wchar \
          -mno-red-zone -maccumulate-outgoing-args -Wall
LIBDIR  = $(GNUEFI)/lib
LDFLAGS = -nostdlib -znocombreloc -T $(LIBDIR)/elf_$(ARCH)_efi.lds -shared -Bsymbolic \
          -L$(LIBDIR) $(LIBDIR)/crt0-efi-$(ARCH).o

all: gop_model gop_app.efi

gop_model: gop_model.c
	gcc -O2 -Wall -Wextra $< -o $@

gop_app.o: gop_app.c
	gcc $(CFLAGS) -c $< -o $@

gop_app.so: gop_app.o
	ld $(LDFLAGS) $< -o $@ -lefi -lgnuefi

gop_app.efi: gop_app.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
	        -j .rel\* -j .rela\* -j .reloc \
	        --target=efi-app-$(ARCH) --subsystem=10 $< $@

clean:
	rm -rf gop_model *.o *.so *.efi esp vars.fd serial.txt shot.ppm
