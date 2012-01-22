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

#include <arc/smp/init.h>
#include <arc/smp/cpu.h>
#include <arc/cpu/gdt.h>
#include <arc/cpu/tss.h>
#include <arc/cpu/idt.h>
#include <arc/cpu/pause.h>
#include <arc/intr/ic.h>
#include <arc/mm/vmm.h>
#include <arc/time/pit.h>
#include <arc/mp.h>
#include <arc/tty.h>
#include <arc/panic.h>
#include <stdlib.h>
#include <string.h>

#define TRAMPOLINE_BASE 0x1000
#define AP_STACK_SIZE  8192
#define AP_STACK_ALIGN 16

/* some variables used to exchange data between the BSP and APs */
static volatile bool ack_sipi = false;
static cpu_t * volatile booted_cpu;

static void print_cpu_info(cpu_t *cpu)
{
  const char *str = cpu->bsp ? ", bsp" : "";
  tty_printf(" => CPU (struct at %0#18x, id %0#10x%s)\n", cpu, cpu->lapic_id, str);
}

/* bring up an AP */
static void smp_boot(cpu_t *cpu)
{
  /* figure out where the trampoline is */
  extern int trampoline_start, trampoline_end, trampoline_stack;
  size_t trampoline_len = (uintptr_t) &trampoline_end - (uintptr_t) &trampoline_start;

  /* map the trampoline into low memory */
  if (!vmm_map_range(TRAMPOLINE_BASE, TRAMPOLINE_BASE, trampoline_len, PG_WRITABLE))
    panic("couldn't map SMP trampoline code");

  /* allocate a stack for this AP */
  void *ap_stack = memalign(AP_STACK_ALIGN, AP_STACK_SIZE);
  if (!ap_stack)
    panic("couldn't allocate AP stack");

  /* set up this cpu's bootstrap stack */
  uint64_t *rsp = (uint64_t *) &trampoline_stack;
  *rsp = (uint64_t) ap_stack + AP_STACK_SIZE;

  /* set the pointer to the cpu struct of the cpu we are booting */
  booted_cpu = cpu;

  /* copy the trampoline into low memory */
  memcpy((void *) TRAMPOLINE_BASE, &trampoline_start, trampoline_len);

  /* reset the ack flag */
  ack_sipi = false;
 
  /* send INIT IPI */
  ic_ipi_init(cpu->lapic_id);
  pit_mdelay(10);

  /* send STARTUP IPI */
  uint8_t vector = TRAMPOLINE_BASE / FRAME_SIZE;
  ic_ipi_startup(cpu->lapic_id, vector);
  pit_mdelay(1);

  /* send STARTUP IPI again */
  if (!ack_sipi)
  {
    ic_ipi_startup(cpu->lapic_id, vector);
    pit_mdelay(1);
  }

  /* wait for the AP to come up */
  while (!ack_sipi)
    pause_once();

  /* unmap the trampoline */
  vmm_unmap_range(TRAMPOLINE_BASE, trampoline_len);
}

void smp_init(void)
{
  /* bring up all of the APs */
  for (cpu_t *cpu = cpu_iter(); cpu; cpu = cpu->next)
  {
    if (!cpu->bsp)
      smp_boot(cpu);
    else
      print_cpu_info(cpu);
  }
}

void smp_ap_init(void)
{
  /* save the per-cpu data area pointer so we can ack the SIPI straight away */
  cpu_ap_install(booted_cpu);

  /* print a message to indicate the AP has been booted */
  cpu_t *cpu = cpu_get();
  print_cpu_info(cpu);

  /* acknowledge the STARTUP IPI */
  ack_sipi = true;

  /* now start the real work! - set up the GDT, TSS and BSP */
  gdt_init();
  tss_init();
  idt_ap_init(); /* we re-use the same IDT for every CPU */

  /* set up the interrupt controller on this CPU */
  ic_ap_init();
}

