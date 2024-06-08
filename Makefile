CC=gcc
CPPFLAGS=-Iinclude
CFLAGS=-g -c -O0 -m32 -MMD $(CPPFLAGS) -fno-pic -fno-pie -fno-stack-protector -fno-builtin -nostdlib -nostdinc -nodefaultlibs -nostartfiles
BOOT_ENTRY=0x7c00
BOOT_LDFLAGS=-m elf_i386 -Ttext $(BOOT_ENTRY) 

KERNEL_ENTRY=0x100000
KERNEL_LDFLAGS=-m elf_i386 -Ttext $(KERNEL_ENTRY)

KERNEL_OBJS=build/head.o build/main.o \
build/int/int.o build/int/interrupt.o \
build/lib/print.o build/lib/string.o \
build/mm/heap.o

.PHONY:all

all:builddir imagedir  imagefile
-include **/*.d
builddir:
	mkdir -p build
	mkdir -p build/boot
	mkdir -p build/int
	mkdir -p build/lib
	mkdir -p build/mm
imagedir:
	mkdir -p image
imagefile:./build/boot/boot.bin ./build/kernel.bin
	dd if=/dev/zero of=image/disk.img bs=16M count=1
	dd if=./build/boot/boot.bin of=image/disk.img conv=notrunc
	dd if=./build/kernel.bin of=image/disk.img conv=notrunc seek=1
./build/boot/boot.bin: boot/boot.S
	gcc $(CFLAGS) -o build/boot/boot.o boot/boot.S
	ld  $(BOOT_LDFLAGS) -o build/boot/boot.elf build/boot/boot.o
	objcopy -O binary build/boot/boot.elf build/boot/boot.bin
./build/kernel.bin:$(KERNEL_OBJS)
	ld $(KERNEL_LDFLAGS) -o build/kernel.elf $(KERNEL_OBJS)
	objcopy -O binary build/kernel.elf build/kernel.bin
./build/head.o:head.S
	gcc $(CFLAGS) -o $@ $<
./build/main.o:main.c
	gcc $(CFLAGS) -o $@ $<
./build/int/int.o:int/int.S
	gcc $(CFLAGS) -o $@ $<
./build/int/%.o:int/%.c
	gcc $(CFLAGS) -o $@ $<
./build/lib/%.o:lib/%.c
	gcc $(CFLAGS) -o $@ $<
./build/mm/%.o:mm/%.c
	gcc $(CFLAGS) -o $@ $<

.PHONY:clean debug run
clean:
	rm -rf build
	rm -rf image
debug:all
	qemu-system-i386  -smp 1 -m 128M -s -S -drive file=image/disk.img,index=0,media=disk,format=raw -monitor stdio -no-reboot
run:all
	qemu-system-i386  -smp 1 -m 128M       -drive file=image/disk.img,index=0,media=disk,format=raw -monitor stdio -no-reboot
