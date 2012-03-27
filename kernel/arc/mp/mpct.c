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

#include <arc/mp/mpct.h>
#include <arc/intr/apic.h>

bool mpct_valid(mpct_t *mpct)
{
  if (mpct->signature != MPCT_SIGNATURE)
    return false;

  uint8_t sum = 0;
  uint8_t *ptr_start = (uint8_t *) mpct;
  uint8_t *ptr_end = ptr_start + mpct->len;

  for (uint8_t *ptr = ptr_start; ptr < ptr_end; ptr++)
    sum += *ptr;

  return sum == 0;
}

void mpct_scan(mpct_t *mpct)
{
  uintptr_t entry_end = (uintptr_t) mpct + mpct->len;
  uintptr_t entry_addr = (uintptr_t) &mpct->entries[0];

  while (entry_addr < entry_end)
  {
    mpct_entry_t *entry = (mpct_entry_t *) entry_addr;
    switch (entry->type)
    {
      case MPCT_TYPE_PROC:
        entry_addr += sizeof(entry->proc);
        break;

      case MPCT_TYPE_BUS:
        entry_addr += sizeof(entry->bus);
        break;

      case MPCT_TYPE_IO_APIC:
        entry_addr += sizeof(entry->io_apic);
        break;

      case MPCT_TYPE_IO_INTR:
        entry_addr += sizeof(entry->io_intr);
        break;

      case MPCT_TYPE_LOCAL_INTR:
        entry_addr += sizeof(entry->local_intr);
        break;
    }

    entry_addr += sizeof(entry->type);
  }

  xapic_init(mpct->lapic_phy_addr);
}

