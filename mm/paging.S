.code32

.global load_page_directory
.global enable_paging

.text

load_page_directory:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax # In X86, the first argument is at [ebp + 8]
    movl %eax, %cr3 # CR3 = page directory base address
    popl %ebp # Restore the caller's original base pointer ebp
    ret

enable_paging:
    pushl %ebp
    movl %esp, %ebp
    movl %cr0, %eax
    orl $0x80000000, %eax # Set the PG bit in CR0
    movl %eax, %cr0
    popl %ebp
    ret