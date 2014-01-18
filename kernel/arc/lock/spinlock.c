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

#include <arc/lock/spinlock.h>
#include <arc/cpu/pause.h>
#include <arc/lock/intr.h>
#include <assert.h>

void spin_lock(spinlock_t *lock)
{
  for (;;)
  {
    if (spin_try_lock(lock))
      break;

    pause_once();
  }
}

bool spin_try_lock(spinlock_t *lock)
{
  intr_lock();

  if (__sync_bool_compare_and_swap(lock, SPIN_UNLOCKED, SPIN_LOCKED))
    return true;

  intr_unlock();
  return false;
}

void spin_unlock(spinlock_t *lock)
{
  assert(__sync_bool_compare_and_swap(lock, SPIN_LOCKED, SPIN_UNLOCKED));
  intr_unlock();
}
