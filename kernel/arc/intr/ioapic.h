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

#define IOAPIC_ID     0x00
#define IOAPIC_VER    0x01
#define IOAPIC_ARB    0x02
#define IOAPIC_REDTBL 0x10

/*
 * first 8 bits of redtbl entry = vector
 * last 8 bits of redtbl entry  = dest (apic id or set of processors)
 */

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

typedef struct ioapic
{
  struct ioapic *next; /* a pointer to the next I/O APIC */
  ioapic_id_t id; /* the id of this I/O APIC */
  volatile uint32_t *reg, *val; /* the MMIO registers */
  gsi_t intr_base; /* the global system interrupt base */
  intr_id_t intrs; /* the number of interrupts this I/O APIC redirects */
} ioapic_t;

bool ioapic_init(ioapic_id_t id, uintptr_t addr, gsi_t intr_base);
ioapic_t *ioapic_iter(void);

#endif

