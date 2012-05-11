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

[extern syscall_table]
[extern syscall_table_size]

[global syscall_stub]
syscall_stub:
  ; switch the GS base to the kernel's
  swapgs

  ; switch to the kernel stack
  mov [gs:16], rsp
  mov rsp, [gs:8]

  ; it is safe for to re-enable interrupts now, for information about the
  ; race condition see syscall.c where the SYSCALL flags mask is set
  sti

  ; preserve RCX and R11, these are used by SYSCALL/SYSRET
  push rcx
  push r11

  ; check if the syscall number is out of range
  mov r11, qword syscall_table_size
  cmp rax, [r11]
  jge .invalid_syscall

  ; call the function in the syscall table
  mov r11, qword syscall_table
  mov rcx, r10 ; syscall ABI uses R10 instead of RCX, fix that for normal ABI
  call [r11 + rax * 8]

.invalid_syscall:
  ; TODO: we probably want some sort of error upon an invalid syscall

  ; restore the RCX and R11 registers
  pop r11
  pop rcx

  ; mask interrupts again, for the same race condition reasons
  cli

  ; switch back to the user stack
  mov rsp, [gs:16]

  ; switch the GS base to the user's
  swapgs

  ; return to user long mode
  o64 sysret

