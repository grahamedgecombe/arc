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

#ifndef ARC_BUS_ISA_H
#define ARC_BUS_ISA_H

#include <arc/types.h>
#include <stdint.h>

/*
 * There are a maximum of 16 ISA interrupts as two PIC chips are used in
 * master-slave mode.
 */
#define ISA_INTR_LINES 16

typedef uint8_t isa_line_t;

/* Sets up the initial mapping of ISA interrupt lines to IRQ numbers. */
void isa_init(void);

/*
 * Returns a pointer to the tuple which describes a particular ISA interrupt
 * line.
 */
irq_tuple_t *isa_irq(isa_line_t line);

#endif
