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

#include <arc/mm/range.h>
#include <arc/mm/common.h>
#include <arc/mm/pmm.h>
#include <arc/mm/vmm.h>
#include <assert.h>

bool range_alloc(uintptr_t addr_start, size_t len, uint64_t flags)
{
  assert((len % FRAME_SIZE) == 0);

  for (uintptr_t addr = addr_start, addr_end = addr + len; addr < addr_end;)
  {
    size_t remaining = addr_end - addr;

    /* try to use a 1G frame */
    if (remaining >= FRAME_SIZE_1G)
    {
      uintptr_t frame = pmm_allocs(SIZE_1G);
      if (frame)
      {
        if (vmm_maps(addr, frame, flags, SIZE_1G))
        {
          addr += FRAME_SIZE_1G;
          continue;
        }
        else
        {
          pmm_frees(frame, SIZE_1G);
        }
      }
    }

    /* try to use a 2M frame */
    if (remaining >= FRAME_SIZE_2M)
    {
      uintptr_t frame = pmm_allocs(SIZE_2M);
      if (frame)
      {
        if (vmm_maps(addr, frame, flags, SIZE_2M))
        {
          addr += FRAME_SIZE_2M;
          continue;
        }
        else
        {
          pmm_frees(frame, SIZE_2M);
        }
      }
    }

    /* try to use a 4K frame */
    uintptr_t frame = pmm_alloc();
    if (!frame)
    {
      range_free(addr_start, len);
      return false;
    }

    if (!vmm_map(addr, frame, flags))
    {
      pmm_free(frame);
      range_free(addr_start, len);
      return false;
    }

    addr += FRAME_SIZE;
  }

  return true;
}

void range_free(uintptr_t addr, size_t len)
{
  assert((len % FRAME_SIZE) == 0);

  for (uintptr_t addr_end = addr + len; addr < addr_end;)
  {
    int size = vmm_size(addr);
    if (size != -1)
      pmm_free(vmm_unmap(addr));

    switch (size)
    {
      case SIZE_1G:
        addr += FRAME_SIZE_1G;
        break;

      case SIZE_2M:
        addr += FRAME_SIZE_2M;
        break;
      
      default:
        addr += FRAME_SIZE;
        break;
    }
  }
}

