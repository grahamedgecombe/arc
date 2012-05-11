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
#include <arc/mm/align.h>
#include <arc/mm/common.h>
#include <arc/mm/range.h>
#include <arc/proc/proc.h>
#include <arc/util/container.h>
#include <arc/tty.h>
#include <assert.h>
#include <stdlib.h>

static bool _uheap_alloc_at(uheap_t *heap, void *ptr, size_t size, vm_acc_t flags)
{
  uintptr_t addr = (uintptr_t) ptr;
  assert((size % FRAME_SIZE) == 0);

  list_for_each(&heap->block_list, node)
  {
    uheap_block_t *block = container_of(node, uheap_block_t, node);
    if (addr >= block->start && (addr + size - 1) <= block->end)
    {
      uheap_block_t *left_block = 0, *right_block = 0;

      /* allocate underlying page frames and map the region into memory */
      if (!range_alloc(addr, size, flags))
        return false;

      /* determine if left and right parts of the block can be split away */
      bool left_split = addr != block->start;
      bool right_split = (addr + size - 1) != block->end;

      /* allocate block node for the left side */
      if (left_split)
      {
        left_block = malloc(sizeof(*left_block));
        if (!left_block)
        {
          range_free(addr, size);
          return false;
        }
      }

      /* allocate block node for the right side */
      if (right_split)
      {
        right_block = malloc(sizeof(*right_block));
        if (!right_block)
        {
          range_free(addr, size);
          if (left_split)
            free(left_block);
          return false;
        }
      }

      /* split the left side of the block away */
      if (left_split)
      {
        left_block->start = block->start;
        left_block->end = addr - 1;
        left_block->allocated = false;
        block->start = addr;

        list_insert_before(&heap->block_list, &block->node, &left_block->node);
      }

      /* split the right side of the block away */
      if (right_split)
      {
        right_block->start = addr + size;
        right_block->end = block->end;
        right_block->allocated = false;
        block->end = addr + size - 1;

        list_insert_after(&heap->block_list, &block->node, &right_block->node);
      }

      /* mark this block as allocated */
      block->allocated = true;
      return true;
    }
  }

  return false;
}

static void *_uheap_alloc(uheap_t *heap, size_t size, vm_acc_t flags)
{
  assert((size % FRAME_SIZE) == 0);

  list_for_each(&heap->block_list, node)
  {
    uheap_block_t *block = container_of(node, uheap_block_t, node);
    size_t block_size = block->end - block->start + 1;
    if (!block->allocated && block_size >= size)
    {
      uintptr_t addr = (uintptr_t) block->start;

      /* allocate underlying page frames and map the region into memory */
      if (!range_alloc(addr, size, flags))
        return 0;

      /* split the right part of the block away */
      if (block_size != size)
      {
        uheap_block_t *right_block = malloc(sizeof(*block));
        if (!right_block)
        {
          range_free(addr, size);
          return 0;
        }

        right_block->start = addr + size;
        right_block->end = block->end;
        right_block->allocated = false;
        block->end = addr + size - 1;

        list_insert_after(&heap->block_list, &block->node, &right_block->node);
      }

      /* mark this block as allocated and return a pointer to it */
      block->allocated = true;
      return (void *) block->start;
    }
  }

  return 0;
}

static void _uheap_free(uheap_t *heap, void *ptr)
{
  uintptr_t addr = (uintptr_t) ptr;

  list_for_each(&heap->block_list, node)
  {
    uheap_block_t *block = container_of(node, uheap_block_t, node);
    if (block->allocated && block->start == addr)
    {
      /* free the underlying page frames and unmap the virtual memory */
      size_t block_size = block->end - block->start + 1;
      range_free(addr, block_size);

      /* unmark this block as being allocated */
      block->allocated = false;

      /* try to merge with the left block */
      if (block->node.prev)
      {
        uheap_block_t *left_block = container_of(block->node.prev, uheap_block_t, node);
        if (!left_block->allocated)
        {
          block->start = left_block->start;

          list_remove(&heap->block_list, &left_block->node);
          free(left_block);
        }
      }

      /* try to merge with the right block */
      if (block->node.next)
      {
        uheap_block_t *right_block = container_of(block->node.next, uheap_block_t, node);
        if (!right_block->allocated)
        {
          block->end = right_block->end;

          list_remove(&heap->block_list, &right_block->node);
          free(right_block);
        }
      }

      return;
    }
  }
}

static uheap_t *uheap_get(void)
{
  proc_t *proc = proc_get();
  if (!proc)
    return 0;

  return &proc->heap;
}

bool uheap_init(uheap_t *heap)
{
  /* allocate head block */
  uheap_block_t *block = malloc(sizeof(*block));
  if (!block)
    return false;

  /* set the first last user-space address */
  block->start = 0x1000; /* so NULL pointer isn't included */
  block->end = 0x00007FFFFFFFFFFF;
  block->allocated = false;

  /* init the spinlock */
  heap->lock = SPIN_UNLOCKED;

  /* init the block list and add the block to the head */
  list_init(&heap->block_list);
  list_add_head(&heap->block_list, &block->node);
  return true;
}

void uheap_destroy(void)
{
  /* TODO destroy the heap */
}

bool uheap_alloc_at(void *ptr, size_t size, vm_acc_t flags)
{
  uheap_t *heap = uheap_get();
  if (!heap)
    return false;

  spin_lock(&heap->lock);
  bool ok = _uheap_alloc_at(heap, ptr, size, flags);
  spin_unlock(&heap->lock);

  return ok;
}

void *uheap_alloc(size_t size, vm_acc_t flags)
{
  uheap_t *heap = uheap_get();
  if (!heap)
    return 0;

  spin_lock(&heap->lock);
  void *ptr = _uheap_alloc(heap, size, flags);
  spin_unlock(&heap->lock);

  return ptr;
}

void uheap_free(void *ptr)
{
  uheap_t *heap = uheap_get();
  if (heap)
  {
    spin_lock(&heap->lock);
    _uheap_free(heap, ptr);
    spin_unlock(&heap->lock);
  }
}

void uheap_trace(void)
{
  uheap_t *heap = uheap_get();
  if (heap)
  {
    spin_lock(&heap->lock);

    tty_printf("Tracing user heap...\n");
    list_for_each(&heap->block_list, node)
    {
      uheap_block_t *block = container_of(node, uheap_block_t, node);
      const char *state = block->allocated ? "allocated" : "free";
      tty_printf(" => %0#18x -> %0#18x (%s)\n", block->start, block->end, state);
    }

    spin_unlock(&heap->lock);
  }
}

