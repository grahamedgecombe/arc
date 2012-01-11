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

#include <arc/mm/heap.h>
#include <arc/lock/spinlock.h>
#include <arc/mm/pmm.h>
#include <arc/mm/vmm.h>
#include <arc/mm/align.h>
#include <arc/pack.h>
#include <arc/panic.h>
#include <stdbool.h>
#include <stdint.h>

/* the states a heap node can be in */
#define HEAP_NODE_FREE      0 /* not allocated */
#define HEAP_NODE_RESERVED  1 /* allocated, physical frames not managed by us */
#define HEAP_NODE_ALLOCATED 2 /* allocated, physical frames managed by us */

typedef PACK(struct heap_node
{
  struct heap_node *next;
  struct heap_node *prev;
  int state;
  uintptr_t start; /* the address of the first page, inclusive */
  uintptr_t end;   /* the address of the last page, exclusive */
}) heap_node_t;

static heap_node_t *heap_root;
static spinlock_t heap_lock;

void heap_init(void)
{
  /* find where the kernel image ends and the heap starts (inclusive) */
  extern int _end;
  uintptr_t heap_start = PAGE_ALIGN_2M((uintptr_t) &_end);

  /* hard coded end of the heap (exclusive) */
  uintptr_t heap_end = VM_STACK_OFFSET;

  /* allocate some space for the root node */
  void *root_phy = pmm_alloc();
  if (!root_phy)
    boot_panic("couldn't allocate physical frame for heap root node");

  /* sanity check which probably seems completely ridiculous */
  if (heap_start >= heap_end)
    boot_panic("no room for heap");

  /* the root node will take the first virtual address */
  heap_root = (heap_node_t *) heap_start;
  vmm_map(heap_start, (uintptr_t) root_phy, PG_WRITABLE | PG_NO_EXEC);

  /* fill out the root node */
  heap_root->next = 0;
  heap_root->prev = 0;
  heap_root->state = HEAP_NODE_FREE;
  heap_root->start = heap_start + FRAME_SIZE;
  heap_root->end = heap_end;
}

static void *_heap_alloc(size_t size, bool phy_alloc)
{
  return 0;
}

static void _heap_free(void *ptr)
{

}

void *heap_reserve(size_t size)
{
  spin_lock(&heap_lock);
  void *ptr = _heap_alloc(size, false);
  spin_unlock(&heap_lock);
  return ptr;
}

void *heap_alloc(size_t size)
{
  spin_lock(&heap_lock);
  void *ptr = _heap_alloc(size, true);
  spin_unlock(&heap_lock);
  return ptr;
}

void heap_free(void *ptr)
{
  spin_lock(&heap_lock);
  _heap_free(ptr);
  spin_unlock(&heap_lock);
}

