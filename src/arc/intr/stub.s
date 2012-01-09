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

[extern intr_dispatch]

; fault without error entry code
%macro fault 1
[global fault%1]
fault%1:
  push 0
  push %1
  jmp intr_stub
%endmacro

; fault with error entry code
%macro fault_err 1
[global fault%1]
fault%1:
  push %1
  jmp intr_stub
%endmacro

; irq entry code
%macro irq 2
[global irq%1]
irq%1:
  push 0
  push %2
  jmp intr_stub
%endmacro

; syscall entry code
[global sys_call]
sys_call:
  push 0
  push 0xFF
  jmp intr_stub

intr_stub:
  ; save the register file
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rbp
  push rdi
  push rsi
  push rdx
  push rcx
  push rbx
  push rax

  ; call the C routine for dispatching an interrupt
  cld          ; amd64 SysV ABI states the DF must be cleared by the caller
  mov rdi, rsp ; first argument points to the processor state
  call intr_dispatch

  ; restore the register file
  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  pop rbp
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15

  ; pop the error code and interrupt id
  add rsp, 16

  ; return
  iretq

; the fault and irq stubs are actually made here
fault     0
fault     1
fault     2
fault     3
fault     4
fault     5
fault     6
fault     7
fault_err 8
fault     9
fault_err 10
fault_err 11
fault_err 12
fault_err 13
fault_err 14
fault     15
fault     16
fault_err 17
fault     18
fault     19
fault     20
fault     21
fault     22
fault     23
fault     24
fault     25
fault     26
fault     27
fault     28
fault     29
fault     30
fault     31

irq 0,  32
irq 1,  33
irq 2,  34
irq 3,  35
irq 4,  36
irq 5,  37
irq 6,  38
irq 7,  39
irq 8,  40
irq 9,  41
irq 10, 42
irq 11, 43
irq 12, 44
irq 13, 45
irq 14, 46
irq 15, 47
irq 16, 48
irq 17, 49
irq 18, 50
irq 19, 51
irq 20, 52
irq 21, 53
irq 22, 54
irq 23, 55

