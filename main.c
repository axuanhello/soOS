#include"io.h"
#include"string.h"
#include"print.h"
#include"mm.h"
void idt_init();
void main(void) {
    clear_screen();
    set_cursor(0);
    kheap_init();

    idt_init();
    int a = 1;
    //int b=a / 0;
    void* p = kmalloc(1000);
    put_uint(p);
    print("\n");
    void* q = kmalloc(100);
    put_uint(q);
    print("\n");

    kfree(p);

    void* pp = kmalloc(500);
    put_uint(pp);
    print("\n");
    void* ppp = kmalloc(500);
    put_uint(ppp);
    print("\n");
    print("hello world!");
    for(;;);
}