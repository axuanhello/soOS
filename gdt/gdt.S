.global load_gdt

.section .text
# Load the GDT
load_gdt:
    movl 4(%esp), %eax          # mov eax, [esp+4]
    movl %eax, gdt_desc+2       # mov [gdt_desc+2], eax
    movw 8(%esp), %ax           # mov ax, [esp+8]
    movw %ax, gdt_desc          # mov [gdt_desc], ax
    lgdt gdt_desc               # lgdt [gdt_desc]
    ret

.section .data
gdt_desc:
    .word 0                     # Length
    .long 0                     # Base