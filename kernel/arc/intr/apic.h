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

#ifndef ARC_INTR_APIC_H
#define ARC_INTR_APIC_H

#include <arc/types.h>
#include <stdbool.h>
#include <stdint.h>

/* LVT registers */
#define APIC_LVT_TIMER  0x32
#define APIC_LVT_LINT0  0x35
#define APIC_LVT_LINT1  0x36
#define APIC_LVT_ERROR  0x37

/* LVT flags */
#define LVT_MASKED         0x00010000
#define LVT_TYPE_FIXED     0x00000000
#define LVT_TYPE_SMI       0x00000200
#define LVT_TYPE_NMI       0x00000400
#define LVT_TYPE_EXTINT    0x00000700
#define LVT_DELIVS         0x00001000
#define LVT_REMOTE_IRR     0x00004000
#define LVT_TRIGGER_LEVEL  0x00008000
#define LVT_TRIGGER_EDGE   0x00000000
#define LVT_TIMER_PERIODIC 0x00020000
#define LVT_TIMER_ONE_SHOT 0x00000000

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

