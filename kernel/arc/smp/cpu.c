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

#include <arc/smp/cpu.h>
#include <arc/cpu/msr.h>
#include <arc/pack.h>
#include <stdlib.h>
#include <string.h>

#define KERNEL_STACK_SIZE 8192
#define KERNEL_STACK_ALIGN 16

list_t cpu_list = LIST_EMPTY;

static cpu_t cpu_bsp;
static ALIGN(uint8_t bsp_stack[KERNEL_STACK_SIZE], KERNEL_STACK_ALIGN);

void cpu_bsp_init(void)
{
  memset(&cpu_bsp, 0, sizeof(cpu_bsp));
  cpu_bsp.stack = (uintptr_t) bsp_stack + KERNEL_STACK_SIZE;
  cpu_bsp.self = &cpu_bsp;
  cpu_bsp.bsp = true;
  cpu_bsp.intr_depth = 1;
  cpu_bsp.proc = 0;
  cpu_bsp.thread = 0;
  msr_write(MSR_GS_BASE, (uint64_t) &cpu_bsp);
  msr_write(MSR_GS_KERNEL_BASE, (uint64_t) &cpu_bsp);

  list_add_tail(&cpu_list, &cpu_bsp.node);
}

bool cpu_ap_init(cpu_lapic_id_t lapic_id, cpu_acpi_id_t acpi_id)
{
  cpu_t *cpu = malloc(sizeof(*cpu));
  if (!cpu)
    return false;

  memclr(cpu, sizeof(*cpu));

  void *stack = memalign(KERNEL_STACK_ALIGN, KERNEL_STACK_SIZE);
  if (!stack)
  {
    free(cpu);
    return false;
  }
  cpu->stack = (uintptr_t) stack + KERNEL_STACK_SIZE;
  cpu->self = cpu;
  cpu->lapic_id = lapic_id;
  cpu->acpi_id = acpi_id;
  cpu->intr_depth = 1;
  cpu->proc = 0;
  cpu->thread = 0;

  list_add_tail(&cpu_list, &cpu->node);
  return true;
}

void cpu_ap_install(cpu_t *cpu)
{
  msr_write(MSR_GS_BASE, (uint64_t) cpu);
  msr_write(MSR_GS_KERNEL_BASE, (uint64_t) cpu);
}

