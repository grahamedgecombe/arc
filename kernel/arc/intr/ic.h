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

#ifndef ARC_INTR_IC_H
#define ARC_INTR_IC_H

#include <arc/intr/ic.h>
#include <arc/intr/common.h>
#include <arc/smp/cpu.h>
#include <stdarg.h>

#define IC_TYPE_NONE    0x0
#define IC_TYPE_PIC     0x1
#define IC_TYPE_LAPIC   0x2
#define IC_TYPE_LX2APIC 0x3

void ic_bsp_init(int type, ...);
void ic_bsp_vinit(int type, va_list args);
void ic_ap_init(void);

void ic_ack(intr_id_t id);

void ic_ipi_init(cpu_lapic_id_t id);
void ic_ipi_startup(cpu_lapic_id_t id, uint8_t trampoline_addr);

#endif

