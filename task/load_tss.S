.section .text

.global load_tss

load_tss:
    push %ebp
    mov %esp, %ebp
    mov 8(%ebp), %ax         # mov ax, [ebp+8]
    ltr %ax
    pop %ebp
    ret