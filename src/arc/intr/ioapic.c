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

#include <arc/intr/ioapic.h>
#include <arc/intr/common.h>
#include <arc/mm/phy32.h>
#include <stdbool.h>

static uint32_t ioapic_read(uint32_t mmio_addr, uint32_t reg)
{
  volatile uint32_t *reg_ptr = (volatile uint32_t *) aphy32_to_virt(mmio_addr);
  volatile uint32_t *val_ptr = (volatile uint32_t *) aphy32_to_virt(mmio_addr + 0x10);
  *reg_ptr = reg;
  return *val_ptr;
}

static void ioapic_write(uint32_t mmio_addr, uint32_t reg, uint32_t val)
{
  volatile uint32_t *reg_ptr = (volatile uint32_t *) aphy32_to_virt(mmio_addr);
  volatile uint32_t *val_ptr = (volatile uint32_t *) aphy32_to_virt(mmio_addr + 0x10);
  *reg_ptr = reg;
  *val_ptr = val;
}

void ioapic_init(uint32_t mmio_addr)
{

}

void ioapic_route(uint32_t mmio_addr, intr_id_t src, intr_id_t vec, bool high, bool lt)
{
  uint8_t dst = 0xFF;
  uint64_t redtbl_entry = REDTBL_DESTMOD_PHYSICAL | REDTBL_DELMOD_FIXED | (((uint64_t) (dst & 0xFF)) << 56) | (vec & 0xFF);

  if (high)
    redtbl_entry |= REDTBL_ACTIVE_HIGH;
  else
    redtbl_entry |= REDTBL_ACTIVE_LOW;

  if (lt)
    redtbl_entry |= REDTBL_TRIGGER_LEVEL;
  else
    redtbl_entry |= REDTBL_TRIGGER_EDGE;

  ioapic_write(mmio_addr, IOAPIC_REDTBL0 + 2 * src, redtbl_entry & 0xFFFFFFFF);
  ioapic_write(mmio_addr, IOAPIC_REDTBL0 + 2 * src + 1, (redtbl_entry >> 32) & 0xFFFFFFFF);
}

