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

#include <arc/mm/vmm.h>
#include <arc/mm/align.h>
#include <arc/mm/pmm.h>
#include <arc/panic.h>
#include <arc/cpu/tlb.h>
#include <stddef.h>
#include <string.h>

#define PML1_OFFSET 0xFFFFFF8000000000
#define PML2_OFFSET 0xFFFFFFFFC0000000
#define PML3_OFFSET 0xFFFFFFFFFFE00000
#define PML4_OFFSET 0xFFFFFFFFFFFFF000

typedef struct
{
  uint64_t *pml4, *pml3, *pml2, *pml1;
  size_t pml4e, pml3e, pml2e, pml1e;
} page_index_t;

static void addr_to_index(page_index_t *index, uintptr_t addr)
{
  /* calculate pml4 pointer */
  index->pml4 = (uint64_t *) PML4_OFFSET;

  /* calculate pml4 index */
  if (addr >= VM_OFFSET)
  {
    addr -= VM_OFFSET;
    index->pml4e = (TABLE_SIZE / 2) + addr / FRAME_SIZE_512G;
  }
  else
  {
    index->pml4e = addr / FRAME_SIZE_512G;
  }
  addr %= FRAME_SIZE_512G;

  /* calculate pml3 pointer */
  index->pml3 = (uint64_t *) (PML3_OFFSET + index->pml4e * FRAME_SIZE);

  /* calculate pml3 index */
  index->pml3e = addr / FRAME_SIZE_1G;
  addr %= FRAME_SIZE_1G;

  /* calculate pml2 pointer */
  index->pml2 = (uint64_t *) (PML2_OFFSET + index->pml4e * FRAME_SIZE_2M + index->pml3e * FRAME_SIZE);

  /* calculate pml2 index */
  index->pml2e = addr / FRAME_SIZE_2M;
  addr %= FRAME_SIZE_2M;

  /* calculate pml1 pointer */
  index->pml1 = (uint64_t *) (PML1_OFFSET + index->pml4e * FRAME_SIZE_1G + index->pml3e * FRAME_SIZE_2M + index->pml2e * FRAME_SIZE);

  /* calculate pml1 index */
  index->pml1e = addr / FRAME_SIZE;
}

void vmm_init(void)
{
  /*
   * touch all higher half pml4 entries, this means when we have multiple
   * address spaces, we can easily keep the higher half mapped in exactly the
   * same way by not creating all higher half pml4 entries now and never
   * changing them again
   */
  for (int pml4_index = (TABLE_SIZE / 2); pml4_index <= TABLE_SIZE; pml4_index++)
  {
    if (!vmm_touch(VM_OFFSET + (pml4_index - (TABLE_SIZE / 2)) * FRAME_SIZE_512G, SIZE_1G))
      panic("failed to touch pml4 entry %d", pml4_index);
  }
}

bool vmm_touch(uintptr_t virt, int size)
{
  page_index_t index;
  addr_to_index(&index, virt);  

  uint64_t pml4 = index.pml4[index.pml4e];
  uintptr_t frame3 = 0;
  if (!(pml4 & PG_PRESENT))
  {
    frame3 = pmm_alloc();
    if (!frame3)
      return false;

    pml4 = frame3 | PG_WRITABLE | PG_PRESENT;
    index.pml4[index.pml4e] = pml4;
    tlb_invlpg((uintptr_t) index.pml3);
    memset(index.pml3, 0, FRAME_SIZE);
  }

  if (size == SIZE_1G)
    return true;

  uint64_t pml3 = index.pml3[index.pml3e];
  uintptr_t frame2 = 0;
  if (pml3 & PG_BIG)
    goto rollback_pml4;
  if (!(pml3 & PG_PRESENT))
  {
    frame2 = pmm_alloc();
    if (!frame2)
      goto rollback_pml4;

    pml3 = frame2 | PG_WRITABLE | PG_PRESENT;
    index.pml3[index.pml3e] = pml3;
    tlb_invlpg((uintptr_t) index.pml2);
    memset(index.pml2, 0, FRAME_SIZE);
  }

  if (size == SIZE_2M)
    return true;

  uint64_t pml2 = index.pml2[index.pml2e];
  if (pml2 & PG_BIG)
    goto rollback_pml3;
  if (!(pml2 & PG_PRESENT))
  {
    uintptr_t frame1 = pmm_alloc();
    if (!frame1)
      goto rollback_pml3;

    pml2 = frame1 | PG_WRITABLE | PG_PRESENT;
    index.pml2[index.pml2e] = pml2;
    tlb_invlpg((uintptr_t) index.pml1);
    memset(index.pml1, 0, FRAME_SIZE);
  }

  return true;

rollback_pml3:
  if (frame2)
  {
    index.pml3[index.pml3e] = 0;
    tlb_invlpg((uintptr_t) index.pml2);
    pmm_free(frame2);
  }
rollback_pml4:
  if (frame3)
  {
    index.pml4[index.pml4e] = 0;
    tlb_invlpg((uintptr_t) index.pml3);
    pmm_free(frame3);
  }
  return false;
}

