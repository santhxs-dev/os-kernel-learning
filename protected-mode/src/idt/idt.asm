section .asm

global idt_load
idt_load:
    push ebp         ; save ebp
    mov ebp, esp     ; ebp now point to the top of the stack

    mov ebx, [ebp+8] ; first function param
    lidt [ebx]
    pop ebp          ; restore ebp
    ret
