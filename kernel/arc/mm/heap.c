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
#include <arc/mm/range.h>
#include <arc/pack.h>
#include <arc/panic.h>
#include <arc/tty.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/* the states a heap node can be in */
#define HEAP_NODE_FREE      0 /* not allocated */
#define HEAP_NODE_RESERVED  1 /* allocated, physical frames not managed by us */
#define HEAP_NODE_ALLOCATED 2 /* allocated, physical frames managed by us */

/* a magic id used to check if requests are valid */
#define HEAP_MAGIC 0x461E7B705515DB7F

typedef PACK(struct heap_node
{
  struct heap_node *next;
  struct heap_node *prev;
  int state;
  uintptr_t start; /* the address of the first byte, inclusive */
  uintptr_t end;   /* the address of the last byte, exclusive */
  uint64_t magic;
}) heap_node_t;

static heap_node_t *heap_root;
static spinlock_t heap_lock = SPIN_UNLOCKED;

void heap_init(void)
{
  /* find where the kernel image ends and the heap starts (inclusive) */
  extern int _end;
  uintptr_t heap_start = PAGE_ALIGN_2M((uintptr_t) &_end);

  /* hard coded end of the heap (exclusive) */
  uintptr_t heap_end = VM_STACK_OFFSET;

  /* allocate some space for the root node */
  uintptr_t root_phy = pmm_alloc();
  if (!root_phy)
    panic("couldn't allocate physical frame for heap root node");

  /* sanity check which probably seems completely ridiculous */
  if (heap_start >= heap_end)
    panic("no room for heap");

  /* the root node will take the first virtual address */
  heap_root = (heap_node_t *) heap_start;
  if (!vmm_map(heap_start, root_phy, VM_R | VM_W))
    panic("couldn't map heap root node into the virtual memory");

  /* fill out the root node */
  heap_root->next = 0;
  heap_root->prev = 0;
  heap_root->state = HEAP_NODE_FREE;
  heap_root->start = heap_start + FRAME_SIZE;
  heap_root->end = heap_end;
  heap_root->magic = heap_root->start ^ HEAP_MAGIC;
}

static heap_node_t *find_node(size_t size)
{
  /* look for the first node that will fit the requested size */
  for (heap_node_t *node = heap_root; node; node = node->next)
  {
    /* skip free nodes */
    if (node->state != HEAP_NODE_FREE)
      continue;

    /* skip nodes that are too small */
    size_t node_size = node->end - node->start;
    if (node_size < size)
      continue;

    /* check if splitting the node would actually leave some space */
    size_t extra_size = node_size - size;
    if (extra_size >= (FRAME_SIZE * 2))
    {
      /* only split the node if we can allocate a physical page */
      uintptr_t phy = pmm_alloc();
      if (phy)
      {
        /* map the new heap_node_t into virtual memory, only split if it works */
        heap_node_t *next = (heap_node_t *) ((uintptr_t) node + size + FRAME_SIZE);
        if (vmm_map((uintptr_t) next, phy, VM_R | VM_W))
        {
          /* fill in the new heap_node_t */
          next->start = (uintptr_t) node + size + FRAME_SIZE * 2;
          next->end = node->end;
          next->state = HEAP_NODE_FREE;
          next->prev = node;
          next->next = node->next;
          next->magic = next->start ^ HEAP_MAGIC;

          /* update the node that was split */
          node->end = (uintptr_t) next;

          /* update the surrounding nodes */
          node->next = next;
          if (next->next)
            next->next->prev = next;
        }
        else
        {
          /* free the unused physical frame */
          pmm_free(phy);
        }
      }
    }

    /* update the state of the allocated node */
    node->state = HEAP_NODE_RESERVED;
    return node;
  }

  return 0;
}

static void _heap_free(void *ptr)
{
  /* find where the node is */
  heap_node_t *node = (heap_node_t *) ((uintptr_t) ptr - FRAME_SIZE);

  /* check if the magic matches to see if we get passed a dodgy pointer */
  assert(node->magic == (node->start ^ HEAP_MAGIC));

  /* free the physical frames if heap_alloc allocated them */
  size_t size = node->end - node->start;
  if (node->state == HEAP_NODE_ALLOCATED)
    range_free(node->start, size);

  /* set the node's state to free */
  node->state = HEAP_NODE_FREE;

  /* try to coalesce with the next node */
  heap_node_t *next = node->next;
  if (next && next->state == HEAP_NODE_FREE)
  {
    /* update the pointers */
    node->next = next->next;
    if (next->next)
      next->next->prev = node;

    /* update the address range */
    node->end = next->end;

    /* unmap and free the physical frame behind the next node */
    pmm_free(vmm_unmap((uintptr_t) next));
  }

  /* try to coalesce with the previous node */
  heap_node_t *prev = node->prev;
  if (prev && prev->state == HEAP_NODE_FREE)
  {
    /* update the pointers */
    prev->next = node->next;
    if (node->next)
      node->next->prev = prev;

    /* update the address range */
    prev->end = node->end;

    /* unmap and free the physical frame behind the next node */
    pmm_free(vmm_unmap((uintptr_t) next));
  }
}

static void *_heap_alloc(size_t size, vm_acc_t flags, bool phy_alloc)
{
  /* round up the size such that it is a multiple of the page size */
  size = PAGE_ALIGN(size);

  /* find a node that can satisfy the size */
  heap_node_t *node = find_node(size);
  if (!node)
    return node;

  if (phy_alloc)
  {
    /* change the state to allocated so heap_free releases the frames */
    node->state = HEAP_NODE_ALLOCATED;

    /* allocate physical frames and map them into memory */
    if (!range_alloc(node->start, size, flags))
      _heap_free((void *) node->start);
  }

  return (void *) ((uintptr_t) node + FRAME_SIZE);
}

void *heap_reserve(size_t size)
{
  spin_lock(&heap_lock);
  void *ptr = _heap_alloc(size, 0, false);
  spin_unlock(&heap_lock);
  return ptr;
}

void *heap_alloc(size_t size, vm_acc_t flags)
{
  spin_lock(&heap_lock);
  void *ptr = _heap_alloc(size, flags, true);
  spin_unlock(&heap_lock);
  return ptr;
}

void heap_free(void *ptr)
{
  spin_lock(&heap_lock);
  _heap_free(ptr);
  spin_unlock(&heap_lock);
}

void heap_trace(void)
{
  spin_lock(&heap_lock);

  tty_printf("Tracing kernel heap...\n");
  for (heap_node_t *node = heap_root; node; node = node->next)
  {
    const char *state = "free";
    if (node->state == HEAP_NODE_RESERVED)
      state = "reserved";
    else if (node->state == HEAP_NODE_ALLOCATED)
      state = "allocated";

    tty_printf(" => %0#18x -> %0#18x (%s)\n", node->start, node->end - 1, state);
  }

  spin_unlock(&heap_lock);
}

