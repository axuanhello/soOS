    .global _start
_start:
    //主片
    //写入ICW1，初始化连接方式：级联，中断信号触发方式：边沿触发，需要ICW4。0001,LTIM,ADI,SNGL,IC4
    mov $0x11,%al
    outb %al,$0x20
    //outb(0x20, 0x11);
    
    //写入ICW2，设置起始中断向量号。T7-T3,ID2-0
    mov $0x20,%al
    outb %al,$0x21
    //outb(0x21, 0x20);

    //写入ICW3，设置级联接口为S2。S7-S0
    mov $0x4,%al
    outb %al,$0x21
    //outb(0x21, 0x04);

    //写入ICW4，x86必须写入。置EOI=0，需手动响应中断。000,SFNM,BUF,M/S(BUF=1有效),AEOI,uPM(x86=1)
    mov $0x1,%al
    outb %al,$0x21
    //outb(0x21, 0x01);

    //从片
    mov $0x11,%al
    outb %al,$0xa0
    //outb(0xa0, 0x11);

    mov $0x28,%al
    outb %al,$0xa1
    //outb(0xa1, 0x28);//中断开始0x28
    
    mov $0x2,%al
    outb %al,$0xa1
    //outb(0xa1, 0x02);//设置级联主片口为2号
    
    mov $0x1,%al
    outb %al,$0xa1
    //outb(0xa1, 0x01);//EOI
    call main
    jmp .