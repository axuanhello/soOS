#include"mmu.h"
#include"types.h"
#include"io.h"
#include"print.h"
#include "int.h"

struct gatedesc idt[256];
extern void (*int_entry_addr[])(void);

extern void sendeoi();

void do_timer() {

    enable_int();

}
void int_dispatch(struct intr_stack* pintr_stack) {
    //外部中断
    if (pintr_stack->int_no >= 32) {
        switch (pintr_stack->int_no) {
        case 32:
            //put_str("Timer Interrupt. ");
            sendeoi();
            do_timer();
            break;
        case 33:
            //do_keyboard();
            sendeoi();
            break;
        default:
            sendeoi();
            break;
        }
        return;
    }
    //内中断
    else {
        set_cursor(0);
        put_str("Interrupt! No: ");
        put_uint(pintr_stack->int_no);
        put_str("  \nError code:");
        put_uinth(pintr_stack->error_code);
        for (;;) {
            asm("hlt");
        }
    }
    return;
}

static void timer_init(int frequency) {
    //控制字，设置计数器编号，读写锁存器，模式
    outb(0x43, 0<<6|3 << 4 | 2 << 1);
    uint16_t count = 1193180 / frequency;
    //计数值低8位
    outb(0x40, (uint8_t)count);
    //高8位
    outb(0x40, (uint8_t)(count >> 8));
}

void idt_init() {
    //pic_init();
    timer_init(50);//设置频率50Hz
    for (int i = 0;i < sizeof(idt) / sizeof(idt[0]);++i) {
        SETGATE(idt[i], 0, 8, int_entry_addr[i], 0);
    }
    //uint64_t idt_ptr = (uint64_t)(&idt) << 16 + sizeof idt - 1;//BUG!!!指针竟默认当成有符号数！（应该是编译后就是立即数，默认int了）当直接扩展位表示时，遵循符号扩展而不是零扩展！必须先强转为无符号再扩展！
    uint64_t idt_ptr=((uint64_t)((uint32_t)(&idt))<<16)+sizeof idt - 1;

    asm volatile("lidt %0"::"m"(idt_ptr));
    //允许中断
    asm volatile("sti");

}