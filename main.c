#include "io.h"
#include "string.h"
#include "print.h"
#include "mm.h"
#include "page.h"
#include "disk.h"
#include "vfs.h"
#include "config.h"
#include "gdt.h"
#include "tss.h"

void idt_init();

static struct page_directory *kernel_dir = 0;

struct tss tss;
struct gdt_entry real_gdt[TOTAL_GDT_SEGMENTS];

// Reference: https://wiki.osdev.org/Global_Descriptor_Table 
struct gdt_structure structured_gdt[TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00}, // null segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a}, // kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92}, // kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8}, // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2}, // User data segment
    {.base = (uint32_t)&tss, .limit = sizeof(tss), .type=0xe9}, // TSS segment 
};


void main(void)
{
    clear_screen();
    set_cursor(0);

    memset(real_gdt, 0x00, sizeof(real_gdt));
    gdt_structure_to_gdt_entry(structured_gdt, real_gdt, TOTAL_GDT_SEGMENTS);

    // Load gdt
    load_gdt(real_gdt, sizeof(real_gdt));

    kheap_init();

    init_fs();

    search_and_init_disk();

    idt_init();

    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = DATA_SELECTOR;

    // Index(13bit): 5(在GDT表中下标为5的条目是tss段描述符)
    // RPL(2bit): 0
    // TI(1bit): 0
    // Selector(16bit): 5 << 3 + 0 = 0x28
    load_tss(0x28); 
    
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

    panic("Kernel panic\n");
    for (;;)
    ;
}