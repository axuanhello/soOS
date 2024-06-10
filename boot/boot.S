#include "config.h"    
    .global _start             
    .text
fat16_header:
    .code16
    # Byte Offset: 0x000
    # Length: 3 bytes
    # Opstring: 0XEB(jmp short) 0Xxx(start) 0X90(nop)
    jmp _start
    nop

    # BIOS Parameter Block
    # Byte Offset: 0x003
    .ascii "soOSv1.0"           # OEM名称
    .word 512                   # 每扇区字节数
    .byte 128                   # 每个簇的扇区数
    .word 200                   # 保留扇区数
    .byte 2                     # FAT表数
    .word 64                    # 根目录项数
    .word 0x8000                # 扇区数
    .byte 0xf8                  # 媒体描述符
    .word 0x100                 # 每FAT扇区数
    .word 0x20                  # 每道扇区数
    .word 16                    # 磁头数
    .long 0                     # 隐藏扇区数
    .long 0x773594              # 逻辑扇区总数

    # Extended BIOS Parameter Block
    # Byte Offset: 0x024
    .byte 0x80                  # 驱动器号
    .byte 0                     # 保留(WINNT BIT)
    .byte 0x29                  # 扩展引导标志
    .long 0xD105                # 卷序列号
    .ascii "SOOS BOOTIN"        # 卷标
    .ascii "FAT16   "           # 文件系统类型

_start:
    xor %eax,%eax
    xor %ebx,%ebx
    xor %ecx,%ecx
    xor %edx,%edx
    xor %ebp,%ebp
    xor %esi,%esi
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%ss
    mov $0x7c00,%sp
    mov %sp,%bp


start_enter_32:    
    #关中断，32位中断表尚未建立
    cli

    #打开A20地址线，Fast A20 Gate法
    inb $0x92,%al
    orb $0x2,%al
    outb %al, $0x92

    #加载GDT
    lgdt gdt_ptr

    #置cr0,PE位为1，进入保护模式
    movl %cr0,%eax
    orl $0x1,%eax
    movl %eax,%cr0
    /*
    * Segment Selector:
    * 15-3  Index
    * 2     TI table Idicator 0=GDT,1=LDT
    * 1-0   RPL requested privilege level
    */
    ljmp $0x8, $start32 #通过jmp设置cs选择子，使用jmp可清空流水线，刷新gdt缓存
     
start32:
    .code32
    movw $0x10,%ax
    movw %ax,%ds    #设置数据段选择子为2号gdt项
    movw %ax,%es
    movw %ax,%ss
    movw %ax,%fs
    movw %ax,%gs
    movl $0x200000,%esp #设置栈顶 
    movl %esp,%ebp

read_kernel:    
    #接下来需要将操作系统剩余部分代码从硬盘转到内存，并跳转执行
    movl $BOOT_END_SECTOR, %eax     #起始扇区。LBA从0开始编号，CHS从1开始编号
    movl $KERNEL_START_PADDR,%edi   #内核代码存放地址
    movb $255,%cl                   #读取扇区数，目前内核应不会超过255*512B约128KB。
    call read_disk

    ljmp $0x8,$KERNEL_START_PADDR
    hlt

#读取硬盘到内存。参数：eax起始扇区，LBA28定址；cl读取扇区数；es:edi，存放内存位置
read_disk:
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %edi
    #LBA方式从IO端口读取硬盘数据到内存
    #端口0x1f3，设置LBA0~7位
    movw $0x1f3,%dx
    out %al,%dx
    #端口0x1f4，设置LBA8~15位
    movw $0x1f4,%dx
    shr $8,%eax
    out %al,%dx
    #端口0x1f5，设置LBA16~23位
    movw $0x1f5,%dx
    shr $8,%eax
    out %al,%dx

    #端口0x1f6，设置device寄存器，选择LBA寻址模式（6位置1），选择主盘（4位置0），0-3位置LBA地址27~24位，5、7位默认为1
    movw $0x1f6,%dx
    shr $8,%eax
    #4位置0
    and $0xf,%al
    #5,6,7位置1
    or $0xe0,%al 
    out %al,%dx
    
    #端口0x1f2，设置读取扇区数
    movw $0x1f2,%dx
    movb %cl,%al
    out %al,%dx

    #端口0x1f7，设置command读取命令
    movw $0x1f7,%dx
    movb $0x20,%al   #command=0x20,read;0x30,write;0xec,identify
    out %al,%dx
next_sector:
    push %ecx

    #轮询方式等待IO数据
wait_for_disk:    
    #端口0x1f7，读取status  !重新赋值dx!
    movw $0x1f7,%dx
    in %dx,%al
    and $0x89,%al  #只看第0,3,7位。0位错误，3位完成，6位就绪，7位忙
    cmp $0x08,%al
    #nop
    jne wait_for_disk

    #端口0x1f0，每次读取2字节数据，一个扇区512字节，循环256次。
 
read_port_data:
    mov $256,%ecx
    movw $0x1f0,%dx
    rep insw
    
    pop %ecx
    loop next_sector
    
    popl %edi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax

    ret


.p2align 2  #内存对齐到2^2字节处

gdt:
    /*
    * 高地址到低地址依次为：
    * base 31_24    (8bit)
    
    * G             Granualrity
    * D/B           Default operation size
    * L             64-bit code seg
    * AVL           Availabel for use by system software
    
    * Seg Limit 19_16(4bit)
    
    * P             Segment present
    * DPL(2bit)     Descriptor privilege level
    * S             Descriptor type(0=system;1=code or data)
    * Type(4bit)    Segment type
    
    * Base 23_16    (8bit)
    
    * Base 15_0
    * Seg Limit 15_0
    */
NULL_GDT:
    .quad 0x0
CODE_DESC:
    #小端，高地址到低地址
    #Base=0,G=1,D/B=1,SegLim=0xfffff,P=1,DPL=0,S=1,Type=8(execute only)
    .quad 0x00cf98000000ffff
DATA_DESC:
    #Type=2(Read/Write)
    .quad 0x00cf92000000ffff
#STACK_DESC:#可以直接用data的描述符
#   .quad 0x004
GDT_LIMITS:

gdt_ptr:
    .word GDT_LIMITS -gdt-1  #低地址gdt表长度（字节）-1
    .long gdt       #高地址gdt基地址位置

endcode:
.org 510  #0x1fe
.byte 0x55,0xaa