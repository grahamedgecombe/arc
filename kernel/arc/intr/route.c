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

#include <arc/intr/route.h>
#include <arc/intr/ic.h>
#include <arc/intr/ioapic.h>
#include <arc/tty.h>

void intr_route_init(void)
{
  ic_print_info();

  for (ioapic_t *apic = ioapic_iter(); apic; apic = apic->next)
  {
    uint64_t addr = apic->_phy_addr;
    ioapic_id_t id = apic->id;
    gsi_t intr_first = apic->intr_base;
    gsi_t intr_last = apic->intr_base + apic->intrs - 1;
    tty_printf(" => Using I/O APIC (at %0#18x, id %0#4x, intrs %d-%d)\n", addr, id, intr_first, intr_last);
  }
}

