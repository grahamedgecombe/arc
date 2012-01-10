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
    index->pml4e = 256 + addr / FRAME_SIZE_512G;
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
  for (int pml4_index = 256; pml4_index <= 512; pml4_index++)
  {
    if (!vmm_touch(VM_OFFSET + (pml4_index - 256) * FRAME_SIZE_512G, SIZE_1G))
      boot_panic("failed to touch pml4 entry %d", pml4_index);
  }
}

bool vmm_touch(uintptr_t virt, int size)
{
  page_index_t index;
  addr_to_index(&index, virt);  

  uint64_t pml4 = index.pml4[index.pml4e];
  void *frame3 = 0;
  if (!(pml4 & PG_PRESENT))
  {
    frame3 = pmm_alloc();
    if (!frame3)
      return false;

    pml4 = (uint64_t) frame3 | PG_WRITABLE | PG_PRESENT;
    index.pml4[index.pml4e] = pml4;
    tlb_invlpg((uintptr_t) index.pml3);
    memset(index.pml3, 0, FRAME_SIZE);
  }

  if (size == SIZE_1G)
    return true;

  uint64_t pml3 = index.pml3[index.pml3e];
  void *frame2 = 0;
  if (pml3 & PG_BIG)
    goto rollback_pml4;
  if (!(pml3 & PG_PRESENT))
  {
    frame2 = pmm_alloc();
    if (!frame2)
      goto rollback_pml4;

    pml3 = (uint64_t) frame2 | PG_WRITABLE | PG_PRESENT;
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
    void *frame1 = pmm_alloc();
    if (!frame1)
      goto rollback_pml3;

    pml2 = (uint64_t) frame1 | PG_WRITABLE | PG_PRESENT;
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
  return false;
}

void vmm_unmap(uintptr_t virt)
{
  vmm_unmaps(virt, SIZE_4K);
}

void vmm_unmaps(uintptr_t virt, int size)
{

}

