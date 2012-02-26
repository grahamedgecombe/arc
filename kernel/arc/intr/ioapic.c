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
#include <arc/mm/mmio.h>
#include <arc/types.h>
#include <stdlib.h>

#define IOAPIC_ID     0x00
#define IOAPIC_VER    0x01
#define IOAPIC_ARB    0x02
#define IOAPIC_REDTBL 0x10

#define REDTBL_MASK             0x0000000000010000
#define REDTBL_TRIGGER_LEVEL    0x0000000000008000
#define REDTBL_TRIGGER_EDGE     0x0000000000000000
#define REDTBL_REMOTE_IRR       0x0000000000004000
#define REDTBL_ACTIVE_HIGH      0x0000000000002000
#define REDTBL_ACTIVE_LOW       0x0000000000000000
#define REDTBL_DELIVS           0x0000000000001000
#define REDTBL_DESTMOD_LOGICAL  0x0000000000000800
#define REDTBL_DESTMOD_PHYSICAL 0x0000000000000000
#define REDTBL_DELMOD_FIXED     0x0000000000000000
#define REDTBL_DELMOD_LOWPRI    0x0000000000000400
#define REDTBL_DELMOD_SMI       0x0000000000000200
#define REDTBL_DELMOD_NMI       0x0000000000000100
#define REDTBL_DELMOD_INIT      0x0000000000000500
#define REDTBL_DELMOD_EXTINT    0x0000000000000700

static ioapic_t *ioapic_head; /* a pointer to the first I/O APIC in the system */

static uint32_t ioapic_read(ioapic_t *apic, uint32_t reg)
{
  *(apic->reg) = reg;
  return *(apic->val);
}

static void ioapic_write(ioapic_t *apic, uint32_t reg, uint32_t val)
{
  *(apic->reg) = reg;
  *(apic->val) = val;
}

bool ioapic_init(ioapic_id_t id, uintptr_t addr, irq_t irq_base)
{
  ioapic_t *apic = malloc(sizeof(*apic));
  if (!apic)
    return false;

  uintptr_t virt_addr = (uintptr_t) mmio_map(addr, 32, MMIO_R | MMIO_W);
  if (!virt_addr)
  {
    free(apic);
    return false;
  }

  static ioapic_t *ioapic_tail = 0;
  if (ioapic_tail)
  {
    ioapic_tail->next = apic;
    ioapic_tail = apic;
  }
  else
  {
    ioapic_head = ioapic_tail = apic;
  }

  apic->next = 0;
  apic->id = id;
  apic->irq_base = irq_base;
  apic->reg = (volatile uint32_t *) virt_addr;
  apic->val = (volatile uint32_t *) (virt_addr + 16);
  apic->irqs = ((ioapic_read(apic, IOAPIC_VER) >> 16) & 0xFF) + 1;
  apic->_phy_addr = addr;
  return true;
}

ioapic_t *ioapic_iter(void)
{
  return ioapic_head;
}

void ioapic_route(ioapic_t *apic, irq_t src, intr_t vec, bool high, bool lt)
{
  uint8_t dst = 0xFF;
  uint64_t redtbl_entry = REDTBL_DESTMOD_PHYSICAL | REDTBL_DELMOD_LOWPRI | (((uint64_t) (dst & 0xFF)) << 56) | (vec & 0xFF);

  if (high)
    redtbl_entry |= REDTBL_ACTIVE_HIGH;
  else
    redtbl_entry |= REDTBL_ACTIVE_LOW;

  if (lt)
    redtbl_entry |= REDTBL_TRIGGER_LEVEL;
  else
    redtbl_entry |= REDTBL_TRIGGER_EDGE;

  ioapic_write(apic, IOAPIC_REDTBL + 2 * src + 1, (redtbl_entry >> 32) & 0xFFFFFFFF);
  ioapic_write(apic, IOAPIC_REDTBL + 2 * src, redtbl_entry & 0xFFFFFFFF);
}

