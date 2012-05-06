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
#include <arc/smp/mode.h>
#include <arc/cpu/gdt.h>
#include <arc/cpu/tss.h>
#include <arc/cpu/idt.h>
#include <arc/cpu/pause.h>
#include <arc/cpu/halt.h>
#include <arc/cpu/tlb.h>
#include <arc/lock/intr.h>
#include <arc/lock/spinlock.h>
#include <arc/intr/apic.h>
#include <arc/mm/vmm.h>
#include <arc/time/pit.h>
#include <arc/proc/sched.h>
#include <arc/proc/syscall.h>
#include <arc/util/container.h>
#include <arc/tty.h>
#include <arc/panic.h>
#include <stdlib.h>
#include <string.h>

#define TRAMPOLINE_BASE 0x1000
#define IDLE_STACK_SIZE  8192
#define IDLE_STACK_ALIGN 16

/* some variables used to exchange data between the BSP and APs */
static volatile bool ack_sipi = false;
static cpu_t * volatile booted_cpu;

/* a counter of ready CPUs, smp_init() blocks until all APs are ready */
static int ready_cpus = 1;
static spinlock_t ready_cpus_lock = SPIN_UNLOCKED;

/* a flag used to indicate that the system is now in SMP mode */
static bool mode_switched = false;
static spinlock_t mode_switch_lock = SPIN_UNLOCKED;

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
  if (!vmm_map_range(TRAMPOLINE_BASE, TRAMPOLINE_BASE, trampoline_len, VM_R | VM_W | VM_X))
    panic("couldn't map SMP trampoline code");

  /* allocate a stack for this AP */
  void *idle_stack = memalign(IDLE_STACK_ALIGN, IDLE_STACK_SIZE);
  if (!idle_stack)
    panic("couldn't allocate AP stack");

  /* set up this cpu's bootstrap stack */
  uint64_t *rsp = (uint64_t *) &trampoline_stack;
  *rsp = (uint64_t) idle_stack + IDLE_STACK_SIZE;

  /* set the pointer to the cpu struct of the cpu we are booting */
  booted_cpu = cpu;

  /* copy the trampoline into low memory */
  memcpy((void *) TRAMPOLINE_BASE, &trampoline_start, trampoline_len);

  /* reset the ack flag */
  ack_sipi = false;
 
  /* send INIT IPI */
  apic_ipi_init(cpu->lapic_id);
  pit_mdelay(10);

  /* send STARTUP IPI */
  uint8_t vector = TRAMPOLINE_BASE / FRAME_SIZE;
  apic_ipi_startup(cpu->lapic_id, vector);
  pit_mdelay(1);

  /* send STARTUP IPI again */
  if (!ack_sipi)
  {
    apic_ipi_startup(cpu->lapic_id, vector);
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
  list_for_each(&cpu_list, node)
  {
    cpu_t *cpu = container_of(node, cpu_t, node);

    if (!cpu->bsp)
      smp_boot(cpu);
    else
      print_cpu_info(cpu);
  }

  /* wait for all CPUs to be ready */
  int ready;
  do
  {
    spin_lock(&ready_cpus_lock);
    ready = ready_cpus;
    spin_unlock(&ready_cpus_lock);
  } while (ready != cpu_list.size);

  /* switch to SMP mode */
  smp_mode = MODE_SMP;

  /* now let the APs carry on */
  spin_lock(&mode_switch_lock);
  mode_switched = true;
  spin_unlock(&mode_switch_lock);
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

  /* now start the real work! - set up the GDT, TSS, BSP and SYSCALL/RET */
  gdt_init();
  tss_init();
  idt_ap_init(); /* we re-use the same IDT for every CPU */
  syscall_init();

  /* set up the local APIC on this CPU */
  apic_init();

  /* flush the TLB (as up until this point we won't have received TLB shootdowns) */
  tlb_flush();

  /* increment the ready counter */
  spin_lock(&ready_cpus_lock);
  ready_cpus++;
  spin_unlock(&ready_cpus_lock);

  /* wait for the switch to SMP mode */
  bool ready;
  do
  {
    spin_lock(&mode_switch_lock);
    ready = mode_switched;
    spin_unlock(&mode_switch_lock);
  } while (!ready);

  /* set up the scheduler */
  sched_init();

  /* enable interrupts and halt forever */
  intr_unlock();
  halt_forever();
}

