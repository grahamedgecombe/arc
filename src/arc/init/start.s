;
;  Copyright (c) 2011 Graham Edgecombe <graham@grahamedgecombe.com>
;
;  Permission to use, copy, modify, and/or distribute this software for any
;  purpose with or without fee is hereby granted, provided that the above
;  copyright notice and this permission notice appear in all copies.
;
;  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
;  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
;  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
;  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
;  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
;  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
;  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;

[bits 32]
[extern init]
[extern _start]
[extern _end]

; higher-half virtual memory address
KERNEL_VMA equ 0xFFFF800000000000

; MSR numbers
MSR_EFER equ 0xC0000080

; EFER bitmasks
EFER_LM equ 0x100
EFER_NX equ 0x800

; CR0 bitmasks
CR0_PAGING equ 0x80000000

; CR4 bitmasks
CR4_PAE equ 0x20
CR4_PSE equ 0x10

; page flag bitmasks
PG_PRESENT  equ 0x1
PG_WRITABLE equ 0x2
PG_USER     equ 0x4
PG_BIG      equ 0x80
PG_NO_EXEC  equ 0x8000000000000000

; page and table size constants
LOG_TABLE_SIZE equ 9
LOG_PAGE_SIZE  equ 12
PAGE_SIZE  equ (1 << LOG_PAGE_SIZE)
TABLE_SIZE equ (1 << LOG_TABLE_SIZE)

; bootstrap stack size and alignment
STACK_SIZE  equ 8192
STACK_ALIGN equ 16

; multiboot flag bitmasks
MB_FLAGS_ALIGN equ 0x1
MB_FLAGS_MMAP  equ 0x2
MB_FLAGS_VIDEO equ 0x4
MB_FLAGS_AOUT  equ 0x8000

; multiboot constants
MB_ALIGN    equ 8
MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ (MB_FLAGS_ALIGN | MB_FLAGS_MMAP)
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

; includes initialization code and lower-half data
[section .init.lower align=MB_ALIGN]
; the multiboot header
dd MB_MAGIC
dd MB_FLAGS
dd MB_CHECKSUM

; paging structures
align PAGE_SIZE
boot_pml4:
  dq (boot_pml3 + PG_PRESENT + PG_WRITABLE)
  times (TABLE_SIZE / 2 - 1) dq 0
  dq (boot_pml3 + PG_PRESENT + PG_WRITABLE)
  times (TABLE_SIZE / 2 - 2) dq 0
  dq (boot_pml4 + PG_PRESENT + PG_WRITABLE + PG_NO_EXEC)

boot_pml3:
  dq (boot_pml2 + PG_PRESENT + PG_WRITABLE)
  times (TABLE_SIZE - 1) dq 0

boot_pml2:
  dq (0x0 + PG_PRESENT + PG_WRITABLE + PG_BIG)
  times (TABLE_SIZE - 1) dq 0

; the global descriptor table
gdt:
  .null:
    dq 0
  .code:
    dw 0xFFFF
    dw 0
    dw 0x9800
    dw 0x00AF
  .data:
    dw 0xFFFF
    dw 0
    dw 0x9200
    dw 0x00CF
gdtr:
  dw $ - gdt - 1
  dq gdt

; the entry point of the kernel executable
[global start]
start:
  ; push the info GRUB passes us as we trash the EAX register
  push dword 0
  push eax
  push dword 0
  push ebx

  ; enable PAE and PSE
  mov eax, cr4
  or eax, (CR4_PAE + CR4_PSE)
  mov cr4, eax

  ; enable long mode and the NX bit
  mov ecx, MSR_EFER
  rdmsr
  or eax, (EFER_LM + EFER_NX)
  wrmsr

  ; set cr3 to a pointer to pml4
  mov eax, boot_pml4
  mov cr3, eax

  ; enable paging
  mov eax, cr0
  or eax, CR0_PAGING
  mov cr0, eax

  ; leave compatibility mode
  lgdt [gdtr]
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  jmp 0x08:.trampoline

; some 64-bit code in the lower half used to jump to the higher half
[bits 64]
.trampoline:
  ; enter the higher half
  mov rax, qword .next
  jmp rax

; the higher-half code
[section .init.higher]
.next:
  ; re-load the GDTR with a virtual base address
  mov rax, [gdtr + 2]
  mov rbx, qword KERNEL_VMA
  add rax, rbx
  mov [gdtr + 2], rax
  mov rax, qword gdtr + KERNEL_VMA
  lgdt [rax]

  ; map the rest of the kernel into virtual memory
  mov rax, qword _start - KERNEL_VMA      ; first page number
  shr rax, LOG_PAGE_SIZE + LOG_TABLE_SIZE
  mov rbx, qword _end - KERNEL_VMA        ; last page number
  shr rbx, LOG_PAGE_SIZE + LOG_TABLE_SIZE
  mov rcx, qword boot_pml2 + KERNEL_VMA   ; pointer into pml2 table
  .map_page:
    ; calculate the value of the page table entry
    mov rdx, rax
    shl rdx, LOG_PAGE_SIZE + LOG_TABLE_SIZE
    mov rsi, rdx
    mov rdi, qword KERNEL_VMA
    add rsi, rdi
    or rdx, PG_PRESENT + PG_WRITABLE + PG_BIG

    ; write the page table entry
    mov [rcx], rdx
    invlpg [rsi]

    ; increment pml2 pointer
    add rcx, 8

    ; check if we should terminate the loop
    cmp rax, rbx
    je .map_page_end

    ; increment the counter and map the next page
    inc rax
    jmp .map_page
  .map_page_end:

  ; pop the info GRUB passed us
  pop rsi
  pop rdi

  ; set up the new stack (GRUB's is in lower memory)
  mov rbp, 0
  mov rsp, qword stack + STACK_SIZE

  ; unmap the identity-mapped memory
  mov qword [boot_pml4], 0x0

  ; invalidate the TLB cache for the identity-mapped memory
  invlpg [0x0]

  ; clear the RFLAGS register
  push qword 0x0
  popf

  ; call the kernel
  ; - the arguments were popped into RDI and RSI above
  ; - the DF has been reset by the code above - no CLD is required
  call init

  ; disable interrupts (when we return here the kernel might have enabled them)
  cli 

  ; hang the CPU (looped in case an NMI occurs)
  .hang:
    hlt
    jmp .hang

; memory reserved for the kernel's stack
[section .bss align=STACK_ALIGN]
stack:
  resb STACK_SIZE

