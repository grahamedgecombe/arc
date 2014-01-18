;
;  Copyright (c) 2011-2014 Graham Edgecombe <graham@grahamedgecombe.com>
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
KERNEL_VMA equ 0xFFFFFFFF80000000

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
MB_ALIGN          equ 8
MB_MAGIC          equ 0xE85250D6
MB_ARCH_I386      equ 0
MB_TAG_TERMINATOR equ 0
MB_TAG_INFO       equ 1
MB_TAG_ALIGN      equ 6
MB_INFO_MODULES   equ 3
MB_INFO_MMAP      equ 6

; includes initialization code and lower-half data
[section .initl align=MB_ALIGN]
; the multiboot header
mb_hdr_start:
  dd MB_MAGIC
  dd MB_ARCH_I386
  dd (mb_hdr_end - mb_hdr_start)
  dd (0 - MB_MAGIC - MB_ARCH_I386 - (mb_hdr_end - mb_hdr_start)) & 0xFFFFFFFF

  mb_tag_info_start:
    dw MB_TAG_INFO
    dw 0
    dd (mb_tag_info_end - mb_tag_info_start)
    dd MB_INFO_MODULES
    dd MB_INFO_MMAP
  mb_tag_info_end:

  mb_tag_align_start:
    dw MB_TAG_ALIGN
    dw 0
    dd (mb_tag_align_end - mb_tag_align_start)
  mb_tag_align_end:

  mb_tag_terminator_start:
    dw MB_TAG_TERMINATOR
    dw 0
    dd (mb_tag_terminator_end - mb_tag_terminator_start)
  mb_tag_terminator_end:
mb_hdr_end:

; paging structures
align PAGE_SIZE
[global boot_pml4]
boot_pml4:
  dq (boot_pml3l + PG_PRESENT + PG_WRITABLE)
  times (TABLE_SIZE - 4) dq 0
  dq (identity_pml3 + PG_PRESENT + PG_WRITABLE)
  dq (boot_pml4 + PG_PRESENT + PG_WRITABLE + PG_NO_EXEC)
  dq (boot_pml3h + PG_PRESENT + PG_WRITABLE)

boot_pml3l:
  dq (boot_pml2 + PG_PRESENT + PG_WRITABLE)
  dq 0
  times (TABLE_SIZE - 2) dq 0

boot_pml3h:
  times (TABLE_SIZE - 2) dq 0
  dq (boot_pml2 + PG_PRESENT + PG_WRITABLE)
  dq 0

boot_pml2:
  dq (0x0 + PG_PRESENT + PG_WRITABLE + PG_BIG)
  times (TABLE_SIZE - 1) dq 0

identity_pml3:
  times (TABLE_SIZE - 5) dq 0
  dq (pmm_stack_pml2 + PG_PRESENT + PG_WRITABLE)
  dq (identity_pml2a + PG_PRESENT + PG_WRITABLE)
  dq (identity_pml2b + PG_PRESENT + PG_WRITABLE)
  dq (identity_pml2c + PG_PRESENT + PG_WRITABLE)
  dq (identity_pml2d + PG_PRESENT + PG_WRITABLE)

pmm_stack_pml2:
  times (TABLE_SIZE - 1) dq 0
  dq (pmm_stack_pml1 + PG_PRESENT + PG_WRITABLE)

pmm_stack_pml1:
  times TABLE_SIZE dq 0

identity_pml2a:
  %assign pg 0
  %rep TABLE_SIZE
    dq (pg + PG_PRESENT + PG_WRITABLE + PG_BIG + PG_NO_EXEC)
    %assign pg pg+PAGE_SIZE*TABLE_SIZE
  %endrep

identity_pml2b:
  %rep TABLE_SIZE
    dq (pg + PG_PRESENT + PG_WRITABLE + PG_BIG + PG_NO_EXEC)
    %assign pg pg+PAGE_SIZE*TABLE_SIZE
  %endrep

identity_pml2c:
  %rep TABLE_SIZE
    dq (pg + PG_PRESENT + PG_WRITABLE + PG_BIG + PG_NO_EXEC)
    %assign pg pg+PAGE_SIZE*TABLE_SIZE
  %endrep

identity_pml2d:
  %rep TABLE_SIZE
    dq (pg + PG_PRESENT + PG_WRITABLE + PG_BIG + PG_NO_EXEC)
    %assign pg pg+PAGE_SIZE*TABLE_SIZE
  %endrep

; the global descriptor table
gdt:
  ; null selector
    dq 0
  ; cs selector
    dq 0x00AF98000000FFFF
  ; ds selector
    dq 0x00CF92000000FFFF
gdt_end:
  dq 0 ; some extra padding so the gdtr is 16-byte aligned
gdtr:
  dw gdt_end - gdt - 1
  dq gdt

; the entry point of the kernel executable
[global start]
start:
  ; move the info GRUB passes us into the registers used for the main() call
  ; later on
  mov edi, eax
  mov esi, ebx

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
  mov ss, ax
  mov ax, 0x0
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  jmp 0x08:.trampoline

; some 64-bit code in the lower half used to jump to the higher half
[bits 64]
.trampoline:
  ; enter the higher half
  mov rax, qword .next
  jmp rax

; the higher-half code
[section .inith]
.next:
  ; re-load the GDTR with a virtual base address
  mov rax, [gdtr + 2]
  mov rbx, KERNEL_VMA
  add rax, rbx
  mov [gdtr + 2], rax
  mov rax, gdtr + KERNEL_VMA
  lgdt [rax]

  ; map the rest of the kernel into virtual memory
  mov rax, _start - KERNEL_VMA      ; first page number
  shr rax, LOG_PAGE_SIZE + LOG_TABLE_SIZE
  mov rbx, _end - KERNEL_VMA        ; last page number
  shr rbx, LOG_PAGE_SIZE + LOG_TABLE_SIZE
  mov rcx, boot_pml2 + KERNEL_VMA   ; pointer into pml2 table
  .map_page:
    ; calculate the value of the page table entry
    mov rdx, rax
    shl rdx, LOG_PAGE_SIZE + LOG_TABLE_SIZE
    mov r8, rdx
    mov r9, KERNEL_VMA
    add r8, r9
    or rdx, PG_PRESENT + PG_WRITABLE + PG_BIG

    ; write the page table entry
    mov [rcx], rdx
    invlpg [r8]

    ; increment pml2 pointer
    add rcx, 8

    ; check if we should terminate the loop
    cmp rax, rbx
    je .map_page_end

    ; increment the counter and map the next page
    inc rax
    jmp .map_page
  .map_page_end:

  ; set up the new stack (multiboot2 spec says the stack pointer could be
  ; anything - even pointing to invalid memory)
  mov rbp, 0 ; terminate stack traces here
  mov rsp, qword stack + STACK_SIZE

  ; unmap the identity-mapped memory
  mov qword [boot_pml4], 0x0

  ; invalidate the TLB cache for the identity-mapped memory
  invlpg [0x0]

  ; clear the RFLAGS register
  push 0x0
  popf

  ; call the kernel
  ; - the arguments were moved into EDI and ESI at the start
  ; - the DF has been reset by the code above - no CLD is required
  call init

; memory reserved for the kernel's stack
[section .bss align=STACK_ALIGN]
stack:
  resb STACK_SIZE
