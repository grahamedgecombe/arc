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
#include <arc/proc/proc.h>
#include <arc/lock/spinlock.h>
#include <arc/mm/tlb.h>
#include <arc/mm/align.h>
#include <arc/mm/pmm.h>
#include <arc/mm/mmio.h>
#include <arc/panic.h>
#include <arc/cpu/tlb.h>
#include <arc/cpu/features.h>
#include <stddef.h>
#include <string.h>

#define PML1_OFFSET 0xFFFFFF0000000000
#define PML2_OFFSET 0xFFFFFF7F80000000
#define PML3_OFFSET 0xFFFFFF7FBFC00000
#define PML4_OFFSET 0xFFFFFF7FBFDFE000

typedef struct
{
  uint64_t *pml4, *pml3, *pml2, *pml1;
  size_t pml4e, pml3e, pml2e, pml1e;
} page_index_t;

static bool vmm_1g_pages;
static spinlock_t kernel_vmm_lock = SPIN_UNLOCKED;

/* forward declarations of internal vmm functions with no locking */
static bool _vmm_touch(uintptr_t virt, int size);
static bool _vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags);
static bool _vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size);
static uintptr_t _vmm_unmap(uintptr_t virt);
static uintptr_t _vmm_unmaps(uintptr_t virt, int size);
static void _vmm_untouch(uintptr_t virt, int size);
static bool _vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags);
static void _vmm_unmap_range(uintptr_t virt, size_t len);
static int _vmm_size(uintptr_t virt);

static void vmm_lock(uintptr_t addr)
{
  proc_t *proc = proc_get();
  if (proc && addr < VM_HIGHER_HALF)
    spin_lock(&proc->vmm_lock);
  else
    spin_lock(&kernel_vmm_lock);
}

static void vmm_unlock(uintptr_t addr)
{
  proc_t *proc = proc_get();
  if (proc && addr < VM_HIGHER_HALF)
    spin_unlock(&proc->vmm_lock);
  else
    spin_unlock(&kernel_vmm_lock);
}

static void addr_to_index(page_index_t *index, uintptr_t addr)
{
  /* calculate pml4 pointer */
  index->pml4 = (uint64_t *) PML4_OFFSET;

  /* calculate pml4 index */
  if (addr >= VM_HIGHER_HALF)
  {
    addr -= VM_HIGHER_HALF;
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
  /* set 1g support flag */
  vmm_1g_pages = cpu_feature_supported(FEATURE_1G_PAGE);

  /*
   * touch all higher half pml4 entries, this means when we have multiple
   * address spaces, we can easily keep the higher half mapped in exactly the
   * same way by not creating all higher half pml4 entries now and never
   * changing them again
   */
  for (int pml4_index = (TABLE_SIZE / 2); pml4_index <= TABLE_SIZE; pml4_index++)
  {
    if (!_vmm_touch(VM_HIGHER_HALF + (pml4_index - (TABLE_SIZE / 2)) * FRAME_SIZE_512G, SIZE_1G))
      panic("failed to touch pml4 entry %d", pml4_index);
  }
}

bool vmm_init_pml4(uintptr_t pml4_table_addr)
{
  // TODO: pre-allocate this MMIO area so this call always succeeds
  uint64_t *pml4_table = mmio_map(pml4_table_addr, FRAME_SIZE, VM_R | VM_W);
  if (!pml4_table)
    return false;

  uint64_t *master_pml4_table = (uint64_t *) PML4_OFFSET;

  /* reset the lower half PML4 entries */
  memset(pml4_table, 0, FRAME_SIZE / 2);

  /* copy the higher half PML4 entries from the master table */
  for (int pml4e = TABLE_SIZE / 2; pml4e < TABLE_SIZE; pml4e++)
  {
    pml4_table[pml4e] = master_pml4_table[pml4e];
  }

  /* map PML4 into itself */
  pml4_table[TABLE_SIZE - 2] = pml4_table_addr | PG_PRESENT | PG_WRITABLE | PG_NO_EXEC;

  mmio_unmap(pml4_table, FRAME_SIZE);
  return true;
}

static int _vmm_size(uintptr_t virt)
{
  page_index_t index;
  addr_to_index(&index, virt);

  uint64_t pml4 = index.pml4[index.pml4e];
  if (!(pml4 & PG_PRESENT))
    return -1;

  uint64_t pml3 = index.pml3[index.pml3e];
  if (!(pml3 & PG_PRESENT))
    return -1;
  if (pml3 & PG_BIG)
    return SIZE_1G;

  uint64_t pml2 = index.pml2[index.pml2e];
  if (!(pml2 & PG_PRESENT))
    return -1;
  if (pml2 & PG_BIG)
    return SIZE_2M;

  uint64_t pml1 = index.pml1[index.pml1e];
  if (!(pml1 & PG_PRESENT))
    return -1;

  return SIZE_4K;
}

static bool _vmm_touch(uintptr_t virt, int size)
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
    if (index.pml4e < (TABLE_SIZE / 2))
      pml4 |= PG_USER;

    index.pml4[index.pml4e] = pml4;
    tlb_transaction_queue_invlpg((uintptr_t) index.pml3);
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
    if (index.pml4e < (TABLE_SIZE / 2))
      pml3 |= PG_USER;

    index.pml3[index.pml3e] = pml3;
    tlb_transaction_queue_invlpg((uintptr_t) index.pml2);
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
    if (index.pml4e < (TABLE_SIZE / 2))
      pml2 |= PG_USER;

    index.pml2[index.pml2e] = pml2;
    tlb_transaction_queue_invlpg((uintptr_t) index.pml1);
    memset(index.pml1, 0, FRAME_SIZE);
  }

  return true;

