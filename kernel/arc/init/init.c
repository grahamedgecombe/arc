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

#include <arc/init.h>
#include <arc/tty.h>
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
#include <arc/panic.h>
#include <arc/smp/cpu.h>
#include <arc/acpi/scan.h>
#include <arc/mp/scan.h>
#include <arc/smp/init.h>
#include <arc/proc/sched.h>
#include <arc/proc/syscall.h>
#include <arc/proc/module.h>
#include <arc/lock/intr.h>
#include <string.h>
#include <stdbool.h>

static void print_banner(void)
{
  const char *banner = "Arc Operating System";
  int gap_len = TTY_WIDTH / 2 - strlen(banner) / 2;

  char dashes[TTY_WIDTH + 1], gap[gap_len + 1];

  for (int i = 0; i < TTY_WIDTH; i++)
    dashes[i] = '-';
  dashes[TTY_WIDTH] = 0;

  for (int i = 0; i < gap_len; i++)
    gap[i] = ' ';
  gap[gap_len] = 0;

  tty_printf("%s%s%s%s%s", dashes, gap, banner, gap, dashes);
}

void init(uint32_t magic, multiboot_t *multiboot)
{
  /* set up the BSP's percpu structure */
  cpu_bsp_init();

  /* initialise tty driver */
  tty_init();

  /* print banner */
  print_banner();

  /* check the multiboot magic number */
  if (magic != MULTIBOOT_MAGIC)
    panic("invalid multiboot magic (expected %0#10x, got %0#10x)", MULTIBOOT_MAGIC,
      magic);

  /* convert physical 32-bit multiboot address to virtual address */
  multiboot = phy32_to_virt(multiboot);

  /* set up the GDT, TSS, IDT and SYSCALL/RET */
  gdt_init();
  tss_init();
  idt_bsp_init();
  syscall_init();

  /* scan CPU features */
  cpu_features_init();

  /* map physical memory */
  tty_puts("Mapping physical memory...\n");
  mm_map_t *map = mm_map_init(multiboot);

  /* set up the physical memory manager */
  tty_puts("Setting up the physical memory manager...\n");
  pmm_init(map);

  /* set up the virtual memory manager */
  tty_puts("Setting up the virtual memory manager...\n");
  vmm_init();

  /* set up the heap */
  tty_puts("Setting up the heap...\n");
  heap_init();

  /* parse command line arguments now we can malloc */
  cmdline_init(multiboot);

  /* init ISA bus */
  isa_init();

  /* uniprocessor fallback flag */
  bool up_fallback = false;

  /* search for ACPI tables */
  tty_puts("Scanning ACPI tables...\n");
  if (!acpi_scan())
  {
    /* search for MP tables if the PC doesn't support ACPI */
    tty_puts("Scanning MP tables...\n");

    if (!mp_scan())
    {
      /* fall back to non-SMP mode using the PIC */
      tty_puts("Falling back to single processor mode...\n");
      pic_init();
      up_fallback = true;
    }
  }

  /* workaround for a bug in the Bochs ACPI/MP tables */
  if (!up_fallback)
    isa_bochs_workaround();

  /* set up the local APIC on the BSP if we are in SMP mode */
  if (!up_fallback)
    apic_init();

  /* enable interrupts now the IDT and interrupt controllers are set up */
  intr_unlock();

  /* set up interrupt routing */
  panic_init(); /* all of these calls route interrupts */
  fault_init();
  tlb_init();

  /* set up symmetric multi-processing */
  if (!up_fallback)
  {
    tty_puts("Setting up SMP...\n");
    smp_init();
  }

  /* set up the scheduler */
  sched_init();

  /* set up modules */
  module_init(multiboot);

  /* halt forever - the scheduler will take over from here */
  halt_forever();
}

