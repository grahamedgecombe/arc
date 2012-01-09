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

#include <arc/acpi/rsdp.h>
#include <arc/mm/phy32.h>
#include <arc/bda.h>

static rsdp_t *rsdp_scan_range(uintptr_t start, uintptr_t end)
{
  start = aphy32_to_virt(start);
  end = aphy32_to_virt(end);

  for (uintptr_t ptr = start; ptr < end; ptr += RSDP_ALIGN)
  {
    rsdp_t *rsdp = (rsdp_t *) ptr;
    if (rsdp->signature == RSDP_SIGNATURE && acpi_table_valid((acpi_header_t *) rsdp))
      return rsdp;
  }

  return 0;
}

rsdp_t *rsdp_scan(void)
{
  /* find the address of the EBDA */
  uintptr_t ebda = bda_reads(BDA_EBDA) << 4;

  /* search the first kilobyte of the EBDA */
  rsdp_t *rsdp = rsdp_scan_range(ebda, ebda + 1024);
  if (rsdp)
    return rsdp;

  /* search the shadow BIOS ROM */
  rsdp = rsdp_scan_range(0xE0000, 0x100000);
  if (rsdp)
    return rsdp;

  return 0;
}

