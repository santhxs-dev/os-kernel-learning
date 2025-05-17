section .asm


extern int21h_handler
extern no_interrupt_handler

global int21h
global idt_load
global no_interrupt

idt_load:
    push ebp         ; save ebp
    mov ebp, esp     ; ebp now point to the top of the stack

    mov ebx, [ebp+8] ; first function param
    lidt [ebx]
    pop ebp          ; restore ebp
    ret

int21h:
    cli
    pushad
    call int21h_handler
    popad
    sti
    iret

no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret