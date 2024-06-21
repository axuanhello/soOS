#include "io.h"
#include "string.h"
#include "print.h"
#include "mm.h"
#include "page.h"
#include "disk.h"
#include "vfs.h"

void idt_init();

static struct page_directory *kernel_dir = 0;

void main(void)
{
    clear_screen();
    set_cursor(0);
    kheap_init();

    init_fs();

    search_and_init_disk();

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

    char *ptr = kmalloc(4096);
    set_paging(get_page_directory(kernel_dir), (void *)0x1000, (uint32_t)ptr | PAGE_ACCESS_FROM_ALL | PAGE_IS_PRESENT | PAGE_IS_WRITABLE);

    char *ptr2 = (char *)0x1000;
    *ptr2 = 'a';
    ptr2[1] = 'b';
    ptr2[2] = 'c';

    print(ptr2);
    print(ptr);

    int fd = fopen("0:/hello.txt", "r");
    if(fd)
    {
        // print(fd +'0');
        print("file opened\n");
        char buf[8];
        fread(buf, 7, 1, fd);
        buf[7] = 0x00;
        print(buf);
    } else if (fd == -1){
        print("file not opened\n");
    }

    // struct disk_stream *stream = create_disk_stream(0);
    // seek_disk_stream(stream, 0x21c);
    // unsigned char c=0;
    
    // read_disk_stream(stream, 1, &c);
    // print(c);

    for (;;)
    ;
}