bool vmm_map(uintptr_t virt, uintptr_t phy, uint64_t flags)
{
  return vmm_maps(virt, phy, flags, SIZE_4K);
}

bool vmm_maps(uintptr_t virt, uintptr_t phy, uint64_t flags, int size)
{
  if (!vmm_touch(virt, size))
    return false;

  page_index_t index;
  addr_to_index(&index, virt);

  switch (size)
  {
    case SIZE_4K:
      index.pml1[index.pml1e] = phy | PG_PRESENT | flags;
      break;

    case SIZE_2M:
      index.pml2[index.pml2e] = phy | PG_PRESENT | PG_BIG | flags;
      break;

    case SIZE_1G:
      index.pml3[index.pml3e] = phy | PG_PRESENT | PG_BIG | flags;
      break;
  }

  tlb_invlpg(virt);
  return true;
}

uintptr_t vmm_unmap(uintptr_t virt)
{
  return vmm_unmaps(virt, SIZE_4K);
}

uintptr_t vmm_unmaps(uintptr_t virt, int size)
{
  page_index_t index;
  addr_to_index(&index, virt);

  uintptr_t frame = 0;
  switch (size)
  {
    case SIZE_4K:
      if (index.pml1[index.pml1e] & PG_PRESENT)
        frame = index.pml1[index.pml1e] & PG_ADDR_MASK;
      index.pml1[index.pml1e] = 0;
      break;

    case SIZE_2M:
      if (index.pml2[index.pml2e] & PG_PRESENT)
        frame = index.pml2[index.pml2e] & PG_ADDR_MASK;
      index.pml2[index.pml2e] = 0;
      break;

    case SIZE_1G:
      if (index.pml3[index.pml3e] & PG_PRESENT)
        frame = index.pml3[index.pml3e] & PG_ADDR_MASK;
      index.pml3[index.pml3e] = 0;
      break;
  }

  tlb_invlpg(virt);
  vmm_untouch(virt, size);
  return frame;
}

void vmm_untouch(uintptr_t virt, int size)
{
  page_index_t index;
  addr_to_index(&index, virt);

  if (size == SIZE_4K)
  {
    bool empty = true;
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
      if (index.pml1[i] & PG_PRESENT)
      {
        empty = false;
        break;
      }
    }

    if (empty)
    {
      pmm_free(index.pml2[index.pml2e] & PG_ADDR_MASK);
      index.pml2[index.pml2e] = 0;
      tlb_invlpg((uintptr_t) index.pml1);
    }
  }

  if (size == SIZE_4K || size == SIZE_2M)
  {
    bool empty = true;
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
      if (index.pml2[i] & PG_PRESENT)
      {
        empty = false;
        break;
      }
    }

    if (empty)
    {
      pmm_free(index.pml3[index.pml3e] & PG_ADDR_MASK);
      index.pml3[index.pml3e] = 0;
      tlb_invlpg((uintptr_t) index.pml2);
    }
  }

  if ((size == SIZE_4K || size == SIZE_2M || size == SIZE_1G) && (index.pml4e < (TABLE_SIZE / 2)))
  {
    bool empty = true;
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
      if (index.pml3[i] & PG_PRESENT)
      {
        empty = false;
        break;
      }
    }

    if (empty)
    {
      pmm_free(index.pml4[index.pml4e] & PG_ADDR_MASK);
      index.pml4[index.pml4e] = 0;
      tlb_invlpg((uintptr_t) index.pml3);
    }
  }
}

bool vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, uint64_t flags)
{
  size_t off;
  for (off = 0; off < PAGE_ALIGN(len); off += FRAME_SIZE)
  {
    if (!vmm_map(virt + off, phy + off, flags))
    {
      goto rollback;
    }
  }
  return true;

rollback:
  for (size_t rb_off = 0; rb_off < off; rb_off += FRAME_SIZE)
    vmm_unmap(rb_off);
  return false;
}

void vmm_unmap_range(uintptr_t virt, size_t len)
{
  for (size_t off = 0; off < PAGE_ALIGN(len); off += FRAME_SIZE)
    vmm_unmap(virt + off);
}

