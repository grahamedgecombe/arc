/*
 * Copyright (c) 2011-2013 Graham Edgecombe <graham@grahamedgecombe.com>
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

#ifndef ARC_INTR_APIC_H
#define ARC_INTR_APIC_H

#include <arc/types.h>
#include <stdbool.h>
#include <stdint.h>

/* global xAPIC/x2APIC enable */
bool xapic_init(uintptr_t addr); /* init in xAPIC mode using MMIO */
void x2apic_init(void);          /* init in x2APIC mode using MSRs */

/* enable this CPU's APIC */
void apic_init(void);

/* acknowledge an interrupt */
void apic_ack(void);

/* IPIs */
void apic_ipi_init(cpu_lapic_id_t id);
void apic_ipi_startup(cpu_lapic_id_t id, uint8_t trampoline_addr);
void apic_ipi_fixed(cpu_lapic_id_t id, intr_t intr);
void apic_ipi_all(intr_t intr);
void apic_ipi_all_exc_self(intr_t intr);
void apic_ipi_self(intr_t intr);

#endif
