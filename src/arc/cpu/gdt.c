/*
 * Copyright (c) 2011 Graham Edgecombe <graham@grahamedgecombe.com>
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

#include <arc/cpu/gdt.h>
#include <string.h>

static gdt_gate_t gdt_gates[GDT_GATES];
static gdtr_t gdtr;

static void gdt_set_gate(uint16_t sel, uint8_t flags, uint8_t gran)
{
  gdt_gate_t *gate = &gdt_gates[sel / sizeof(gdt_gate_t)];
  gate->flags = flags;
  gate->granularity = (gran << 4) | 0x0F;
  gate->limit_low = 0xFFFF;
}

extern int gdt;

void gdt_init(void)
{
  memset(gdt_gates, 0, sizeof(gdt_gates));

  gdt_set_gate(SLTR_KERNEL_CODE, 0x98, 0xA);
  gdt_set_gate(SLTR_KERNEL_DATA, 0x92, 0xC);
  gdt_set_gate(SLTR_USER_CODE,   0xF8, 0xA);
  gdt_set_gate(SLTR_USER_DATA,   0xF2, 0xC);

  gdtr.addr = (uint64_t) gdt_gates;
  gdtr.len = sizeof(gdt_gates) - 1;
  gdtr_install(&gdtr, SLTR_KERNEL_CODE, SLTR_KERNEL_DATA);
}

