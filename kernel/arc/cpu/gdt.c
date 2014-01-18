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

static void gdt_set_descriptor(gdt_descriptor_t *gdt_descriptors, uint16_t sel, uint8_t flags, uint8_t gran)
{
  gdt_descriptor_t *descriptor = &gdt_descriptors[sel / sizeof(*gdt_descriptors)];
  descriptor->flags = flags;
  descriptor->granularity = (gran << 4) | 0x0F;
  descriptor->limit_low = 0xFFFF;
}

static void gdt_set_xdescriptor(gdt_descriptor_t *gdt_descriptors, uint16_t sel, uint8_t flags, uint8_t gran, uint64_t base, uint64_t limit)
{
  gdt_xdescriptor_t *descriptor = (gdt_xdescriptor_t *) (&gdt_descriptors[sel / sizeof(*gdt_descriptors)]);
  descriptor->low.flags = flags;
  descriptor->low.granularity = (gran << 4) | ((limit >> 16) & 0x0F);
  descriptor->low.limit_low = limit & 0xFFFF;
  descriptor->low.base_low = base & 0xFFFF;
  descriptor->low.base_mid = ((base >> 16) & 0xFF);
  descriptor->low.base_high = ((base >> 24) & 0xFF);
  descriptor->high.base_xhigh = ((base >> 32) & 0xFFFFFFFF);
  descriptor->high.reserved = 0;
}

void gdt_init(void)
{
  /* get this CPU's local data */
  cpu_t *cpu = cpu_get();

  /* get pointers to the GDT and GDTR */
  gdtr_t *gdtr = &cpu->gdtr;
  gdt_descriptor_t *gdt_descriptors = cpu->gdt_descriptors;

  /* get pointer to the TSS and calculate the limit */
  tss_t *tss = &cpu->tss;
  uint64_t tss_base = (uint64_t) tss;
  uint64_t tss_limit = sizeof(*tss);

  /* reset the GDT */
  memset(gdt_descriptors, 0, sizeof(*gdt_descriptors) * GDT_DESCRIPTORS);

  /* fill in the entries we need */
  gdt_set_descriptor( gdt_descriptors, SLTR_KERNEL_CODE, GDT_PRESENT | GDT_CS,                           GDT_LM);
  gdt_set_descriptor( gdt_descriptors, SLTR_KERNEL_DATA, GDT_PRESENT | GDT_DS | GDT_WRITABLE,            0);
  gdt_set_descriptor( gdt_descriptors, SLTR_USER_DATA,   GDT_PRESENT | GDT_DS | GDT_USER | GDT_WRITABLE, 0);
  gdt_set_descriptor( gdt_descriptors, SLTR_USER_CODE,   GDT_PRESENT | GDT_CS | GDT_USER,                GDT_LM);
  gdt_set_xdescriptor(gdt_descriptors, SLTR_TSS,         GDT_PRESENT | GDT_TSS,                          0, tss_base, tss_limit);

  /*
   * read the GS_BASE MSRs so we can restore it after updating the segment
   * registers
   */
  uint64_t gs_base = msr_read(MSR_GS_BASE);

  /* update the GDTR structure and install it */
  gdtr->addr = (uint64_t) gdt_descriptors;
  gdtr->len = sizeof(*gdt_descriptors) * GDT_DESCRIPTORS - 1;
  gdtr_install(gdtr, SLTR_KERNEL_CODE, SLTR_KERNEL_DATA);

  /* restore the GS_BASE and GS_KERNEL_BASE MSR */
  msr_write(MSR_GS_BASE, gs_base);
  msr_write(MSR_GS_KERNEL_BASE, gs_base);
}
