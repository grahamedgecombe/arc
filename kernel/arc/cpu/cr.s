;
;  Copyright (c) 2011-2013 Graham Edgecombe <graham@grahamedgecombe.com>
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

[global cr0_read]
cr0_read:
  push rbp
  mov rbp, rsp
  mov rax, cr0
  pop rbp
  ret

[global cr0_write]
cr0_write:
  push rbp
  mov rbp, rsp
  mov cr0, rdi
  pop rbp
  ret

[global cr2_read]
cr2_read:
  push rbp
  mov rbp, rsp
  mov rax, cr2
  pop rbp
  ret

[global cr3_read]
cr3_read:
  push rbp
  mov rbp, rsp
  mov rax, cr3
  pop rbp
  ret

[global cr3_write]
cr3_write:
  push rbp
  mov rbp, rsp
  mov cr3, rdi
  pop rbp
  ret

[global cr4_read]
cr4_read:
  push rbp
  mov rbp, rsp
  mov rax, cr4
  pop rbp
  ret

[global cr4_write]
cr4_write:
  push rbp
  mov rbp, rsp
  mov cr4, rdi
  pop rbp
  ret
