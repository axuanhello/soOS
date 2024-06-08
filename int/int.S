    .global int21h,  ignore_int
int21h:
    cli
    pushal
    call int21h_handler
    popal
    sti
    iret

ignore_int:
    cli
    pushal
    call ignore_int_handler
    popal
    sti
    iret
