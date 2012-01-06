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

#ifndef ARC_INTR_LAPIC_H
#define ARC_INTR_LAPIC_H

#include <stdint.h>

#define LAPIC_ID           0x08
#define LAPIC_VER          0x0C
#define LAPIC_TPR          0x20
#define LAPIC_APR          0x24
#define LAPIC_PPR          0x28
#define LAPIC_EOI          0x2C
#define LAPIC_RRD          0x30
#define LAPIC_SPURIOUS_VEC 0x3C
#define LAPIC_LDR          0x34
#define LAPIC_DFR          0x38
#define LAPIC_ISR0         0x40
#define LAPIC_ISR1         0x44
#define LAPIC_ISR2         0x48
#define LAPIC_ISR3         0x4C
#define LAPIC_ISR4         0x50
#define LAPIC_ISR5         0x54
#define LAPIC_ISR6         0x58
#define LAPIC_ISR7         0x5C
#define LAPIC_TMR0         0x60
#define LAPIC_TMR1         0x64
#define LAPIC_TMR2         0x68
#define LAPIC_TMR3         0x6C
#define LAPIC_TMR4         0x70
#define LAPIC_TMR5         0x74
#define LAPIC_TMR6         0x78
#define LAPIC_TMR7         0x7C
#define LAPIC_IRR0         0x80
#define LAPIC_IRR1         0x84
#define LAPIC_IRR2         0x88
#define LAPIC_IRR3         0x8C
#define LAPIC_IRR4         0x90
#define LAPIC_IRR5         0x94
#define LAPIC_IRR6         0x98
#define LAPIC_IRR7         0x9C
#define LAPIC_ESR          0xA0
#define LAPIC_LVT_CMCI     0xBC
#define LAPIC_ICR0         0xC0
#define LAPIC_ICR1         0xC4
#define LAPIC_LVT_TIMER    0xC8
#define LAPIC_LVT_LINT0    0xD4
#define LAPIC_LVT_LINT1    0xD8
#define LAPIC_LVT_ERROR    0xDC
#define LAPIC_TIMER_ICR    0xE0
#define LAPIC_TIMER_CCR    0xE4
#define LAPIC_TIMER_DCR    0xF8

void lapic_mmio_init(uint32_t mmio_addr);
void lapic_init(void);
void lapic_ack(void);

#endif

