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
#include <stdlib.h>
#include <string.h>

static cpu_t cpu_bsp;
static int cpu_cnt = 1;

void cpu_bsp_init(void)
{
  memset(&cpu_bsp, 0, sizeof(cpu_bsp));
  cpu_bsp.self = &cpu_bsp;
  cpu_bsp.bsp = true;
  msr_write(MSR_GS_BASE, (uint64_t) &cpu_bsp);
}

bool cpu_ap_init(cpu_lapic_id_t lapic_id, cpu_acpi_id_t acpi_id)
{
  static cpu_t *cpu = &cpu_bsp;
  if (!cpu)
    return false;

  cpu_t *prev_cpu = cpu;

  cpu = malloc(sizeof(*cpu));
  if (!cpu)
    return false;

  prev_cpu->next = cpu;

  memset(cpu, 0, sizeof(*cpu));

  cpu->self = cpu;
  cpu->lapic_id = lapic_id;
  cpu->acpi_id = acpi_id;
  cpu_cnt++;

  return true;
}

void cpu_ap_install(cpu_t *cpu)
{
  msr_write(MSR_GS_BASE, (uint64_t) cpu);
}

cpu_t *cpu_iter(void)
{
  return &cpu_bsp;
}

int cpu_count(void)
{
  return cpu_cnt;
}

