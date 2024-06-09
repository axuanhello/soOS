#include"io.h"
#include"string.h"
#include"print.h"
#include "mm.h"
#include "page.h"
#include "vfs.h"

void idt_init();

static struct page_directory* kernel_dir = 0;

void main(void) {
    clear_screen();
    set_cursor(0);
    kheap_init();

    idt_init();
    // int a = 1;
    // //int b=a / 0;
    // void* p = kmalloc(1000);
    // put_uint(p);
    // print("\n");
    // void* q = kmalloc(100);
    // put_uint(q);
    // print("\n");

    // kfree(p);

    // void* pp = kmalloc(500);
    // put_uint(pp);
    // print("\n");
    // void* ppp = kmalloc(500);
    // put_uint(ppp);
    // print("\n");
    // print("hello world!");
    kernel_dir = create_page_directory(PAGE_IS_WRITABLE | PAGE_IS_PRESENT | PAGE_ACCESS_FROM_ALL);

    switch_page(get_page_directory(kernel_dir));

    enable_paging();

    char* ptr = kmalloc(4096); 
    set_paging(get_page_directory(kernel_dir), (void*)0x1000, (uint32_t)ptr | PAGE_ACCESS_FROM_ALL | PAGE_IS_PRESENT | PAGE_IS_WRITABLE);

    char* ptr2 = (char*)0x1000;
    *ptr2 = 'a';
    ptr2[1] = 'b';
    ptr2[2] = 'c';

    print(ptr2);
    print(ptr);

    int fd = fopen("0:/hello.txt", "r");
    if(fd)
    {
        print("file opened\n");
    } else {
        print("file not opened\n");
    }
    for(;;);
}