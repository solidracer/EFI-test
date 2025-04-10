# SOURCE: https://wiki.osdev.org/GNU-EFI#Libraries, https://wiki.osdev.org/UEFI

cc ?= gcc

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c,build/%.o,$(src))

# these directories might change based on how you installed GNU-EFI
efi-inc = /usr/include/efi
efi-libs = /usr/lib

qemu-ram-cap = 4G

all: build/main.efi

./build:
	@mkdir -p build

build/main.efi: ./build build/main.so
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 build/main.so build/main.efi

build/main.so: $(obj)
	ld -shared -Bsymbolic -L$(efi-libs) -T$(efi-libs)/elf_x86_64_efi.lds $(efi-libs)/crt0-efi-x86_64.o $(obj) -o build/main.so -lgnuefi -lefi

build/%.o: src/%.c
	$(cc) -DEFIAPI=__attribute__\(\(ms_abi\)\) -I$(efi-inc) -Wall -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c $< -o $@

run: all
	uefi-run -b ovmf/OVMF.fd -q /usr/bin/qemu-system-x86_64 -f kernel.bin build/main.efi -- -cpu qemu64 -m $(qemu-ram-cap)

clean:
	@rm -rf build
