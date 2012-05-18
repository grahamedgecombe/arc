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
#include <arc/bus/isa.h>
#include <arc/intr/apic.h>
#include <arc/intr/ioapic.h>
#include <arc/intr/pic.h>
#include <arc/intr/nmi.h>
#include <arc/smp/cpu.h>
#include <arc/util/container.h>
#include <arc/panic.h>

static void mpct_flags_to_trigger(irq_tuple_t *tuple, uint16_t flags)
{
  if ((flags & MPCT_IO_INTR_POLARITY_HIGH) == MPCT_IO_INTR_POLARITY_HIGH)
    tuple->active_polarity = POLARITY_HIGH;
  else if ((flags & MPCT_IO_INTR_POLARITY_LOW) == MPCT_IO_INTR_POLARITY_LOW)
    tuple->active_polarity = POLARITY_LOW;

  if ((flags & MPCT_IO_INTR_TRIGGER_EDGE) == MPCT_IO_INTR_TRIGGER_EDGE)
    tuple->trigger = TRIGGER_EDGE;
  else if ((flags & MPCT_IO_INTR_TRIGGER_LEVEL) == MPCT_IO_INTR_TRIGGER_LEVEL)
    tuple->trigger = TRIGGER_LEVEL;
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

void mpct_scan(mpct_t *mpct)
{
  uintptr_t entry_end = (uintptr_t) mpct + mpct->len;
  uintptr_t entry_addr = (uintptr_t) &mpct->entries[0];

  bool isa_bus_found = false;
  uint8_t isa_bus_id = 0; /* zero to prevent warnings */

  /* perform the first pass to register processors, I/O APICs and buses */
  while (entry_addr < entry_end)
  {
    mpct_entry_t *entry = (mpct_entry_t *) entry_addr;
    switch (entry->type)
    {
      case MPCT_TYPE_PROC:
        if (entry->proc.flags & MPCT_PROC_FLAGS_ENABLED)
        {
          if (entry->proc.flags & MPCT_PROC_FLAGS_BSP)
          {
            cpu_t *cpu_bsp = cpu_get();
            cpu_bsp->lapic_id = entry->proc.id;
            cpu_bsp->acpi_id = 0;
          }
          else
          {
            if (!cpu_ap_init(entry->proc.id, 0))
              panic("failed to register AP");
          }
        }
        entry_addr += sizeof(entry->proc);
        break;

      case MPCT_TYPE_BUS:
        {
          char *type = (char *) entry->bus.type;
          if (type[0] == 'I' && type[1] == 'S' && type[2] == 'A' &&
              type[3] == ' ' && type[4] == ' ' && type[5] == ' ')
          {
            isa_bus_found = true;
            isa_bus_id = entry->bus.id;
          }
        }
        entry_addr += sizeof(entry->bus);
        break;

      case MPCT_TYPE_IOAPIC:
        {
          uint8_t id = entry->ioapic.id;
          uint32_t addr = entry->ioapic.addr;
          uint32_t gsi_base = id * IOAPIC_MAX_IRQS; /* fake ACPI GSI numbers */
          if (!ioapic_init(id, addr, gsi_base))
            panic("failed to register I/O APIC");
        }
        entry_addr += sizeof(entry->ioapic);
        break;

      /* the following entries are skipped and processed in the second pass */
      case MPCT_TYPE_IO_INTR:
        entry_addr += sizeof(entry->io_intr);
        break;

      case MPCT_TYPE_LOCAL_INTR:
        entry_addr += sizeof(entry->local_intr);
        break;
    }

    entry_addr += sizeof(entry->type);
  }

  /* perform the second pass to register interrupt overrides */
  entry_addr = (uintptr_t) &mpct->entries[0];
  while (entry_addr < entry_end)
  {
    mpct_entry_t *entry = (mpct_entry_t *) entry_addr;
    switch (entry->type)
    {
      case MPCT_TYPE_IO_INTR:
        {
          uint8_t type = entry->io_intr.type;
          uint16_t flags = entry->io_intr.flags;
          uint8_t bus = entry->io_intr.bus;
          uint8_t ioapic = entry->io_intr.ioapic;
          uint8_t intr = entry->io_intr.intr;
          uint8_t line = entry->io_intr.irq;

          /* fake the ACPI GSI number */
          uint32_t gsi = ioapic * IOAPIC_MAX_IRQS + intr;

          if (type == MPCT_IO_INTR_TYPE_NMI)
          {
            irq_tuple_t tuple;
            tuple.type = IRQ_IO;
            tuple.irq = gsi;

            /* TODO: check if this should be the default */
            tuple.active_polarity = POLARITY_HIGH;
            tuple.trigger = TRIGGER_EDGE;

            mpct_flags_to_trigger(&tuple, flags);

            if (!nmi_add(tuple))
              panic("failed to register NMI");
          }
          else if (type == MPCT_IO_INTR_TYPE_INT && isa_bus_found && bus == isa_bus_id)
          {
            if (line >= ISA_INTR_LINES)
              panic("ISA interrupt line out of range: %d", line);

            irq_tuple_t *tuple = isa_irq(line);
            tuple->irq = gsi;

            mpct_flags_to_trigger(tuple, flags);
          }
        }
        entry_addr += sizeof(entry->io_intr);
        break;

      case MPCT_TYPE_LOCAL_INTR:
        {
          uint8_t type = entry->local_intr.type;
          uint16_t flags = entry->local_intr.flags;
          uint8_t bus = entry->local_intr.bus;
          uint8_t line = entry->local_intr.irq;
          uint8_t lapic = entry->local_intr.lapic;
          uint8_t lint = entry->local_intr.lint;

          if (type == MPCT_IO_INTR_TYPE_NMI)
          {
            irq_tuple_t tuple;
            tuple.type = IRQ_LOCAL;
            tuple.local.apic = lapic;
            tuple.local.intn = lint;

            /* TODO: check if this should be the default */
            tuple.active_polarity = POLARITY_HIGH;
            tuple.trigger = TRIGGER_EDGE;

            mpct_flags_to_trigger(&tuple, flags);

            if (lapic == 0xFF)
            {
              /* magic value which indicates NMI is connected to all APICs */
              list_for_each(&cpu_list, node)
              {
                cpu_t *cpu = container_of(node, cpu_t, node);
                tuple.local.apic = cpu->lapic_id;

                if (!nmi_add(tuple))
                  panic("failed to register local NMI");
              }
            }
            else
            {
              if (!nmi_add(tuple))
                panic("failed to register local NMI");
            }
          }
          else if (type == MPCT_IO_INTR_TYPE_INT && isa_bus_found && bus == isa_bus_id)
          {
            if (line >= ISA_INTR_LINES)
              panic("ISA interrupt line out of range: %d", line);

            // TODO consider what we should do in this case
            if (lapic == 0xFF)
              panic("ISA interrupt line hardwired to all local APICs");

            irq_tuple_t *tuple = isa_irq(line);
            tuple->type = IRQ_IO;
            tuple->local.apic = lapic;
            tuple->local.intn = lint;

            mpct_flags_to_trigger(tuple, flags);
          }
        }
        entry_addr += sizeof(entry->local_intr);
        break;

      /* skip entries that were already processed in the first pass */
      case MPCT_TYPE_PROC:
        entry_addr += sizeof(entry->proc);
        break;

      case MPCT_TYPE_BUS:
        entry_addr += sizeof(entry->bus);
        break;

      case MPCT_TYPE_IOAPIC:
        entry_addr += sizeof(entry->ioapic);
        break;
    }

    entry_addr += sizeof(entry->type);
  }

  /* mask the PICs */
  pic_init();

  /* initialise the local APIC */
  xapic_init(mpct->lapic_phy_addr);
}

