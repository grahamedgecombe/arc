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

#define PML1_OFFSET 0xFFFFFF8000000000
#define PML2_OFFSET 0xFFFFFFFFC0000000
#define PML3_OFFSET 0xFFFFFFFFFFE00000
#define PML4_OFFSET 0xFFFFFFFFFFFFF000

void vmm_init(void)
{
  /*
   * touch all higher half pml4 entries, this means when we have multiple
   * address spaces, we can easily keep the higher half mapped in exactly the
   * same way by not creating all higher half pml4 entries now and never
   * changing them again
   * 
   * we touch entry 256 to 510 inclusive - 511 is already used for mapping the
   * 32-bit physical address space into virtual memory and for the physical
   * free page stacks, 512 is used for the recursive page directory
   */
  for (int pml4_index = 256; pml4_index < 510; pml4_index++)
    vmm_touch(VM_OFFSET + pml4_index * FRAME_SIZE_512G, SIZE_1G);
}

void vmm_touch(uintptr_t virt, int size)
{

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

