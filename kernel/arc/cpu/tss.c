/*
 * Copyright (c) 2011-2014 Graham Edgecombe <graham@grahamedgecombe.com>
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

#include <arc/cpu/tss.h>
#include <arc/smp/cpu.h>
#include <string.h>

void tss_init(void)
{
  /* find this CPU's TSS */
  cpu_t *cpu = cpu_get();
  tss_t *tss = &cpu->tss;

  /* reset all the fields */
  memset(tss, 0, sizeof(*tss));
  tss->iomap_base = sizeof(*tss);

  /* install it using the LTR instruction */
  tss_install(SLTR_TSS);
}

void tss_set_rsp0(uint64_t rsp0) {
  /* find this CPU's TSS */
  cpu_t *cpu = cpu_get();
  tss_t *tss = &cpu->tss;

  /* set the stack pointer for this CPU */
  tss->rsp0 = rsp0;
}
