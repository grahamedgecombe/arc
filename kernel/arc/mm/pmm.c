/*
 * Copyright (c) 2011-2013 Graham Edgecombe <graham@grahamedgecombe.com>
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
#include <arc/mm/common.h>
#include <arc/cpu/tlb.h>
#include <arc/lock/spinlock.h>
#include <arc/trace.h>
#include <string.h>

#define PAGE_TABLE_OFFSET 0xFFFFFF7F7F7FF000
#define STACKS (ZONE_COUNT * SIZE_COUNT)
#define PMM_STACK_SIZE (TABLE_SIZE - 2)
#define SZ_TO_IDX(s,z) ((s) * ZONE_COUNT + (z))

typedef struct
{
  uint64_t next;
  uint64_t count;
  uint64_t frames[PMM_STACK_SIZE];
} __attribute__((__packed__)) pmm_stack_t;

static pmm_stack_t pmm_phy_stacks[STACKS] __attribute__((__aligned__(FRAME_SIZE)));
static pmm_stack_t *pmm_stacks = (pmm_stack_t *) VM_STACK_OFFSET;
static uint64_t *pmm_page_table = (uint64_t *) PAGE_TABLE_OFFSET;
static spinlock_t pmm_lock = SPIN_UNLOCKED;
static uint64_t pmm_counts[STACKS];

static const char *get_zone_str(int zone)
{
  switch (zone)
  {
    case ZONE_DMA:
      return "DMA  ";

    case ZONE_DMA32:
      return "DMA32";

    case ZONE_STD:
      return "STD  ";
  }

  return 0;
}

static const char *get_size_str(int size)
{
  switch (size)
  {
    case SIZE_4K:
      return "4K";

    case SIZE_2M:
      return "2M";

    case SIZE_1G:
      return "1G";
  }

  return 0;
}

static int get_zone(int size, uintptr_t addr)
{
  switch (size)
  {
    case SIZE_4K:
      addr += FRAME_SIZE - 1;
      break;

    case SIZE_2M:
      addr += FRAME_SIZE_2M - 1;
      break;

    case SIZE_1G:
      addr += FRAME_SIZE_1G - 1;
      break;
  }

  if (addr <= ZONE_LIMIT_DMA)
    return ZONE_DMA;
  else if (addr <= ZONE_LIMIT_DMA32)
    return ZONE_DMA32;
  else
    return ZONE_STD;
}

static uintptr_t stack_switch(int size, int zone, uintptr_t addr)
{
  int idx = SZ_TO_IDX(size, zone);
  int table_idx = TABLE_SIZE - STACKS + idx;

  uintptr_t old_addr = pmm_page_table[table_idx] & PG_ADDR_MASK;
  pmm_page_table[table_idx] = addr | PG_PRESENT | PG_WRITABLE | PG_NO_EXEC;
  tlb_invlpg(VM_STACK_OFFSET + idx * FRAME_SIZE);

  return old_addr;
}

static void _pmm_free(int size, int zone, uintptr_t addr);

static uintptr_t _pmm_alloc(int size, int zone)
{
  int idx = SZ_TO_IDX(size, zone);
  pmm_stack_t *stack = &pmm_stacks[idx];

  if (stack->count != 0)
  {
    return stack->frames[--stack->count];
  }

  if (stack->next)
  {
    uintptr_t addr = stack_switch(size, zone, stack->next);
    int stack_zone = get_zone(SIZE_4K, addr);
    if (size == SIZE_4K && stack_zone <= zone)
    {
      return addr;
    }
    else
    {
      _pmm_free(SIZE_4K, stack_zone, addr);
      return _pmm_alloc(size, zone);
    }
  }

  if (size == SIZE_2M)
  {
    uintptr_t addr = _pmm_alloc(SIZE_1G, zone);
    if (addr)
    {
      for (uintptr_t off = FRAME_SIZE_2M; off < FRAME_SIZE_1G; off += FRAME_SIZE_2M)
        _pmm_free(SIZE_2M, zone, addr + off);

      return addr;
    }
  }
  else if (size == SIZE_4K)
  {
    uintptr_t addr = _pmm_alloc(SIZE_2M, zone);
    if (addr)
    {
      for (uintptr_t off = FRAME_SIZE; off < FRAME_SIZE_2M; off += FRAME_SIZE)
        _pmm_free(SIZE_4K, zone, addr + off);

      return addr;
    }
  }

  if (zone > ZONE_DMA)
  {
    return _pmm_alloc(size, zone - 1);
  }

  return 0;
}

static void _pmm_free(int size, int zone, uintptr_t addr)
{
  int idx = SZ_TO_IDX(size, zone);
  pmm_stack_t *stack = &pmm_stacks[idx];

  if (stack->count != PMM_STACK_SIZE)
  {
    stack->frames[stack->count++] = addr;
    return;
  }

  if (size == SIZE_4K && zone == ZONE_STD)
  {
    stack->next = stack_switch(size, zone, addr);
    stack->count = 0;
    return;
  }

  uintptr_t new_addr = _pmm_alloc(SIZE_4K, ZONE_STD);
  if (new_addr)
  {
    stack->next = stack_switch(size, zone, new_addr);
    stack->count = 0;
    return;
  }

  if (size == SIZE_4K)
  {
    stack->next = stack_switch(size, zone, addr);
    stack->count = 0;
  }
  else if (size == SIZE_2M)
  {
    stack->next = stack_switch(SIZE_4K, zone, addr);
    stack->count = 0;

    for (uintptr_t inner_addr = addr + FRAME_SIZE; inner_addr < addr + FRAME_SIZE_2M; inner_addr += FRAME_SIZE)
      _pmm_free(SIZE_4K, get_zone(SIZE_4K, inner_addr), inner_addr);
  }
  else if (size == SIZE_1G)
  {
    stack->next = stack_switch(SIZE_4K, zone, addr);
    stack->count = 0;

    for (uintptr_t inner_addr = addr + FRAME_SIZE; inner_addr < addr + FRAME_SIZE_2M; inner_addr += FRAME_SIZE)
      _pmm_free(SIZE_4K, get_zone(SIZE_4K, inner_addr), inner_addr);

    for (uintptr_t inner_addr = addr + FRAME_SIZE_2M; inner_addr < addr + FRAME_SIZE_1G; inner_addr += FRAME_SIZE_2M)
      _pmm_free(SIZE_2M, get_zone(SIZE_2M, inner_addr), inner_addr);
  }
}

static void pmm_push_range(uintptr_t start, uintptr_t end, int size)
{
  uintptr_t inc = 0;
  switch (size)
  {
    case SIZE_4K:
      inc = FRAME_SIZE;
      break;

    case SIZE_2M:
      inc = FRAME_SIZE_2M;
      break;

    case SIZE_1G:
      inc = FRAME_SIZE_1G;
      break;
  }

  for (uintptr_t addr = start; addr < end; addr += inc)
  {
    int zone = get_zone(size, addr);
    int idx = SZ_TO_IDX(size, zone);
    _pmm_free(size, zone, addr);
    pmm_counts[idx]++;
  }
}

void pmm_init(mm_map_t *map)
{
  for (int size = 0; size < SIZE_COUNT; size++)
  {
    for (int zone = 0; zone < ZONE_COUNT; zone++)
    {
      int idx = SZ_TO_IDX(size, zone);
      stack_switch(size, zone, (uintptr_t) &pmm_phy_stacks[idx] - VM_KERNEL_IMAGE);
      memset(&pmm_stacks[idx], 0, sizeof(*pmm_stacks));
    }
  }

  for (size_t id = 0; id < map->count; id++)
  {
    mm_map_entry_t *entry = &map->entries[id];
    if (entry->type != MULTIBOOT_MMAP_AVAILABLE)
      continue;

    uintptr_t start = PAGE_ALIGN(entry->addr_start);
    uintptr_t end = PAGE_ALIGN_REVERSE(entry->addr_end + 1);

    uintptr_t start_2m = PAGE_ALIGN_2M(entry->addr_start);
    uintptr_t end_2m = PAGE_ALIGN_REVERSE_2M(entry->addr_end + 1);

    uintptr_t start_1g = PAGE_ALIGN_1G(entry->addr_start);
    uintptr_t end_1g = PAGE_ALIGN_REVERSE_1G(entry->addr_end + 1);

    if (start_1g <= end_1g)
    {
      if (start <= start_2m)
        pmm_push_range(start, start_2m, SIZE_4K);

      if (end_2m <= end)
        pmm_push_range(end_2m, end, SIZE_4K);

      if (start_2m <= start_1g)
        pmm_push_range(start_2m, start_1g, SIZE_2M);

      if (end_1g <= end_2m)
        pmm_push_range(end_1g, end_2m, SIZE_2M);

      pmm_push_range(start_1g, end_1g, SIZE_1G);
    }
    else if (start_2m <= end_2m)
    {
      if (start <= start_2m)
        pmm_push_range(start, start_2m, SIZE_4K);

      if (end_2m <= end)
        pmm_push_range(end_2m, end, SIZE_4K);

      pmm_push_range(start_2m, end_2m, SIZE_2M);
    }
    else if (start <= end)
    {
      pmm_push_range(start, end, SIZE_4K);
    }
  }

  for (int zone = 0; zone < ZONE_COUNT; zone++)
  {
    for (int size = 0; size < SIZE_COUNT; size++)
    {
      int idx = SZ_TO_IDX(size, zone);
      uint64_t count = pmm_counts[idx];
      if (count > 0)
      {
        const char *zone_str = get_zone_str(zone);
        const char *size_str = get_size_str(size);
        trace_printf(" => Zone %s Size %s: %d frames\n", zone_str, size_str, count);
      }
    }
  }
}

uintptr_t pmm_alloc(void)
{
  return pmm_allocsz(SIZE_4K, ZONE_STD);
}

uintptr_t pmm_allocs(int size)
{
  return pmm_allocsz(size, ZONE_STD);
}

uintptr_t pmm_allocz(int zone)
{
  return pmm_allocsz(SIZE_4K, zone);
}

uintptr_t pmm_allocsz(int size, int zone)
{
  spin_lock(&pmm_lock);
  uintptr_t addr = _pmm_alloc(size, zone);
  spin_unlock(&pmm_lock);
  return addr;
}

void pmm_free(uintptr_t addr)
{
  pmm_frees(SIZE_4K, addr);
}

void pmm_frees(int size, uintptr_t addr)
{
  spin_lock(&pmm_lock);
  _pmm_free(size, get_zone(size, addr), addr);
  spin_unlock(&pmm_lock);
}
