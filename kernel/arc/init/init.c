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
#include <arc/mm/map.h>
#include <arc/mm/phy32.h>
#include <arc/mm/pmm.h>
#include <arc/mm/vmm.h>
#include <arc/mm/heap.h>
#include <arc/cpu/gdt.h>
#include <arc/cpu/tss.h>
#include <arc/cpu/intr.h>
#include <arc/cpu/idt.h>
#include <arc/intr/ic.h>
#include <arc/intr/route.h>
#include <arc/panic.h>
#include <arc/smp/cpu.h>
#include <arc/acpi/scan.h>
#include <arc/smp/init.h>
#include <arc/proc/syscall.h>
#include <string.h>

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
  /* initialise tty driver */
  tty_init();

  /* print banner */
  print_banner();

  /* check the multiboot magic number */
  if (magic != MULTIBOOT_MAGIC)
    panic("invalid multiboot magic (expected 0x%x, got 0x%x)", MULTIBOOT_MAGIC,
      magic);

  /* convert physical 32-bit multiboot address to virtual address */
  multiboot = phy32_to_virt(multiboot);

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

  /* set up the BSP's percpu structure */
  cpu_bsp_init();

  /* set up the GDT, TSS, IDT and SYSCALL/RET */
  gdt_init();
  tss_init();
  idt_bsp_init();
  syscall_init();

  /* search for ACPI tables */
  tty_puts("Scanning ACPI tables...\n");
  if (!acpi_scan())
  {
    /* search for MP tables if the PC doesn't support ACPI */
    tty_puts("Scanning MP tables...\n");

    /* if (!mp_scan()) */
    {
      /* fall back to non-SMP mode using the PIC */
      tty_puts("Falling back to single processor mode...\n");
      ic_bsp_init(IC_TYPE_PIC);
    }
  }

  /* set up the interrupt controller */
  ic_ap_init();

  /* set up symmetric multi-processing */
  tty_puts("Setting up SMP...\n");
  smp_init();

  /* set up interrupt routing */
  tty_puts("Setting up interrupt routing...\n");
  intr_route_init();

  /* enable interrupts */
  tty_puts("Enabling interrupts...\n");
  intr_enable();
}

