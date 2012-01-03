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

#include <arc/mm/pmm.h>
#include <arc/mm/align.h>
#include <arc/lock/spinlock.h>
#include <arc/cpu/tlb.h>
#include <arc/mm/common.h>
#include <arc/multiboot.h>
#include <arc/tty.h>
#include <arc/pack.h>
#include <string.h>

/* where the stacks start in virtual memory */
#define VM_STACK_OFFSET 0xFFFFFF7EFFFFF000

/* where the page table containing the stack pages is in virtual memory */
#define PAGE_TABLE_OFFSET 0xFFFFFFFFBF7FF000

/* the 'guard stack' which is at the end of the stack of stacks */
static ALIGN(pmm_stack_t guard_stack, FRAME_SIZE);

/* pointers to where the current stack in virtual memory */
static pmm_stack_t *stack = (pmm_stack_t *) VM_STACK_OFFSET;

/* the spinlock */
static spinlock_t lock;

/* sets the physical address to the current stack and returns the old one */
static uintptr_t stack_set_phy(uintptr_t addr)
{
  uintptr_t *table = (uintptr_t *) PAGE_TABLE_OFFSET;
  uintptr_t old_addr = table[511] & PG_ADDR_MASK;

  table[511] = addr | PG_PRESENT | PG_WRITABLE | PG_NO_EXEC;
  tlb_invlpg(VM_STACK_OFFSET);

  return old_addr;
}

/* initialises the physical memory manager */
void pmm_init(mm_map_t *map)
{
  /* reset the guard stack */
  memset(&guard_stack, 0, sizeof(guard_stack));

  /* set the physical address to the current stack */
  stack_set_phy(((uintptr_t) &guard_stack) - VM_OFFSET);

  /* a counter of the pages for diagnostics */
  uint64_t count = 0;

  /* look through the available memory regions */
  for (int i = 0; i < map->count; i++)
  {
    mm_map_entry_t *entry = &map->entries[i];
    if (entry->type != MULTIBOOT_MMAP_AVAILABLE)
      continue;

    /* align the start and end addresses */
    uintptr_t start = PAGE_ALIGN(entry->addr_start);
    uintptr_t end = PAGE_ALIGN_REVERSE(entry->addr_end + 1);

    /* add these pages to the stacks */
    for (uintptr_t addr = start; addr < end; addr += FRAME_SIZE, count++)
      pmm_free((void *) addr);
  }

  /* print a diagnostic message */
  tty_printf(" => %d physical memory frames available\n", count);
}

void *pmm_alloc(void)
{
  void *ptr;
  spin_lock(&lock);

  /* switch to the next stack if the current one is empty */
  if (stack->count == 0)
  {
    /* check if we've reached the end of the guard stack */
    if (!stack->next_stack)
      ptr = 0;
    else
      ptr = (void *) stack_set_phy(stack->next_stack);
  }
  else
  {
    /* pop from this stack */
    ptr = (void *) stack->stack[--stack->count];
  }

  spin_unlock(&lock);
  return ptr;
}

void pmm_free(void *ptr)
{
  spin_lock(&lock);

  /* check if this stack is full */
  if (stack->count == PMM_STACK_SIZE)
  {
    /* use the page being freed as a new stack */
    uintptr_t cur_stack = stack_set_phy((uintptr_t) ptr);
    stack->next_stack = cur_stack;
    stack->count = 0;
  }
  else
  {
    /* push to this stack */
    stack->stack[stack->count++] = (uintptr_t) ptr;
  }

  spin_unlock(&lock);
}

