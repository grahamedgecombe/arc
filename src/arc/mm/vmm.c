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

#define PML1_OFFSET 0xFFFFFF8000000000
#define PML2_OFFSET 0xFFFFFFFFC0000000
#define PML3_OFFSET 0xFFFFFFFFFFE00000
#define PML4_OFFSET 0xFFFFFFFFFFFFF000

typedef struct
{
  int pml4;
  int pml3;
  int pml2;
  int pml1;
} page_index_t;

static void addr_to_index(page_index_t *index, uintptr_t addr)
{
  /* calculate pml4 index */
  if (addr >= VM_OFFSET)
  {
    addr -= VM_OFFSET;
    index->pml4 = 256 + addr / FRAME_SIZE_512G;
  }
  else
  {
    index->pml4 = addr / FRAME_SIZE_512G;
  }
  addr %= FRAME_SIZE_512G;

  /* calculate pml3 index */
  index->pml3 = addr / FRAME_SIZE_1G;
  addr %= FRAME_SIZE_1G;

  /* calculate pml2 index */
  index->pml2 = addr / FRAME_SIZE_2M;
  addr %= FRAME_SIZE_2M;

  /* calculate pml1 index */
  index->pml1 = addr / FRAME_SIZE;
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

