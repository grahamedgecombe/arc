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

#ifndef ARC_SMP_CPU_H
#define ARC_SMP_CPU_H

#include <arc/cpu/gdt.h>
#include <arc/cpu/tss.h>
#include <arc/proc/proc.h>
#include <arc/proc/thread.h>
#include <arc/util/list.h>
#include <arc/types.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct cpu
{
  /*
   * cpu_get() relies on the fact that the first 8 bytes in the structure
   * points to itself
   */
  struct cpu *self;

  /*
   * the kernel stack for this cpu (used in interrupt & syscalls), do not move
   * as the syscall entry point relies on the fact that the second 8 bytes in
   * the structure point to a valid kernel stack
   */
  uintptr_t stack;

  /*
   * the user stack pointer is saved here during a SYSCALL, again, this must
   * not be moved
   */
  uintptr_t user_stack;

  /* current 'depth' of the interrupt lock, also must not be moved */
  uint64_t intr_depth;

  /* global CPU list node */
  list_node_t node;

  /* BSP flag */
  bool bsp;

  /* the local APIC and ACPI ids of this processor */
  cpu_lapic_id_t lapic_id;
  cpu_acpi_id_t acpi_id;

  /* the gdt and gdtr for this processor */
  gdtr_t gdtr;
  gdt_gate_t gdt_gates[GDT_GATES];

  /* the tss for this processor */
  tss_t tss;

  /* current process/thread running on this cpu */
  proc_t *proc;
  thread_t *thread;

  /* number of APIC ticks per millisecond */
  uint32_t apic_ticks_per_ms;

  /* flags indicating if LINTn should be programmed as NMIs */
  bool apic_lint_nmi[2];
} cpu_t;

extern list_t cpu_list;

void cpu_bsp_init(void);
bool cpu_ap_init(cpu_lapic_id_t lapic_id, cpu_acpi_id_t acpi_id);
void cpu_ap_install(cpu_t *cpu);
cpu_t *cpu_get(void);

#endif
