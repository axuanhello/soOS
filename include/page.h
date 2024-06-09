#ifndef PAGE_H
#define PAGE_H

/*
 * page.h
 *
 * This file contains definitions and functions for managing page tables
 * on a 32-bit system. Page tables are crucial for virtual memory management,
 * translating virtual addresses to physical addresses, and controlling
 * access permissions.
 *
 * Page Table Entry (PTE) Structure:
 * 
 * 31          ...  12  11     9 8  7 6 5 4 3 2 1 0
 * +-------------+----+----+----+----+----+----+----+
 * |  Address    |AVL |G |D |A |C |W |U |R |P |
 * +-------------+----+----+----+----+----+----+----+
 * 
 * Bit Definitions:
 * - P (Present): Indicates if the page is in physical memory.
 * - R/W (Read/Write): If set, the page is read/write. Otherwise, it is read-only.
 * - U/S (User/Supervisor): Controls access based on privilege level. If set, accessible by all.
 * - PWT (Write-Through): Controls write-through caching. If set, write-through is enabled.
 * - PCD (Cache Disable): If set, the page will not be cached.
 * - A (Accessed): Indicates if the page has been accessed (read during virtual address translation).
 * - D (Dirty): Indicates if the page has been written to.
 * - G (Global): If set, the TLB entry for the page is not invalidated on a MOV to CR3 instruction.
 * - PAT (Page Attribute Table): Used to determine memory caching type, along with PCD and PWT.
 * - AVL (Available): Bits available for OS use.
 * 
 * Note:
 * - 4-MiB pages require PSE to be enabled.
 * - The accessed and dirty bits are managed by the OS, and the processor does not clear them.
 * - When modifying the accessed or dirty bits, invalidate the associated page to ensure the processor updates them correctly.
 * 
 * For more details, refer to the Intel Manuals, specifically Volume 3A.
 */

// Refers to https://wiki.osdev.org/Paging

#include "types.h"

// Bitmask for disabling paging cache
#define PAGE_CACHE_DISABLED  0B00010000

// Bitmask for enabling write-through caching
#define PAGE_WRITE_THROUGH   0B00001000

// Bitmask for enabling access from all privilege levels
#define PAGE_ACCESS_FROM_ALL 0B00000100

// Bitmask for enabling read/write access
#define PAGE_IS_WRITABLE     0B00000010

// Bitmask for indicating page presence in main memory
#define PAGE_IS_PRESENT      0B00000001

#define PAGE_ENTRIES_PER_TABLE 1024
#define PAGE_SIZE 4096


struct page_directory
{
    uint32_t* directory_entry;
    
};

uint32_t* get_page_directory(struct page_directory* paging_directory);
void switch_page(uint32_t* directory);
struct page_directory* create_page_directory(uint8_t flags);

 

int set_paging(uint32_t* directory, void* virtual_addr, uint32_t val);
bool is_page_aligned(void* addr);

// We do not call enable_paging()
// until we have created a paging directory
// and switch to that directory
void enable_paging();
void load_page_directory(uint32_t* directory);

#endif