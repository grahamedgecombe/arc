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

#include <arc/proc/thread.h>
#include <arc/cpu/flags.h>
#include <arc/cpu/gdt.h>
#include <arc/smp/cpu.h>
#include <arc/mm/seg.h>
#include <stdlib.h>

#define THREAD_STACK_SIZE 8192

thread_t *thread_create(proc_t *proc)
{
  thread_t *thread = malloc(sizeof(*thread));
  if (!thread)
    return 0;

  void *stack = seg_alloc(THREAD_STACK_SIZE, VM_R | VM_W);
  if (!stack)
  {
    free(thread);
    return 0;
  }

  thread->proc = proc;
  thread->rsp = (uintptr_t) stack + THREAD_STACK_SIZE;
  thread->rflags = FLAGS_IOPL3 | FLAGS_IF;
  thread->cs = SLTR_USER_CODE | RPL3;
  thread->ss = SLTR_USER_DATA | RPL3;
  return thread;
}

thread_t *thread_get(void)
{
  cpu_t *cpu = cpu_get();
  return cpu->thread;
}
