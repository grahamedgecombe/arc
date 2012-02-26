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

#include <arc/acpi/madt.h>
#include <arc/bus/isa.h>
#include <arc/intr/ic.h>
#include <arc/intr/pic.h>
#include <arc/intr/ioapic.h>
#include <arc/smp/cpu.h>
#include <arc/panic.h>
#include <arc/tty.h>
#include <stdbool.h>
#include <stddef.h>

void madt_scan(madt_t *madt)
{
  /* the 32-bit address of the LAPIC */
  uint64_t lapic_addr = madt->lapic_addr;

  /* a flag indicating if the BSP has been found in the table yet */
  bool bsp = true;

  /* iterate through the MADT entries */
  uintptr_t ptr = (uintptr_t) &madt->entries[0];
  uintptr_t ptr_end = (uintptr_t) madt + madt->header.len;
  while (ptr < ptr_end)
  {
    madt_entry_t *entry = (madt_entry_t *) ptr;
    ptr += entry->len;

    switch (entry->type)
    {
      case MADT_TYPE_INTR:
        {
          uint8_t bus = entry->intr.bus;
          uint8_t line = entry->intr.irq;
          uint32_t gsi = entry->intr.gsi;
          uint16_t flags = entry->intr.flags;

          if (bus == MADT_INTR_BUS_ISA)
          {
            if (line >= ISA_INTR_LINES)
              panic("ISA interrupt line out of range: %d", line);

            irq_tuple_t *tuple = isa_irq(line);
            tuple->irq = gsi;

            if (flags & MADT_INTR_POLARITY_HIGH)
              tuple->active_polarity = POLARITY_HIGH;
            else if (flags & MADT_INTR_POLARITY_LOW)
              tuple->active_polarity = POLARITY_LOW;

            if (flags & MADT_INTR_TRIGGER_EDGE)
              tuple->trigger = TRIGGER_EDGE;
            else if (flags & MADT_INTR_TRIGGER_LEVEL)
              tuple->trigger = TRIGGER_LEVEL;
          }
        }
        break;

      case MADT_TYPE_LAPIC_ADDR:
        /* override the LAPIC address with a 64-bit one */
        lapic_addr = entry->lapic_addr.addr;
        break;

      case MADT_TYPE_LAPIC:
        if (entry->lapic.flags & MADT_LAPIC_FLAGS_ENABLED)
        {
          uint8_t acpi_id  = entry->lapic.cpu_id;
          uint8_t lapic_id = entry->lapic.lapic_id;

          if (bsp)
          {
            bsp = false;

            /*
             * ACPI spec states that the first LAPIC entry is the BSP, fill out
             * the extra bits not done by cpu_bsp_init() here
             */
            cpu_t *cpu_bsp = cpu_get();
            cpu_bsp->lapic_id = lapic_id;
            cpu_bsp->acpi_id = acpi_id;
          }
          else
          {
            if (!cpu_ap_init(lapic_id, acpi_id))
              panic("failed to register AP");
          }
        }
        break;

      case MADT_TYPE_IOAPIC:
        {
          uint8_t id = entry->ioapic.id;
          uint32_t addr = entry->ioapic.addr;
          uint32_t gsi_base = entry->ioapic.gsi_base;
          if (!ioapic_init(id, addr, gsi_base))
            panic("failed to register I/O APIC");
        }
        break;
    }
  }

  /* mask the PICs if they are present */
  if (madt->flags & MADT_FLAGS_PCAT)
    pic_init();

  /* initialise the IC */
  ic_bsp_init(IC_TYPE_LAPIC, lapic_addr);
}

