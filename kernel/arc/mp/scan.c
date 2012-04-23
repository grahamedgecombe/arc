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

#include <arc/mp/scan.h>
#include <arc/cpu/imcr.h>
#include <arc/mp/mpfp.h>
#include <arc/mp/mpct.h>
#include <arc/mm/phy32.h>
#include <arc/panic.h>
#include <arc/tty.h>

bool mp_scan(void)
{
  /* try to find the MPFP structure */
  mpfp_t *mpfp = mpfp_scan();
  if (!mpfp)
  {
    tty_puts(" => MP not supported\n");
    return false;
  }

  /* if features[0] is nonzero, the system uses the default config */
  if (mpfp->features[0] != 0)
  {
    // TODO: add support for default config
    tty_puts(" => MP default configuration not supported\n");
    return false;
  }

  /* get a pointer to where the MPFP is in virtual memory */
  mpct_t *mpct = (mpct_t *) aphy32_to_virt(mpfp->phy_addr);
  if (!mpct_valid(mpct))
    panic("invalid checksum in MPCT");

  /* scan the mpct */
  mpct_scan(mpct);

  /* switch to symmetric I/O mode */
  if (mpfp->features[1] & MPFP_IMCRP)
    imcr_write(1);

  return true;
}

