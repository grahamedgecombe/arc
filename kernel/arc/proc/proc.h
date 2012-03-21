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

#ifndef ARC_PROC_PROC_H
#define ARC_PROC_PROC_H

#include <arc/proc/thread.h>
#include <arc/lock/spinlock.h>
#include <arc/util/list.h>
#include <stdint.h>

typedef struct
{
  /* physical address of the pml4 table of this process */
  uintptr_t pml4_table;

  /* vmm address space lock */
  spinlock_t vmm_lock;

  /* list of threads in this process */
  list_t thread_list;
} proc_t;

proc_t *proc_create(void);
proc_t *proc_get(void);
void proc_destroy(proc_t *proc);

#endif

