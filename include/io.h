#ifndef __IO_H
#define __IO_H
#include"types.h"

//向端口port输出一字节数据data
static inline void __attribute__((always_inline)) outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %b0,%1"
        ::"a"(data),"id"(port)
        );
}

// //向端口port输出两字节数据data
static inline void __attribute__((always_inline)) outw(uint16_t port, uint16_t data) {
    asm volatile ("outw %w0,%1"
        ::"a"(data),"id"(port)
        );
}

//将%ds:addr内存处word_count个字写入端口port
static inline void __attribute__((always_inline)) outsw(uint16_t port, void* addr, uint32_t word_count) {
    asm volatile("cld;\n\t""rep outsw;\n\t"
        :"+S"(addr), "+c"(word_count)//"+"必须写在input处，表示又读又写
        :"d"(port));
}

//从端口port读取一字节数据
static inline uint8_t __attribute__((always_inline)) inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1,%b0"
        :"=a"(data)
        :"id"(port));
    return data;
}

//从端口port读取两字节数据
static inline uint16_t __attribute__((always_inline)) inw(uint16_t port) {
    uint16_t data;
    asm volatile ("inw %1,%w0"
        :"=a"(data)
        :"id"(port));
    return data;
}

// //从端口port读word_count个字写入%es:addr内存处
// static inline void __attribute__((always_inline)) insw(uint16_t port, void* addr, uint32_t word_count) {
//     asm volatile ("cld;\n\t""rep insw"
//         :"+D"(addr), "+c"(word_count):"d"(port)
//         :"memory");
// }

// unsigned char inb(unsigned short port);
// unsigned short inw(unsigned short port);

// void outb(unsigned short port, unsigned char val);
// void outw(unsigned short port, unsigned short val);

// /**
//  * @brief Reads a byte from the specified I/O port.
//  *
//  * @param port The I/O port to read from.
//  * @return The byte read from the specified I/O port.
//  */
// uint8_t inb(uint16_t port) {
//     uint8_t value;
//     __asm__ __volatile__ ("inb %1, %0" : "=a" (value) : "dN" (port));
//     return value;
// }

// /**
//  * @brief Reads a word (2 bytes) from the specified I/O port.
//  *
//  * @param port The I/O port to read from.
//  * @return The word read from the specified I/O port.
//  */
// uint16_t inw(uint16_t port) {
//     uint16_t value;
//     __asm__ __volatile__ ("inw %1, %0" : "=a" (value) : "dN" (port));
//     return value;
// }

// /**
//  * @brief Writes a byte to the specified I/O port.
//  *
//  * @param port The I/O port to write to.
//  * @param value The byte to write to the specified I/O port.
//  */
// void outb(uint16_t port, uint8_t value) {
//     __asm__ __volatile__ ("outb %0, %1" : : "a" (value), "dN" (port));
// }

// /**
//  * @brief Writes a word (2 bytes) to the specified I/O port.
//  *
//  * @param port The I/O port to write to.
//  * @param value The word to write to the specified I/O port.
//  */
// void outw(uint16_t port, uint16_t value) {
//     __asm__ __volatile__ ("outw %0, %1" : : "a" (value), "dN" (port));
// }

#endif