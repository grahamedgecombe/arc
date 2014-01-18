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

#ifndef ARC_INTR_ROUTE_H
#define ARC_INTR_ROUTE_H

#include <arc/types.h>
#include <arc/cpu/state.h>
#include <stdbool.h>

typedef void (*intr_handler_t)(cpu_state_t *state);

void intr_route_init(void);

/* dispatches an interrupt */
void intr_dispatch(cpu_state_t *state);

/* route by interrupt id */
bool intr_route_intr(intr_t intr, intr_handler_t handler);
void intr_unroute_intr(intr_t intr, intr_handler_t handler);

/* route an IRQ to an interrupt */
bool intr_route_irq_to(irq_tuple_t *tuple, intr_t intr);
void intr_unroute_irq_to(irq_tuple_t *tuple, intr_t intr);

/*
 * route an IRQ to an interrupt handler directly without worrying about picking
 * an interrupt id - this function is a combination of the above functions
 */
bool intr_route_irq(irq_tuple_t *tuple, intr_handler_t handler);
void intr_unroute_irq(irq_tuple_t *tuple, intr_handler_t handler);

#endif
