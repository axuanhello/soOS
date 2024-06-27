#ifndef __CONFIG_H
#define __CONFIG_H

#define BOOT_END_SECTOR 0x1
#define KERNEL_START_PADDR 0x100000

// 100MB heap size
#define HEAP_SIZE_BYTES (100*1024*1024)
#define HEAP_BLOCK_SIZE 4096
//堆分配的起始地址
#define HEAP_ADDRESS 0x01000000 
//堆表存放位置
#define HEAP_TABLE_ADDRESS 0x00007E00

#define SECTOR_SIZE 512

#define MAX_FILESYSTEMS 10
#define MAX_FILE_DESCRIPTORS 512

#define TOTAL_GDT_SEGMENTS 6
#define DATA_SELECTOR 0X10
#define CODE_SELECTOR 0X08

#endif