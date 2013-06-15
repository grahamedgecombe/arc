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

#include <arc/proc/idle.h>
#include <arc/proc/proc.h>
#include <arc/cpu/halt.h>
#include <arc/smp/cpu.h>
#include <arc/cpu/gdt.h>
#include <arc/cpu/cr.h>
#include <arc/cpu/flags.h>
#include <arc/util/container.h>
#include <arc/panic.h>

static proc_t *idle_proc;

void idle_init(void)
{
  /*
   * Keep the old PML4 table pointer and restore it later.
   *
   * This is a hacky solution to a problem where the SMP boot code maps a page
   * in lower memory containing the bootstrap code in the address space of the
   * idle process. The APs start up using the bootstrap address space, which
   * does not contain this mapping, causing a page fault.
   */
  uintptr_t old_pml4_table = cr3_read();

  idle_proc = proc_create();
  if (!idle_proc)
    panic("couldn't create idle process");

  proc_switch(idle_proc);

  list_for_each(&cpu_list, node)
  {
    cpu_t *cpu = container_of(node, cpu_t, node);
    thread_t *thread = thread_create(idle_proc, THREAD_KERNEL);
    if (!thread)
      panic("couldn't create idle thread");

    thread->rip = (uint64_t) &halt_forever;
    proc_thread_add(idle_proc, thread);

    cpu->idle_thread = thread;
  }

  cr3_write(old_pml4_table);
}
