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

#include <arc/init.h>
#include <arc/trace.h>
#include <arc/cmdline.h>
#include <arc/mm/map.h>
#include <arc/mm/phy32.h>
#include <arc/mm/pmm.h>
#include <arc/mm/vmm.h>
#include <arc/mm/heap.h>
#include <arc/mm/tlb.h>
#include <arc/bus/isa.h>
#include <arc/cpu/features.h>
#include <arc/cpu/gdt.h>
#include <arc/cpu/tss.h>
#include <arc/cpu/idt.h>
#include <arc/cpu/halt.h>
#include <arc/intr/apic.h>
#include <arc/intr/pic.h>
#include <arc/intr/route.h>
#include <arc/intr/fault.h>
#include <arc/intr/nmi.h>
#include <arc/panic.h>
#include <arc/smp/cpu.h>
#include <arc/acpi/scan.h>
#include <arc/smp/init.h>
#include <arc/proc/sched.h>
#include <arc/proc/syscall.h>
#include <arc/proc/module.h>
#include <arc/proc/idle.h>
#include <arc/lock/intr.h>
#include <string.h>
#include <stdbool.h>

static void print_banner(void)
{
  const char *banner = "Arc Operating System";
  int width = 80;
  int gap_len = width / 2 - strlen(banner) / 2;

  char dashes[width + 1], gap[gap_len + 1];

  memset(dashes, '-', width);
  dashes[width] = 0;

  for (int i = 0; i < gap_len; i++)
    gap[i] = ' ';
  gap[gap_len] = 0;

  trace_printf("%s%s%s%s%s", dashes, gap, banner, gap, dashes);
}

void init(uint32_t magic, multiboot_t *multiboot)
{
  /* convert physical 32-bit multiboot address to virtual address */
  multiboot = phy32_to_virt(multiboot);

  /* set up the BSP's percpu structure */
  cpu_bsp_init();

  /* parse command line arguments */
  cmdline_init(multiboot);

  /* initialise tracing */
  trace_init();

  /* print banner */
  print_banner();

  /* check the multiboot magic number */
  if (magic != MULTIBOOT_MAGIC)
    panic("invalid multiboot magic (expected %0#10x, got %0#10x)", MULTIBOOT_MAGIC,
      magic);

  /* set up the GDT, TSS, IDT and SYSCALL/RET */
  gdt_init();
  tss_init();
  idt_bsp_init();
  syscall_init();

  /* scan CPU features */
  cpu_features_init();

  /* map physical memory */
  trace_puts("Mapping physical memory...\n");
  list_t *map = mm_map_init(multiboot);

  /* set up the physical memory manager */
  trace_puts("Setting up the physical memory manager...\n");
  pmm_init(map);

  /* set up the virtual memory manager */
  trace_puts("Setting up the virtual memory manager...\n");
  vmm_init();

  /* set up the heap */
  trace_puts("Setting up the heap...\n");
  heap_init();

  /* init ISA bus */
  isa_init();

  /* uniprocessor fallback flag */
  bool up_fallback = false;

  /* search for ACPI tables */
  trace_puts("Scanning ACPI tables...\n");
  if (!acpi_scan())
  {
    /* fall back to non-SMP mode using the PIC */
    trace_puts("Falling back to single processor mode...\n");
    pic_init();
    up_fallback = true;
  }

  /* set up the local APIC on the BSP if we are in SMP mode */
  if (!up_fallback)
    apic_init();

  /* enable interrupts now the IDT and interrupt controllers are set up */
  intr_unlock();

  /* route IPIs */
  panic_init();
  fault_init();
  tlb_init();

  /* set up idle process, this must be done before we are in SMP mode */
  idle_init();

  /* set up symmetric multi-processing */
  if (!up_fallback)
  {
    trace_puts("Setting up SMP...\n");
    smp_init();
  }

  /* set up NMI routing, this must be done when we are in SMP mode */
  nmi_init();

  /* set up the scheduler, also needs SMP mode */
  sched_init();

  /* set up modules */
  module_init(multiboot);

  /* halt forever - the scheduler will take over from here */
  halt_forever();
}
