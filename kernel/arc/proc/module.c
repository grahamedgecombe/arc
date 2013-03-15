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

#include <arc/proc/module.h>
#include <arc/cpu/cr.h>
#include <arc/lock/intr.h>
#include <arc/proc/proc.h>
#include <arc/proc/elf64.h>
#include <arc/proc/sched.h>
#include <arc/mm/phy32.h>
#include <arc/panic.h>
#include <stddef.h>

static void module_load(multiboot_tag_t *tag)
{
  /* calculate size and phy32 pointer */
  size_t size = tag->module.mod_end - tag->module.mod_start;
  elf64_ehdr_t *elf = (elf64_ehdr_t *) aphy32_to_virt(tag->module.mod_start);

  /* make a new process */
  proc_t *proc = proc_create();
  if (!proc)
    panic("couldn't create process for module");

  /* switch our address space */
  proc_switch(proc);

  /* load the ELF file */
  if (!elf64_load(elf, size))
    panic("couldn't load elf64 file");

  /* make a new thread */
  thread_t *thread = thread_create(proc);
  if (!thread)
    panic("couldn't create thread for module");

  thread->rip = elf->e_entry;
  proc_thread_add(proc, thread);

  /* add thread to the scheduler's ready queue */
  sched_thread_ready(thread);
}

void module_init(multiboot_t *multiboot)
{
  /*
   * disable interrupts, module loading messes around with address space
   * switches so we don't want to confuse the scheduler
   */
  intr_lock();

  /* keep a copy of the old process */
  proc_t *old_proc = proc_get();

  multiboot_tag_t *tag = multiboot_get(multiboot, MULTIBOOT_TAG_MODULE);
  while (tag)
  {
    module_load(tag);
    tag = multiboot_get_after(multiboot, tag, MULTIBOOT_TAG_MODULE);
  }

  /* switch back to the correct address space */
  if (old_proc)
    proc_switch(old_proc);

  /* enable interrupts again */
  intr_unlock();
}
