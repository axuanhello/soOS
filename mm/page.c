#include "mm.h"
#include "page.h"
#include "errno.h"

static uint32_t* current_directory = 0; // ! Must be static in case of multiple paging directories


struct page_directory* create_page_directory(uint8_t flags)
{
    uint32_t* directory = kmalloc(sizeof(uint32_t) * PAGE_ENTRIES_PER_TABLE);
    int offset = 0;

    for(int i = 0; i < PAGE_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* page_table_entry = kmalloc(sizeof(uint32_t) * PAGE_ENTRIES_PER_TABLE);
        for  (int j = 0; j < PAGE_ENTRIES_PER_TABLE; j++)
        {
            page_table_entry[j] = (offset + PAGE_SIZE * j) | flags;
        }

        offset += PAGE_SIZE * PAGE_ENTRIES_PER_TABLE; // switch to next page table
        directory[i] = (uint32_t)page_table_entry | flags | PAGE_IS_WRITABLE; // set the whole pagetable directory writable
    }

    struct page_directory* paging_directory = kmalloc(sizeof(struct page_directory));

    paging_directory->directory_entry = directory;
    return paging_directory;
}

void switch_page(uint32_t* directory)
{
    load_page_directory(directory);
    current_directory = directory;
}

uint32_t* get_page_directory(struct page_directory* paging_directory)
{
    return paging_directory->directory_entry;
}

bool is_page_aligned(void* addr)
{
    return ((uint32_t)addr % PAGE_SIZE) == 0;
}

int get_page_index(void* virtual_addr, uint32_t* dir_index, uint32_t* table_index)
{
    int res = 0;
    if(!is_page_aligned(virtual_addr))
    {
        res = -EINVARG;
        goto out;
    }

    // TODO: Use right shift instead of division
    *dir_index = ((uint32_t)virtual_addr / (PAGE_SIZE * PAGE_ENTRIES_PER_TABLE));
    *table_index = ((uint32_t)virtual_addr / PAGE_SIZE) % PAGE_ENTRIES_PER_TABLE;
out:
    return res;
}

int set_paging(uint32_t* directory, void* virtual_addr, uint32_t physical_addr)
{
    if(!is_page_aligned(virtual_addr))
    {
        return -EINVARG;
    }

    uint32_t dir_index = 0;
    uint32_t table_index = 0;

    int res = get_page_index(virtual_addr, &dir_index, &table_index);
    if(res < 0)
    {
        return res;
    }

    uint32_t entry = directory[dir_index];
    uint32_t* table = (uint32_t*)(entry & 0xFFFFF000); // Only get the table address

    table[table_index] = physical_addr; // physical_addr is the physical address with flags set
    return res;
}
