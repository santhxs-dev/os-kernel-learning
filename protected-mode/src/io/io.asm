section .asm

global insb
global insws
global outb
global outw

insb:
    push ebp
    mov ebp, esp

    xor eax, eax     ; set to zero
    mov edx, [ebp+8] ; port
    in al, dx        ; byte

    pop ebp
    ret              ; eax is the ret value

insw:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov edx, [ebp+8]
    in ax, dx        ; word

    pop ebp
    ret

outb:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, al

    pop ebp
    ret

outw:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, ax

    pop ebp
    ret
