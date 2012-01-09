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

#include <arc/smp/mptab.h>
#include <arc/mm/phy32.h>
#include <arc/bda.h>

static bool mpfp_valid(mpfp_t *mpfp)
{
  if (mpfp->signature != MPFP_SIGNATURE)
    return false;

  uint8_t sum = 0;
  uint8_t *ptr_start = (uint8_t *) mpfp;
  uint8_t *ptr_end = ptr_start + mpfp->len * MP_ALIGN;

  for (uint8_t *ptr = ptr_start; ptr < ptr_end; ptr++)
    sum += *ptr;

  return sum == 0;
}

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

static mpfp_t *mpfp_scan_range(uintptr_t start, uintptr_t end)
{
  start = aphy32_to_virt(start);
  end = aphy32_to_virt(end);

  for (uintptr_t ptr = start; ptr < end; ptr += MP_ALIGN)
    if (mpfp_valid((mpfp_t *) ptr))
      return (mpfp_t *) ptr;

  return 0;
}
 
mpfp_t *mpfp_scan(void)
{
  /* find the address of the EBDA */
  uintptr_t ebda = bda_reads(BDA_EBDA) << 4;
 
  /* search the first kilobyte of the EBDA */
  mpfp_t *mpfp = mpfp_scan_range(ebda, ebda + 1024);
  if (mpfp)
    return mpfp;

  /* search between 639K and 640K */
  mpfp = mpfp_scan_range(0x9FC00, 0xA0000);
  if (mpfp)
    return mpfp;

  /* search between 511K and 512K */
  mpfp = mpfp_scan_range(0x7FC00, 0x80000);
  if (mpfp)
    return mpfp;

  /* search the shadow BIOS ROM */
  mpfp = mpfp_scan_range(0xF0000, 0x100000);
  if (mpfp)
    return mpfp;

  return 0;
}

