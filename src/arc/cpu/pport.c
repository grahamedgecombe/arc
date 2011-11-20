/*
 * Copyright (c) 2011 Graham Edgecombe <graham@grahamedgecombe.com>
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

#include <arc/cpu/port.h>

uint8_t inb_p(uint16_t port)
{
  uint8_t value = inb(port);
  iowait();
  return value;
}

uint16_t inw_p(uint16_t port)
{
  uint16_t value = inw(port);
  iowait();
  return value;
}

uint32_t inl_p(uint16_t port)
{
  uint32_t value = inl(port);
  iowait();
  return value;
}

void outb_p(uint16_t port, uint8_t value)
{
  outb(port, value);
  iowait();
}

void outw_p(uint16_t port, uint16_t value)
{
  outw(port, value);
  iowait();
}

void outl_p(uint16_t port, uint32_t value)
{
  outl(port, value);
  iowait();
}

