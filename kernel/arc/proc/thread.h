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

#ifndef ARC_PROC_THREAD_H
#define ARC_PROC_THREAD_H

#include <arc/lock/spinlock.h>
#include <arc/util/list.h>
#include <stdlib.h>
#include <stdint.h>

#define THREAD_KERNEL 0x1 /* flag to indicate the thread runs in kernel mode */

typedef enum
{
  THREAD_RUNNABLE,
  THREAD_RUNNING,
  THREAD_SUSPENDED,
  THREAD_ZOMBIE
} thread_state_t;

typedef struct
{
  /*
   * kernel stack for this thread. syscall_stub() relies on this being the
   * first 8 bytes
   */
  uint64_t kernel_rsp;

  /*
   * value of rsp to restore when leaving syscall_stub(). must be the second
   * group of 8 bytes
   */
  uint64_t syscall_rsp;

  /* base of the stacks (only used upon thread_destroy) */
  void *kstack, *stack;

  /* flags the thread was created with (ditto) */
  int flags;

  /* spinlock used to protect concurrent access to this structure */
  spinlock_t lock;

  /* state */
  thread_state_t state;

  /* node used by proc_t's thread_list */
  list_node_t proc_node;

  /* node used by scheduler's queue */
  list_node_t sched_node;

  /* process which 'owns' this thread */
  struct proc *proc;

  /* register file for this thread */
  uint64_t regs[15];
  uint64_t rip, rsp, rflags;
  uint64_t cs, ss;
} thread_t;

thread_t *thread_create(struct proc *proc, int flags);
thread_t *thread_get(void);
void thread_suspend(thread_t *thread);
void thread_resume(thread_t *thread);
void thread_kill(thread_t *thread);
void thread_destroy(thread_t *thread);

#endif
