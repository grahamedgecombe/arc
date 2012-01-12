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

#ifndef ARC_SMP_PERCPU_H
#define ARC_SMP_PERCPU_H

#include <arc/cpu/gdt.h>

typedef struct percpu
{
  /*
   * percpu_get() relies on the fact that the first 8 bytes in the structure
   * points to itself
   */
  struct percpu *self;

  /* this processors' gdt and gdtr */
  gdtr_t gdtr;
  gdt_gate_t gdt_gates[GDT_GATES];
} percpu_t;

void percpu_bsp_init(void);
percpu_t *percpu_get(void);

#endif

