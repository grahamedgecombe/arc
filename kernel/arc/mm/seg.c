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

#include <arc/mm/seg.h>
#include <arc/mm/align.h>
#include <arc/mm/common.h>
#include <arc/mm/range.h>
#include <arc/proc/proc.h>
#include <arc/util/container.h>
#include <arc/trace.h>
#include <assert.h>
#include <stdlib.h>

static bool _seg_alloc_at(seg_t *segments, void *ptr, size_t size, vm_acc_t flags)
{
  uintptr_t addr = (uintptr_t) ptr;
  assert((size % FRAME_SIZE) == 0);

  list_for_each(&segments->block_list, node)
  {
    seg_block_t *block = container_of(node, seg_block_t, node);
    if (addr >= block->start && (addr + size - 1) <= block->end)
    {
      seg_block_t *left_block = 0, *right_block = 0;

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
        left_block->state = SEG_FREE;
        block->start = addr;

        list_insert_before(&segments->block_list, &block->node, &left_block->node);
      }

      /* split the right side of the block away */
      if (right_split)
      {
        right_block->start = addr + size;
        right_block->end = block->end;
        right_block->state = SEG_FREE;
        block->end = addr + size - 1;

        list_insert_after(&segments->block_list, &block->node, &right_block->node);
      }

      /* mark this block as allocated */
      block->state = SEG_ALLOCATED;
      block->flags = flags;
      return true;
    }
  }

  return false;
}

static void *_seg_alloc(seg_t *segments, size_t size, vm_acc_t flags)
{
  assert((size % FRAME_SIZE) == 0);

  list_for_each(&segments->block_list, node)
  {
    seg_block_t *block = container_of(node, seg_block_t, node);
    size_t block_size = block->end - block->start + 1;
    if (block->state == SEG_FREE && block_size >= size)
    {
      uintptr_t addr = (uintptr_t) block->start;

      /* allocate underlying page frames and map the region into memory */
      if (!range_alloc(addr, size, flags))
        return 0;

      /* split the right part of the block away */
      if (block_size != size)
      {
        seg_block_t *right_block = malloc(sizeof(*block));
        if (!right_block)
        {
          range_free(addr, size);
          return 0;
        }

        right_block->start = addr + size;
        right_block->end = block->end;
        right_block->state = SEG_FREE;
        block->end = addr + size - 1;

        list_insert_after(&segments->block_list, &block->node, &right_block->node);
      }

      /* mark this block as allocated and return a pointer to it */
      block->state = SEG_ALLOCATED;
      block->flags = flags;
      return (void *) block->start;
    }
  }

  return 0;
}

static void _seg_free(seg_t *segments, void *ptr)
{
  uintptr_t addr = (uintptr_t) ptr;

  list_for_each(&segments->block_list, node)
  {
    seg_block_t *block = container_of(node, seg_block_t, node);
    if (block->state != SEG_FREE && block->start == addr)
    {
      /* free the underlying page frames and unmap the virtual memory */
      size_t block_size = block->end - block->start + 1;
      range_free(addr, block_size);

      /* unmark this block as being allocated */
      block->state = SEG_FREE;

      /* try to merge with the left block */
      if (block->node.prev)
      {
        seg_block_t *left_block = container_of(block->node.prev, seg_block_t, node);
        if (left_block->state == SEG_FREE)
        {
          block->start = left_block->start;

          list_remove(&segments->block_list, &left_block->node);
          free(left_block);
        }
      }

      /* try to merge with the right block */
      if (block->node.next)
      {
        seg_block_t *right_block = container_of(block->node.next, seg_block_t, node);
        if (right_block->state == SEG_FREE)
        {
          block->end = right_block->end;

          list_remove(&segments->block_list, &right_block->node);
          free(right_block);
        }
      }

      return;
    }
  }
}

static seg_t *seg_get(void)
{
  proc_t *proc = proc_get();
  if (!proc)
    return 0;

  return &proc->segments;
}

bool seg_init(seg_t *segments)
{
  /* allocate head block */
  seg_block_t *block = malloc(sizeof(*block));
  if (!block)
    return false;

  /* set the first last user-space address */
  block->start = 0x1000; /* so NULL pointer isn't included */
  block->end = VM_USER_END;
  block->state = SEG_FREE;

  /* init the spinlock */
  segments->lock = SPIN_UNLOCKED;

  /* init the block list and add the block to the head */
  list_init(&segments->block_list);
  list_add_head(&segments->block_list, &block->node);
  return true;
}

void seg_destroy(void)
{
  seg_t *segments = seg_get();

  /* lock the seg */
  spin_lock(&segments->lock);

  /* iterate through every block in this seg */
  list_for_each(&segments->block_list, node)
  {
    seg_block_t *block = container_of(node, seg_block_t, node);

    /*
     * free the virtual and physical memory used by the block, if it is
     * allocated
     */
    if (block->state != SEG_FREE)
    {
      size_t block_size = block->end - block->start + 1;
      range_free((uintptr_t) block->start, block_size);
    }

    /*
     * remove the block from the list and free the memory the kernel uses to
     * keep track of it
     */
    list_remove(&segments->block_list, node);
    free(block);
  }

  /* a sanity check to ensure we really have emptied the seg */
  assert(segments->block_list.size == 0);

  /*
   * release the lock, any further operations on the seg _will_ fail as it is
   * empty, so it can be safely free()ed by the proc code any time after this
   * function was called, even though we are releasing the lock so there is a
   * potential for it to be used again in a very small period of time
   */
  spin_unlock(&segments->lock);
}

bool seg_alloc_at(void *ptr, size_t size, vm_acc_t flags)
{
  seg_t *segments = seg_get();
  if (!segments)
    return false;

  spin_lock(&segments->lock);
  bool ok = _seg_alloc_at(segments, ptr, size, flags);
  spin_unlock(&segments->lock);

  return ok;
}

void *seg_alloc(size_t size, vm_acc_t flags)
{
  seg_t *segments = seg_get();
  if (!segments)
    return 0;

  spin_lock(&segments->lock);
  void *ptr = _seg_alloc(segments, size, flags);
  spin_unlock(&segments->lock);

  return ptr;
}

void seg_free(void *ptr)
{
  seg_t *segments = seg_get();
  if (segments)
  {
    spin_lock(&segments->lock);
    _seg_free(segments, ptr);
    spin_unlock(&segments->lock);
  }
}

void seg_trace(void)
{
  seg_t *segments = seg_get();
  if (segments)
  {
    spin_lock(&segments->lock);

    trace_printf("Tracing user segments...\n");
    list_for_each(&segments->block_list, node)
    {
      seg_block_t *block = container_of(node, seg_block_t, node);
      const char *state = block->state == SEG_ALLOCATED ? "allocated " : "free";
      const char *r = "", *w = "", *x = "";
      if (block->state == SEG_ALLOCATED)
      {
        r = block->flags & VM_R ? "r" : "-";
        w = block->flags & VM_W ? "w" : "-";
        x = block->flags & VM_X ? "x" : "-";
      }
      trace_printf(" => %0#18x -> %0#18x (%s%s%s%s)\n", block->start, block->end, state, r, w, x);
    }

    spin_unlock(&segments->lock);
  }
}

