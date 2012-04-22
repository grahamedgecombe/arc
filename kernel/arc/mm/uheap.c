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

#include <arc/mm/uheap.h>
#include <arc/proc/proc.h>

static bool _uheap_alloc_at(uheap_t *heap, void *ptr, size_t size, int flags)
{
  return false;
}

static void *_uheap_alloc(uheap_t *heap, size_t size, int flags)
{
  return 0;
}

static void _uheap_free(uheap_t *heap, void *ptr)
{

}

bool uheap_init(uheap_t *heap)
{
  heap->lock = SPIN_UNLOCKED;
  return true;
}

bool uheap_alloc_at(void *ptr, size_t size, int flags)
{
  proc_t *proc = proc_get();
  if (!proc)
    return false;

  uheap_t *heap = &proc->heap;

  spin_lock(&heap->lock);
  bool ok = _uheap_alloc_at(heap, ptr, size, flags);
  spin_unlock(&heap->lock);

  return ok;
}

void *uheap_alloc(size_t size, int flags)
{
  proc_t *proc = proc_get();
  if (!proc)
    return 0;

  uheap_t *heap = &proc->heap;

  spin_lock(&heap->lock);
  void *ptr = _uheap_alloc(heap, size, flags);
  spin_unlock(&heap->lock);

  return ptr;
}

void uheap_free(void *ptr)
{
  proc_t *proc = proc_get();
  if (!proc)
    return;

  uheap_t *heap = &proc->heap;

  spin_lock(&heap->lock);
  _uheap_free(heap, ptr);
  spin_unlock(&heap->lock);
}

