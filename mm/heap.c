#include "errno.h"
#include "mm.h"
#include "config.h"
#include "string.h"
#include"print.h"
struct heap kernel_heap;
struct heap_table kernel_heap_table;

int heap_create(struct heap* heap, void* start, void* end, struct heap_table* table) {
    int res = 0;
    //检查地址是否对齐
    if ((unsigned int)start % HEAP_BLOCK_SIZE || (unsigned int)end % HEAP_BLOCK_SIZE) {
        res = -EINVARG;
        return res;
    }
    else {
        //设置映射物理起始地址
        memset(heap, 0, sizeof(struct heap));
        heap->saddr = start;
        heap->table = table;
    }
    //检查start-end区间是否等同heap表项所能映射的数量
    if (table->total * HEAP_BLOCK_SIZE != (size_t)(end - start)){
        res = -EINVARG;
        return res;
    }  
    //将每个表项置为free状态
    size_t table_size = sizeof(heap_table_entry) * table->total;
    memset(table->entries, HEAP_FREE, table_size);
    return res;
}

void kheap_init() {
    kernel_heap_table.entries = (heap_table_entry*)(HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE;

    void* end = (void*)(HEAP_ADDRESS + HEAP_SIZE_BYTES);
    int res = heap_create(&kernel_heap, (void*)(HEAP_ADDRESS), end, &kernel_heap_table);
    if (res < 0) {
        print("Failed to create heap\n");
        while (1);
    }
}


void kfree(void* ptr) {
    heap_free(&kernel_heap, ptr);
}

static inline int isfree(heap_table_entry entry) {
    return (entry & 0x0f)==HEAP_FREE;
}

int get_free_entry(struct heap* heap, uint32_t total_blocks) {
    struct heap_table* table = heap->table;
    int bc = 0;
    int bs = -1;
    for (size_t i = 0; i < table->total; ++i) {
        if (!isfree(table->entries[i])) {
            //print("Index ");
            //put_int(i);
            //print(" is not free.\n");
            bc = 0;
            bs = -1;
            continue;
        }
        // 如果找到第一块空闲，设置其为开始的块数
        if (bs == -1) {
            bs = i;
        }
        //计数
        ++bc;
        if (bc == total_blocks) {
            break;
        }
    }
    if (bs == -1) {
        return -ENOMEM;
    }
    return bs;
}

static void set_blocks_taken(struct heap* heap, int start_block, int total_blocks) {
    int end_block = (start_block + total_blocks) - 1;
    //设置第一个
    heap_table_entry entry = HEAP_TAKEN /*| HEAP_BLOCK_IS_FIRST*/;
    //确定是否需要设置连续区间
    if (total_blocks > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }
    heap->table->entries[start_block] = entry;

    //设置第二个到倒数第二个
    for (int i = start_block+1; i < end_block; ++i) {
        heap->table->entries[i] = entry;       
    }
    //设置最后一项，（如果不是区间的话，也可能是第一项），HAS_NEXT置空
    heap->table->entries[end_block] = HEAP_TAKEN;
}

void* heap_malloc_blocks(struct heap* heap, uint32_t num_blocks) {
    int index = get_free_entry(heap, num_blocks);
    if (index< 0) {
        return (void*)0;
    }
    //print("entry index:");
    //put_int(index);
    //print("\n");
    set_blocks_taken(heap, index, num_blocks);
    //返回物理地址
    return heap->saddr + (index * HEAP_BLOCK_SIZE);
}

//只实现了按页分配
void* heap_malloc(struct heap* heap, size_t size) {
    //漏了取非！BUG
    if (!size % HEAP_BLOCK_SIZE) {
        return heap_malloc_blocks(heap, size/HEAP_BLOCK_SIZE);
    }
    else {
        return heap_malloc_blocks(heap, size / HEAP_BLOCK_SIZE + 1);
    }
}
void* kmalloc(size_t size) {
    return heap_malloc(&kernel_heap, size);
}

static inline int heap_address_to_block(struct heap* heap, void* address) {
    return ((int)(address - heap->saddr)) / HEAP_BLOCK_SIZE;
}

void set_blocks_free(struct heap* heap, size_t start_block) {
    struct heap_table* table = heap->table;
    for (size_t i = start_block; i < table->total; ++i) {
        heap_table_entry entry = table->entries[i];
        table->entries[i] = HEAP_FREE;
        //print("Index ");
        //put_int(i);
        //print(" is set to free.\n");
        //直到最后一个(entry没有HEAP_BLOCK_HAS_NEXT标记)
        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

//回收也按页回收
void heap_free(struct heap* heap, void* ptr) {
    if (ptr < heap->saddr) {
        print("Bad free!");
        while (1);
    }
    set_blocks_free(heap, heap_address_to_block(heap, ptr));
}