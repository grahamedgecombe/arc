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
#include <stdlib.h>

static bool _uheap_alloc_at(uheap_t *heap, void *ptr, size_t size, int flags)
{
  uintptr_t addr = (uintptr_t) ptr;
  size = PAGE_ALIGN(size);

  list_for_each(&heap->block_list, node)
  {
    uheap_block_t *block = container_of(node, uheap_block_t, node);
    if (addr >= block->start && (addr + size - 1) <= block->end)
    {
      uheap_block_t *left_block = 0, *right_block = 0;

      /* turn the flags into vmm flags */
      uint64_t vmm_flags = PG_USER;
      if (vmm_flags & UHEAP_W)
        vmm_flags |= PG_WRITABLE;
      if (!(vmm_flags & UHEAP_X))
        vmm_flags |= PG_NO_EXEC;

      /* allocate underlying page frames and map the region into memory */
      if (!range_alloc(addr, size, vmm_flags))
        return false;

      /* determine if left and right parts of the block can be split away */
      bool left_split = addr != block->start;
      bool right_split = (addr + size - 1) != block->end;

      /* allocate block node for the left side */
      if (left_split)
      {
        left_block = malloc(sizeof(*left_block));
        if (!left_block)
          return false;
      }

      /* allocate block node for the right side */
      if (right_split)
      {
        right_block = malloc(sizeof(*right_block));
        if (!right_block)
        {
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

        block->start = addr;

        list_insert_before(&heap->block_list, &block->node, &left_block->node);
      }

      /* split the right side of the block away */
      if (right_split)
      {
        right_block->start = addr + size;
        right_block->end = block->end;

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

static void *_uheap_alloc(uheap_t *heap, size_t size, int flags)
{
  return 0;
}

static void _uheap_free(uheap_t *heap, void *ptr)
{

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

  /* set the last user-space address */
  block->end = 0x00007FFFFFFFFFFF;

  /* init the spinlock */
  heap->lock = SPIN_UNLOCKED;

  /* init the block list and add the block to the head */
  list_init(&heap->block_list);
  list_add_head(&heap->block_list, &block->node);
  return true;
}

void uheap_destroy(uheap_t *uheap)
{
  /* TODO destroy the heap */
}

bool uheap_alloc_at(void *ptr, size_t size, int flags)
{
  uheap_t *heap = uheap_get();
  if (!heap)
    return false;

  spin_lock(&heap->lock);
  bool ok = _uheap_alloc_at(heap, ptr, size, flags);
  spin_unlock(&heap->lock);

  return ok;
}

void *uheap_alloc(size_t size, int flags)
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

