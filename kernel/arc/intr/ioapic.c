/*
 * Copyright (c) 2011-2014 Graham Edgecombe <graham@grahamedgecombe.com>
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
#include <arc/lock/barrier.h>
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
#define REDTBL_DELMOD_LOWPRI    0x0000000000000100
#define REDTBL_DELMOD_SMI       0x0000000000000200
#define REDTBL_DELMOD_NMI       0x0000000000000400
#define REDTBL_DELMOD_INIT      0x0000000000000500
#define REDTBL_DELMOD_EXTINT    0x0000000000000700

list_t ioapic_list = LIST_EMPTY;

static uint32_t ioapic_read(ioapic_t *apic, uint32_t reg)
{
  *(apic->reg) = reg;
  barrier();
  return *(apic->val);
}

static void ioapic_write(ioapic_t *apic, uint32_t reg, uint32_t val)
{
  *(apic->reg) = reg;
  barrier();
  *(apic->val) = val;
}

bool ioapic_init(ioapic_id_t id, uintptr_t addr, irq_t irq_base)
{
  ioapic_t *apic = malloc(sizeof(*apic));
  if (!apic)
    return false;

  uintptr_t virt_addr = (uintptr_t) mmio_map(addr, 32, VM_R | VM_W);
  if (!virt_addr)
  {
    free(apic);
    return false;
  }

  apic->id = id;
  apic->irq_base = irq_base;
  apic->reg = (uint32_t *) virt_addr;
  apic->val = (uint32_t *) (virt_addr + 16);
  apic->irqs = ((ioapic_read(apic, IOAPIC_VER) >> 16) & 0xFF) + 1;

  list_add_tail(&ioapic_list, &apic->node);
  return true;
}

void ioapic_route(ioapic_t *apic, irq_tuple_t *tuple, intr_t intr)
{
  // TODO: bochs does not support lowest priority delivery in physical
  //       destination mode, should switch this back to lowest priority and set
  //       up logical mode instead
  uint8_t src = tuple->irq - apic->irq_base;
  uint64_t redtbl_entry = REDTBL_DESTMOD_PHYSICAL | REDTBL_DELMOD_FIXED | intr;

  if (tuple->active_polarity == POLARITY_HIGH)
    redtbl_entry |= REDTBL_ACTIVE_HIGH;
  else
    redtbl_entry |= REDTBL_ACTIVE_LOW;

  if (tuple->trigger == TRIGGER_LEVEL)
    redtbl_entry |= REDTBL_TRIGGER_LEVEL;
  else
    redtbl_entry |= REDTBL_TRIGGER_EDGE;

  ioapic_write(apic, IOAPIC_REDTBL + 2 * src + 1, (redtbl_entry >> 32) & 0xFFFFFFFF);
  ioapic_write(apic, IOAPIC_REDTBL + 2 * src, redtbl_entry & 0xFFFFFFFF); /* unset mask bit last */
}

void ioapic_route_nmi(ioapic_t *apic, irq_tuple_t *tuple)
{
  // TODO: bochs does not support lowest priority delivery in physical
  //       destination mode, should switch this back to lowest priority and set
  //       up logical mode instead
  uint8_t src = tuple->irq - apic->irq_base;
  uint64_t redtbl_entry = REDTBL_DESTMOD_PHYSICAL | REDTBL_DELMOD_NMI;

  if (tuple->active_polarity == POLARITY_HIGH)
    redtbl_entry |= REDTBL_ACTIVE_HIGH;
  else
    redtbl_entry |= REDTBL_ACTIVE_LOW;

  if (tuple->trigger == TRIGGER_LEVEL)
    redtbl_entry |= REDTBL_TRIGGER_LEVEL;
  else
    redtbl_entry |= REDTBL_TRIGGER_EDGE;

  ioapic_write(apic, IOAPIC_REDTBL + 2 * src + 1, (redtbl_entry >> 32) & 0xFFFFFFFF);
  ioapic_write(apic, IOAPIC_REDTBL + 2 * src, redtbl_entry & 0xFFFFFFFF); /* unset mask bit last */
}

void ioapic_mask(ioapic_t *apic, irq_tuple_t *tuple)
{
  uint8_t src = tuple->irq - apic->irq_base;
  uint64_t redtbl_entry = REDTBL_MASK;

  ioapic_write(apic, IOAPIC_REDTBL + 2 * src, redtbl_entry & 0xFFFFFFFF); /* set mask bit first */
  ioapic_write(apic, IOAPIC_REDTBL + 2 * src + 1, (redtbl_entry >> 32) & 0xFFFFFFFF);
}
