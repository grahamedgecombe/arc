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

#include <arc/proc/proc.h>
#include <arc/smp/cpu.h>
#include <arc/mm/pmm.h>
#include <arc/mm/vmm.h>
#include <stdlib.h>

proc_t *proc_create(void)
{
  proc_t *proc = malloc(sizeof(*proc));
  if (!proc)
    return 0;

  proc->pml4_table = pmm_alloc();
  if (!proc->pml4_table)
  {
    free(proc);
    return 0;
  }

  if (!vmm_init_pml4(proc->pml4_table))
  {
    pmm_free(proc->pml4_table);
    free(proc);
    return 0;
  }

  proc->vmm_lock = SPIN_UNLOCKED;

  if (!uheap_init(&proc->heap))
  {
    pmm_free(proc->pml4_table);
    free(proc);
    return 0;
  }

  list_init(&proc->thread_list);
  return proc;
}

proc_t *proc_get(void)
{
  cpu_t *cpu = cpu_get();
  return cpu->proc;
}

void proc_destroy(proc_t *proc)
{
  // TODO: destroy threads within the process
  pmm_free(proc->pml4_table);
  free(proc);
}

