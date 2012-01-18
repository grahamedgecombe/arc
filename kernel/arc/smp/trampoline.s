;
;  Copyright (c) 2011-2012 Graham Edgecombe <graham@grahamedgecombe.com>
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

[bits 16]

TRAMPOLINE_BASE equ 0x1000

; MSR numbers
MSR_EFER equ 0xC0000080

; EFER bitmasks
EFER_LM equ 0x100
EFER_NX equ 0x800

; CR0 bitmasks
CR0_PE equ 0x1
CR0_PAGING equ 0x80000000

; CR4 bitmasks
CR4_PAE equ 0x20
CR4_PSE equ 0x10

[extern boot_pml4]
[extern smp_ap_init]

[global trampoline_start]
trampoline_start:
  ; disable interrupts straight away as the IVT is probably trash right now
  cli

  ; zero all segments (apart from the code segment)
  mov ax, 0x0
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; set up a temporary protected-mode IDT and GDT
  lidt [pm_idtr - trampoline_start + TRAMPOLINE_BASE]
  lgdt [pm_gdtr - trampoline_start + TRAMPOLINE_BASE]

  ; set the PE flag
  mov eax, cr0
  or al, CR0_PE
  mov cr0, eax

  ; jump to the 32-bit code
  jmp 0x8:(code32 - trampoline_start + TRAMPOLINE_BASE)

[bits 32]
code32:
  ; re-load segment selectors
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

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

  ; enable paging (the BSP already identity-mapped us)
  mov eax, cr0
  or eax, CR0_PAGING
  mov cr0, eax

  ; leave compatibility mode
  lgdt [lm_gdtr - trampoline_start + TRAMPOLINE_BASE]
  jmp 0x08:(code64 - trampoline_start + TRAMPOLINE_BASE)

[bits 64]
code64:
  ; re-load segment selectors
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; switch the RIP to use the higher half virtual address instead of the
  ; identity-mapped virtual address
  mov rax, qword vcode64
  jmp rax
vcode64:

  ; set up the stack
  mov rbp, 0x0
  mov rax, [qword trampoline_stack]
  mov rsp, rax

  ; reset RFLAGS
  push 0x0
  popf

  ; call the AP init C code
  mov rax, qword smp_ap_init
  call rax

  ; hang the CPU forever
  cli
  .hang:
    hlt
    jmp .hang

  ; long mode gdt and gdtr
align 16
  lm_gdtr:
    dw lm_gdt_end - lm_gdt_start - 1
    dq lm_gdt_start - trampoline_start + TRAMPOLINE_BASE

align 16
  lm_gdt_start:
    ; null selector
    dq 0
    ; cs selector
    dq 0x00AF98000000FFFF
    ; ds selector
    dq 0x00CF92000000FFFF
  lm_gdt_end:

  ; protected mode gdt and gdtr
align 16
  pm_gdtr:
    dw pm_gdt_end - pm_gdt_start - 1
    dd pm_gdt_start - trampoline_start + TRAMPOLINE_BASE

align 16
  pm_gdt_start:
    ; null selector
    dq 0
    ; cs selector
    dq 0x00CF9A000000FFFF
    ; ds selector
    dq 0x00CF92000000FFFF
  pm_gdt_end:

  ; protected mode idtr
align 16
  pm_idtr:
    dw 0
    dd 0
    dd 0 ; so this is also a valid null idtr in long mode

[global trampoline_stack]
trampoline_stack: ; variable for storing the virtual address of the AP's bootstrap stack
  dq 0

[global trampoline_end]
trampoline_end:

