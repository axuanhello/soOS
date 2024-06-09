
#include"string.h"
#include "io.h"
#include "desc.h"
#include "print.h"

struct gatedesc idt[256];

void int21h();
void ignore_int();

void int21h_handler() {
    print("Keyboard pressed!\n");
    outb(0xa0, 0x20);
    outb(0x20, 0x20);
}

void ignore_int_handler() {
    outb(0xa0, 0x20);
    outb(0x20, 0x20);
}

void idt_init() {
    memset(idt, 0, sizeof(idt));
    for (int i = 0; i < sizeof(idt)/sizeof(idt[0]); ++i) {
        set_int(idt[i], 0x8,ignore_int,3);
    }

    set_int(idt[0x21], 0x8,int21h,0);

    // 设置idt_ptr
    uint64_t idt_ptr=((uint64_t)((uint32_t)(&idt))<<16)+sizeof idt - 1;
    asm volatile("lidt %0"::"m"(idt_ptr));
    //开启中断
    asm volatile("sti");
}
