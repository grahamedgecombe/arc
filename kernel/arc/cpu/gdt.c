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

#include <arc/cpu/gdt.h>
#include <arc/cpu/msr.h>
#include <arc/smp/cpu.h>
#include <string.h>

/* flags */
#define GDT_CS       0x18
#define GDT_DS       0x10
#define GDT_TSS      0x09
#define GDT_WRITABLE 0x02
#define GDT_USER     0x60
#define GDT_PRESENT  0x80

/* granularity */
#define GDT_LM       0x2

static void gdt_set_gate(gdt_gate_t *gdt_gates, uint16_t sel, uint8_t flags, uint8_t gran)
{
  gdt_gate_t *gate = &gdt_gates[sel / sizeof(*gdt_gates)];
  gate->flags = flags;
  gate->granularity = (gran << 4) | 0x0F;
  gate->limit_low = 0xFFFF;
}

static void gdt_set_xgate(gdt_gate_t *gdt_gates, uint16_t sel, uint8_t flags, uint8_t gran, uint64_t base, uint64_t limit)
{
  gdt_xgate_t *gate = (gdt_xgate_t *) (&gdt_gates[sel / sizeof(*gdt_gates)]);
  gate->low.flags = flags;
  gate->low.granularity = (gran << 4) | ((limit >> 16) & 0x0F);
  gate->low.limit_low = limit & 0xFFFF;
  gate->low.base_low = base & 0xFFFF;
  gate->low.base_mid = ((base >> 16) & 0xFF);
  gate->low.base_high = ((base >> 24) & 0xFF);
  gate->high.base_xhigh = ((base >> 32) & 0xFFFFFFFF);
  gate->high.reserved = 0;
}

void gdt_init(void)
{
  /* get this CPU's local data */
  cpu_t *cpu = cpu_get();

  /* get pointers to the GDT and GDTR */
  gdtr_t *gdtr = &cpu->gdtr;
  gdt_gate_t *gdt_gates = cpu->gdt_gates;

  /* get pointer to the TSS and calculate the limit */
  tss_t *tss = &cpu->tss;
  uint64_t tss_base = (uint64_t) tss;
  uint64_t tss_limit = sizeof(*tss);

  /* reset the GDT */
  memset(gdt_gates, 0, sizeof(*gdt_gates) * GDT_GATES);

  /* fill in the entries we need */
  gdt_set_gate( gdt_gates, SLTR_KERNEL_CODE, GDT_PRESENT | GDT_CS,                           GDT_LM);
  gdt_set_gate( gdt_gates, SLTR_KERNEL_DATA, GDT_PRESENT | GDT_DS | GDT_WRITABLE,            0);
  gdt_set_gate( gdt_gates, SLTR_USER_DATA,   GDT_PRESENT | GDT_DS | GDT_USER | GDT_WRITABLE, 0);
  gdt_set_gate( gdt_gates, SLTR_USER_CODE,   GDT_PRESENT | GDT_CS | GDT_USER,                GDT_LM);
  gdt_set_xgate(gdt_gates, SLTR_TSS,         GDT_PRESENT | GDT_TSS,                          0, tss_base, tss_limit);

  /*
   * read the GS_BASE MSRs so we can restore it after updating the segment
   * registers
   */
  uint64_t gs_base = msr_read(MSR_GS_BASE);

  /* update the GDTR structure and install it */
  gdtr->addr = (uint64_t) gdt_gates;
  gdtr->len = sizeof(*gdt_gates) * GDT_GATES - 1;
  gdtr_install(gdtr, SLTR_KERNEL_CODE, SLTR_KERNEL_DATA);

  /* restore the GS_BASE and GS_KERNEL_BASE MSR */
  msr_write(MSR_GS_BASE, gs_base);
  msr_write(MSR_GS_KERNEL_BASE, gs_base);
}
