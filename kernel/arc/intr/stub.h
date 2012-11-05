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

#ifndef ARC_INTR_STUB_H
#define ARC_INTR_STUB_H

void fault0(void);
void fault1(void);
void fault2(void);
void fault3(void);
void fault4(void);
void fault5(void);
void fault6(void);
void fault7(void);
void fault8(void);
void fault9(void);
void fault10(void);
void fault11(void);
void fault12(void);
void fault13(void);
void fault14(void);
void fault15(void);
void fault16(void);
void fault17(void);
void fault18(void);
void fault19(void);
void fault20(void);
void fault21(void);
void fault22(void);
void fault22(void);
void fault23(void);
void fault24(void);
void fault25(void);
void fault26(void);
void fault27(void);
void fault28(void);
void fault29(void);
void fault30(void);
void fault31(void);

void irq0(void);
void irq1(void);
void irq2(void);
void irq3(void);
void irq4(void);
void irq5(void);
void irq6(void);
void irq7(void);
void irq8(void);
void irq9(void);
void irq10(void);
void irq11(void);
void irq12(void);
void irq13(void);
void irq14(void);
void irq15(void);
void irq16(void);
void irq17(void);
void irq18(void);
void irq19(void);
void irq20(void);
void irq21(void);
void irq22(void);
void irq23(void);

void ipi_route(void);
void ipi_panic(void);
void ipi_tlb(void);
void lvt_timer(void);
void lvt_error(void);
void spurious(void);

#endif
