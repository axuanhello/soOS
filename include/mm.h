#ifndef __MM_H
#define __MM_H
#include"types.h"
#define HEAP_TAKEN 0x01
#define HEAP_FREE 0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
//#define HEAP_BLOCK_IS_FIRST  0b01000000

typedef unsigned char heap_table_entry;

struct heap_table{
    heap_table_entry* entries;
    //entry数量
    size_t total;
};

struct heap{
    struct heap_table* table;
    // 堆分配的起始地址
    void* saddr;
};

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);
void* heap_malloc(struct heap* heap, size_t size);
void heap_free(struct heap* heap, void* ptr);
void kheap_init();
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif