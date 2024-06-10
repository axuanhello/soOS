echo "Executing gdbinit\n"
echo targe remot localhost:1234\n
target remote localhost:1234
set architecture i8086
b *0x7c00
c
add-symbol-file build/kernel.elf

b main.c:64
set architecture i386
c