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
#include <stdbool.h>

/* IPI mode flags */
#define LAPIC_IPI_FIXED    0x00
#define LAPIC_IPI_SMI      0x02
#define LAPIC_IPI_NMI      0x04
#define LAPIC_IPI_INIT     0x05
#define LAPIC_IPI_STARTUP  0x06
#define LAPIC_IPI_PHYSICAL 0x00
#define LAPIC_IPI_LOGICAL  0x08
#define LAPIC_IPI_DEASSERT 0x00
#define LAPIC_IPI_ASSERT   0x40
#define LAPIC_IPI_ET       0x00
#define LAPIC_IPI_LT       0x80

/* IPI DSH */
#define LAPIC_IPI_SELF         0x400
#define LAPIC_IPI_ALL          0x800
#define LAPIC_IPI_ALL_EXC_SELF 0xC00

void lapic_print_info(void);
bool lapic_mmio_init(uintptr_t addr);
void lapic_init(void);
void lapic_ipi(uint8_t dest, uint16_t mode, uint8_t vector);
void lapic_ack(void);

#endif

