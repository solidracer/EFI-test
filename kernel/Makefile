cc = gcc
ld = ld

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c,build/%.o,$(src))

build/kernel.bin: ./build $(obj)
	$(ld) -T link.ld $(obj) -o build/kernel.bin

build/%.o: src/%.c
	$(cc) -m64 -ffreestanding -c $< -o $@

./build:
	@mkdir -p build

clean:
	@rm -rf build
