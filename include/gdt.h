#ifndef GDT_H_  
#define GDT_H_
#include "types.h"

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_structure
{
    uint32_t limit;
    uint32_t base;
    uint8_t type;
} __attribute__((packed));

void load_gdt(struct gdt_entry *gdt, uint16_t size);
void gdt_structure_to_gdt_entry(struct gdt_structure *gdt, struct gdt_entry *entry,int total_entries);
#endif