rollback_pml3:
  if (frame2)
  {
    index.pml3[index.pml3e] = 0;
    tlb_transaction_queue_invlpg((uintptr_t) index.pml2);
    pmm_free(frame2);
  }
rollback_pml4:
  if (frame3)
  {
    index.pml4[index.pml4e] = 0;
    tlb_transaction_queue_invlpg((uintptr_t) index.pml3);
    pmm_free(frame3);
  }
  return false;
}

static bool _vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags)
{
  return _vmm_maps(virt, phy, flags, SIZE_4K);
}

static bool _vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size)
{
  if (size == SIZE_1G && !vmm_1g_pages)
    return false;

  if (!_vmm_touch(virt, size))
    return false;

  page_index_t index;
  addr_to_index(&index, virt);

  uint64_t pg_flags = 0;
  if (flags & VM_W)
    pg_flags |= PG_WRITABLE;
  if (!(flags & VM_X))
    pg_flags |= PG_NO_EXEC;
  if (index.pml4e < (TABLE_SIZE / 2))
    pg_flags |= PG_USER;

  switch (size)
  {
    case SIZE_4K:
      index.pml1[index.pml1e] = phy | PG_PRESENT | pg_flags;
      break;

    case SIZE_2M:
      index.pml2[index.pml2e] = phy | PG_PRESENT | PG_BIG | pg_flags;
      break;

    case SIZE_1G:
      index.pml3[index.pml3e] = phy | PG_PRESENT | PG_BIG | pg_flags;
      break;
  }

  tlb_transaction_queue_invlpg(virt);
  return true;
}

static uintptr_t _vmm_unmap(uintptr_t virt)
{
  return _vmm_unmaps(virt, SIZE_4K);
}

static uintptr_t _vmm_unmaps(uintptr_t virt, int size)
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

  tlb_transaction_queue_invlpg(virt);
  _vmm_untouch(virt, size);
  return frame;
}

static void _vmm_untouch(uintptr_t virt, int size)
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
      tlb_transaction_queue_invlpg((uintptr_t) index.pml1);
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
      tlb_transaction_queue_invlpg((uintptr_t) index.pml2);
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
      tlb_transaction_queue_invlpg((uintptr_t) index.pml3);
    }
  }
}

static bool _vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags)
{
  len = PAGE_ALIGN(len);
  for (size_t off = 0; off < len;)
  {
    size_t remaining = len - off;
    if ((PAGE_ALIGN_1G(virt + off) == (virt + off)) && remaining >= FRAME_SIZE_1G)
    {
      if (_vmm_maps(virt + off, phy + off, flags, SIZE_1G))
      {
        off += FRAME_SIZE_1G;
        continue;
      }
    }

    if ((PAGE_ALIGN_2M(virt + off) == (virt + off)) && remaining >= FRAME_SIZE_2M)
    {
      if (_vmm_maps(virt + off, phy + off, flags, SIZE_2M))
      {
        off += FRAME_SIZE_2M;
        continue;
      }
    }

    if (!_vmm_map(virt + off, phy + off, flags))
    {
      _vmm_unmap_range(virt, off);
      return false;
    }

    off += FRAME_SIZE;
  }
  return true;
}

static void _vmm_unmap_range(uintptr_t virt, size_t len)
{
  len = PAGE_ALIGN(len);
  for (size_t off = 0; off < len;)
  {
    int size = _vmm_size(virt + off);
    if (size != -1)
      _vmm_unmaps(size, virt + off);

    if (size == SIZE_1G)
      off += FRAME_SIZE_1G;
    else if (size == SIZE_2M)
      off += FRAME_SIZE_2M;
    else
      off += FRAME_SIZE;
  }
}

bool vmm_touch(uintptr_t virt, int size)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_touch(virt, size);
  if (ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

bool vmm_map(uintptr_t virt, uintptr_t phy, vm_acc_t flags)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_map(virt, phy, flags);
  if (ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

bool vmm_maps(uintptr_t virt, uintptr_t phy, vm_acc_t flags, int size)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_maps(virt, phy, flags, size);
  if (ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

uintptr_t vmm_unmap(uintptr_t virt)
{
  vmm_lock(virt);
  tlb_transaction_init();
  uintptr_t addr = _vmm_unmap(virt);
  tlb_transaction_commit();
  vmm_unlock(virt);
  return addr;
}

uintptr_t vmm_unmaps(uintptr_t virt, int size)
{
  vmm_lock(virt);
  tlb_transaction_init();
  uintptr_t addr = _vmm_unmaps(virt, size);
  tlb_transaction_commit();
  vmm_unlock(virt);
  return addr;
}

void vmm_untouch(uintptr_t virt, int size)
{
  vmm_lock(virt);
  tlb_transaction_init();
  _vmm_untouch(virt, size);
  tlb_transaction_commit();
  vmm_unlock(virt);
}

bool vmm_map_range(uintptr_t virt, uintptr_t phy, size_t len, vm_acc_t flags)
{
  vmm_lock(virt);

  tlb_transaction_init();
  bool ok = _vmm_map_range(virt, phy, len, flags);
  if (ok)
    tlb_transaction_commit();
  else
    tlb_transaction_rollback();

  vmm_unlock(virt);
  return ok;
}

void vmm_unmap_range(uintptr_t virt, size_t len)
{
  vmm_lock(virt);
  tlb_transaction_init();
  _vmm_unmap_range(virt, len);
  tlb_transaction_commit();
  vmm_unlock(virt);
}

int vmm_size(uintptr_t virt)
{
  vmm_lock(virt);
  int size = _vmm_size(virt);
  vmm_unlock(virt);
  return size;
}

