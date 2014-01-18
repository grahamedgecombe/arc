/*
 * Copyright (c) 2011-2014 Graham Edgecombe <graham@grahamedgecombe.com>
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

#define USER_STACK_SIZE 8192
#define KERNEL_STACK_SIZE 8192
#define STACK_ALIGN 32

thread_t *thread_create(proc_t *proc, int flags)
{
  thread_t *thread = malloc(sizeof(*thread));
  if (!thread)
    return 0;

  /* allocate kernel-space stack */
  void *kstack = memalign(STACK_ALIGN, KERNEL_STACK_SIZE);
  if (!kstack)
  {
    free(thread);
    return 0;
  }

  /* allocate user-space stack */
  void *stack;
  if (!(flags & THREAD_KERNEL))
  {
    stack = seg_alloc(USER_STACK_SIZE, VM_R | VM_W);
    if (!stack)
    {
      free(kstack);
      free(thread);
      return 0;
    }
  }

  thread->proc = proc;
  thread->rsp = (flags & THREAD_KERNEL) ? ((uintptr_t) kstack + KERNEL_STACK_SIZE) : ((uintptr_t) stack + USER_STACK_SIZE);
  thread->kernel_rsp = (uintptr_t) kstack + KERNEL_STACK_SIZE;
  thread->rflags = FLAGS_IF;

  if (flags & THREAD_KERNEL)
  {
    thread->cs = SLTR_KERNEL_CODE | RPL0;
    thread->ss = SLTR_KERNEL_DATA | RPL0;
  }
  else
  {
    thread->rflags |= FLAGS_IOPL3;
    thread->cs = SLTR_USER_CODE | RPL3;
    thread->ss = SLTR_USER_DATA | RPL3;
  }

  return thread;
}

thread_t *thread_get(void)
{
  cpu_t *cpu = cpu_get();
  return cpu->thread;
}
