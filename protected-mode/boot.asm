ORG 0x7c00
BITS 16

_start:
  jmp short start
  nop

  times 33 db 0

start:
  jmp 0:step2

step2:
  cli ; clear interrupts
  mov ax, 0x00
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov sp, 0x7c00
  sti ; enables interrupts
  
  jmp $

; GDT
gdt_start:
gdt_null:
  dd 0x0
  dd 0x0

; offset 0x8
gdt_code:       ; CS should point to this
  dw 0xffff     ; segment limits first 0-15 bits
  dw 0          ; base first 0-15 bits
  db 0          ; base 16-23 bits
  db 0x9a       ; access byte
  db 11001111b  ; high 4 bits flags and the low 4 bits flags
  db 0          ; base 24-31 bits

; offset 0x10
gdt_data:       ; DS, SS, ES, FS, GS
  dw 0xffff     ; segment limits first 0-15 bits
  dw 0          ; base first 0-15 bits
  db 0          ; base 16-23 bits
  db 0x92       ; access byte
  db 11001111b  ; high 4 bits flags and the low 4 bits flags
  db 0          ; base 24-31 bits

gdt_end:

gdt_descriptor:
  dw gdt_end - gdt_start-1
  dd gdt_start

print:
  mov bx, 0
.loop:
  lodsb
  cmp al, 0
  je .done
  call print_char
  jmp .loop
.done:
  ret

print_char:
  mov ah, 0eh
  int 0x10
  ret

times 510-($ - $$) db 0
dw 0xAA55