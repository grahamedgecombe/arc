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

#ifndef ARC_LOCK_RWLOCK_H
#define ARC_LOCK_RWLOCK_H

#include <arc/lock/spinlock.h>
#include <stdbool.h>

#define RWLOCK_UNLOCKED { .read_permits = 0, .writing = false, .lock = SPIN_UNLOCKED }

typedef struct
{
  /* The number of threads which have acquired the lock in read-only mode. */
  int read_permits;

  /* A flag indicating if this lock has been acquired in read-write mode. */
  bool writing;

  /*
   * The spinlock that allows atomic operations to be performed on the above
   * fields.
   */
  spinlock_t lock;
} rwlock_t;

/*
 * Acquires the given read-write lock in read-only mode. If the lock is
 * unlocked, or if the lock has already been acquired for reading, it is
 * acquired immediately.
 *
 * Otherwise, this code will block by spinning until the write lock is
 * released, then acquire the read lock.
 *
 * Once acquired, interrupts are also disabled, so this lock is safe to be
 * used inside an interrupt handler.
 *
 * Obtaining the lock in read-only mode is re-entrant, unlike write-only mode.
 */
void rw_rlock(rwlock_t *lock);

/* Releases the read lock. */
void rw_runlock(rwlock_t *lock);

/*
 * Acquires the given read-write lock in read-write mode. This code blocks by
 * spinning until the lock is not locked in either read-only nor read-write
 * mode, and then acquires the lock. Interrupts are disabled upon the lock
 * being acquired, as described above.
 */
void rw_wlock(rwlock_t *lock);

/* Releases the write lock. */
void rw_wunlock(rwlock_t *lock);

#endif
