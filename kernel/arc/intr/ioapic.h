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

#ifndef ARC_INTR_IOAPIC_H
#define ARC_INTR_IOAPIC_H

#include <stdbool.h>
#include <stdint.h>
#include <arc/types.h>

typedef struct ioapic
{
  /* a pointer to the next I/O APIC */
  struct ioapic *next;

  /* the id of this I/O APIC */
  ioapic_id_t id;

  /* the MMIO registers */
  volatile uint32_t *reg, *val;

  /* the base IRQ number */
  irq_t irq_base;

  /* the number of IRQs this I/O APIC routes */
  irq_t irqs;
} ioapic_t;

bool ioapic_init(ioapic_id_t id, uintptr_t addr, irq_t irq_base);
ioapic_t *ioapic_iter(void);
void ioapic_route(ioapic_t *apic, irq_tuple_t *tuple, intr_t intr);
void ioapic_mask(ioapic_t *apic, irq_tuple_t *tuple);

#endif

