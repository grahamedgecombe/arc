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

#include <arc/smp/init.h>
#include <arc/smp/mptab.h>
#include <arc/tty.h>
#include <arc/mm/phy32.h>
#include <string.h>

void smp_init(void)
{
  /* find MP floating pointer */
  tty_puts(" => Searching for Intel MP structures...\n");
  mpfp_t *mpfp = mpfp_scan();
  if (!mpfp)
  {
    tty_printf(" => No MPFP structure found, SMP disabled\n");
    return;
  }
  tty_printf(" => Found MPFP structure at %0#10x\n", (uintptr_t) mpfp & 0xFFFFFFFF);

  /* find MP config table header */
  if (!mpfp->phy_addr)
  {
    tty_printf(" => No MPCT structure found, SMP disabled\n");
    return;
  }

  /* check for validity of the MP config table header */
  mpct_t *mpct = (mpct_t *) aphy32_to_virt(mpfp->phy_addr);
  if (!mpct_valid(mpct))
  {
    tty_printf(" => MPCT structure corrupt, SMP disabled\n");
    return;
  }
  tty_printf(" => Found MPCT structure at %0#10x\n", (uintptr_t) mpct & 0xFFFFFFFF);

  /* go through the MP config table entries */
  uintptr_t entry_addr = (uintptr_t) mpct + sizeof(mpct_t); 
  for (int i = 0; i < mpct->entry_count; i++)
  {
    mpct_entry_t *entry = (mpct_entry_t *) entry_addr;
    switch (entry->type)
    {
      case MPCT_TYPE_PROC:
        {
          int family   =  entry->proc.cpu_signature       & 0xF;
          int model    = (entry->proc.cpu_signature >> 4) & 0xF;
          int stepping = (entry->proc.cpu_signature >> 8) & 0xF;

          tty_printf("     => cpu%d: family=%d, model=%d, stepping=%d\n", entry->proc.id, family, model, stepping); 
        }
        entry_addr += sizeof(entry->proc) + 1;
        break;

      case MPCT_TYPE_BUS:
        {
          char type[7];
          memcpy(type, entry->bus.type, sizeof(type) - 1);
          type[6] = 0;

          tty_printf("     => bus%d: type=%s\n", entry->bus.id, type);
        }
        entry_addr += sizeof(entry->bus) + 1;
        break;

      case MPCT_TYPE_IO_APIC:
        tty_printf("     => ioapic%d: addr=%0#10x\n", entry->io_apic.id, entry->io_apic.phy_addr);
        entry_addr += sizeof(entry->io_apic) + 1;
        break;

      case MPCT_TYPE_IO_INTR:
        tty_printf("     => iointr: source=bus%dirq%d, dest=ioapic%dint%d\n", entry->io_intr.source_bus, entry->io_intr.source_irq, entry->io_intr.dest_io_apic, entry->io_intr.dest_intr);
        entry_addr += sizeof(entry->io_intr) + 1;
        break;

      case MPCT_TYPE_LOCAL_INTR:
        tty_printf("     => localintr: source=bus%dirq%d, dest=lapic%dint%d\n", entry->local_intr.source_bus, entry->local_intr.source_irq, entry->local_intr.dest_local_apic, entry->local_intr.dest_intr);
        entry_addr += sizeof(entry->local_intr) + 1;
        break;
    }
  }
}

