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

[section .text]
[global _start]
_start:
  mov rdi, hello_str
  push rcx
  push r11
  syscall
  pop r11
  pop rcx
  jmp $

[section .data]
hello_str:
  db `hello, world!\n`
  db 0

