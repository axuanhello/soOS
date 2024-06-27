#include "gdt.h"
#include "print.h"

void encode_gdt_entry(uint8_t* dest, struct gdt_structure src)
{
    if((src.limit > 65536) && (src.limit & 0xFFF) != 0XFFF) // 
    {
        panic("encode to gdt entry failed!\n");
    }

    dest[6] = 0x40; // set the granularity bit
    if(src.limit > 65536)
    {
        src.limit = src.limit >> 12;
        dest[6] = 0xc0; // 
    }

    // Encode the limit
    dest[0] = (uint8_t)(src.limit & 0xFF);
    dest[1] = (uint8_t)((src.limit >> 8) & 0xFF);
    dest[6] |= (uint8_t)((src.limit >> 16) & 0x0F);

    // Encode the base
    dest[2] = (uint8_t)(src.base & 0xFF);
    dest[3] = (uint8_t)((src.base >> 8) & 0xFF);
    dest[4] = (uint8_t)((src.base >> 16) & 0xFF);
    dest[7] = (uint8_t)((src.base >> 24) & 0xFF);

    // Encode the type
    dest[5] = src.type;
}


void gdt_structure_to_gdt_entry(struct gdt_structure *gdt, struct gdt_entry *entry,int total_entries){
    for(int i = 0; i < total_entries; i ++ ){
        encode_gdt_entry((uint8_t*)&entry[i], gdt[i]); // encode a simpler structured gdt to a normal gdt
    }
}