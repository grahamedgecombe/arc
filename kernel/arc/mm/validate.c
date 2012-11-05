/*
 * Copyright (c) 2011-2012 Graham Edgecombe <graham@grahamedgecombe.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <arc/mm/validate.h>
#include <arc/mm/common.h>
#include <stdint.h>

bool valid_buffer(const void *ptr, size_t len)
{
  uintptr_t addr_start = (uintptr_t) ptr;
  uintptr_t addr_end = addr_start + len;
  return addr_start <= VM_USER_END && addr_end <= VM_USER_END && addr_start <= addr_end;
}

bool valid_string(const char *str)
{
  for (;;)
  {
    if ((uintptr_t) str > VM_USER_END)
      return false;

    if (*str++ == 0)
      return true;
  }
}
