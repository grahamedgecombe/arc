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
#include <arc/mm/common.h>
#include <arc/cpu/tlb.h>
#include <arc/lock/spinlock.h>
#include <arc/pack.h>

#define PAGE_TABLE_OFFSET 0xFFFFFFFFBF7FF000
#define STACKS (ZONE_COUNT * SIZE_COUNT)
#define PMM_STACK_SIZE (TABLE_SIZE - 2)
#define SZ_TO_IDX(s,z) ((s) * ZONE_COUNT + (z))

static uint64_t *pmm_page_table = (uint64_t *) PAGE_TABLE_OFFSET;
static spinlock_t pmm_lock;

typedef PACK(struct
{
  uint64_t next;
  uint64_t count;
  uint64_t frames[PMM_STACK_SIZE];
}) pmm_stack_t;

static int zone(int size, uintptr_t addr)
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

static uintptr_t _pmm_alloc(int size, int zone)
{
  return 0;
}

static void _pmm_free(int size, int zone, uintptr_t addr)
{

}

void pmm_init(mm_map_t *map)
{

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
  _pmm_free(size, zone(size, addr), addr);
  spin_unlock(&pmm_lock);
}

