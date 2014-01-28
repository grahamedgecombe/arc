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
#include <arc/proc/sched.h>
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
  thread->kstack = memalign(STACK_ALIGN, KERNEL_STACK_SIZE);
  if (!thread->kstack)
  {
    free(thread);
    return 0;
  }

  /* allocate user-space stack */
  if (!(flags & THREAD_KERNEL))
  {
    // TODO seg_alloc() means we need to be in the address space of proc
    thread->stack = seg_alloc(USER_STACK_SIZE, VM_R | VM_W);
    if (!thread->stack)
    {
      free(thread->kstack);
      free(thread);
      return 0;
    }
  }

  thread->lock = SPIN_UNLOCKED;
  thread->state = THREAD_SUSPENDED;
  thread->proc = proc;
  thread->flags = flags;
  thread->rsp = (flags & THREAD_KERNEL) ? ((uintptr_t) thread->kstack + KERNEL_STACK_SIZE) : ((uintptr_t) thread->stack + USER_STACK_SIZE);
  thread->kernel_rsp = (uintptr_t) thread->kstack + KERNEL_STACK_SIZE;
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

  /* attach thread to parent process */
  proc_thread_add(proc, thread);

  return thread;
}

thread_t *thread_get(void)
{
  cpu_t *cpu = cpu_get();
  return cpu->thread;
}

void thread_suspend(thread_t *thread)
{
  spin_lock(&thread->lock);
  // TODO what if thread is zombie etc.? what if it is already running?
  thread->state = THREAD_SUSPENDED;
  sched_thread_suspend(thread);
  spin_unlock(&thread->lock);
}

void thread_resume(thread_t *thread)
{
  spin_lock(&thread->lock);
  // TODO as above
  thread->state = THREAD_RUNNING;
  sched_thread_resume(thread);
  spin_unlock(&thread->lock);
}

void thread_kill(thread_t *thread)
{
  spin_lock(&thread->lock);
  // TODO as above

  /* suspend the thread if it is runnable */
  if (thread->state == THREAD_RUNNING)
    sched_thread_suspend(thread);

  /*
   * we need to keep the struct around as it may be referenced in other places
   * (e.g. one thread may be waiting for another to another to deliver a
   * message), therefore we use a zombie state to indicate a process is dead.
   *
   * once the reference count drops to zero the struct can be cleaned up for
   * good.
   */
  thread->state = THREAD_ZOMBIE;

  // TODO we can probably free the user-space stack etc. here?

  spin_unlock(&thread->lock);
}

void thread_destroy(thread_t *thread)
{
  // TODO need to switch address spaces to be able to free the user-space stack

  /* detach thread from parent process */
  proc_thread_remove(thread->proc, thread);

  /* free user-space stack */
  if (!(thread->flags & THREAD_KERNEL))
    seg_free(thread->stack);

  /* free kernel-space stack */
  free(thread->kstack);

  /* free thread structure itself */
  free(thread);
}
