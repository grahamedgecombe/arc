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

#ifndef ARC_ACPI_MADT_H
#define ARC_ACPI_MADT_H

#include <stdint.h>
#include <arc/pack.h>
#include <arc/acpi/common.h>

#define MADT_SIGNATURE 0x67738041 /* 'APIC' */

#define MADT_FLAGS_PCAT 0x1 /* there are dual-8259 PICs that must be masked */

#define MADT_TYPE_LAPIC      0x00
#define MADT_TYPE_IOAPIC     0x01
#define MADT_TYPE_INTR       0x02
#define MADT_TYPE_NMI        0x03
#define MADT_TYPE_LNMI       0x04
#define MADT_TYPE_LAPIC_ADDR 0x05
#define MADT_TYPE_LX2APIC    0x09
#define MADT_TYPE_LX2NMI     0x0A

#define MADT_LAPIC_FLAGS_ENABLED 0x1

#define MADT_INTR_POLARITY_STD  0x00
#define MADT_INTR_POLARITY_HIGH 0x01
#define MADT_INTR_POLARITY_LOW  0x03
#define MADT_INTR_TRIGGER_STD   0x00
#define MADT_INTR_TRIGGER_EDGE  0x08
#define MADT_INTR_TRIGGER_LEVEL 0x0C

typedef PACK(struct
{
  uint8_t type, len;
  union
  {
    PACK(struct
    {
      uint8_t cpu_id;
      uint8_t lapic_id;
      uint32_t flags;
    }) lapic;

    PACK(struct
    {
      uint8_t ioapic_id;
      uint8_t reserved;
      uint32_t addr;
      uint32_t gsi_base;
    }) ioapic;

    PACK(struct
    {
      uint8_t bus;
      uint8_t irq;
      uint32_t gsi;
      uint16_t flags;
    }) intr;

    PACK(struct
    {
      uint16_t flags;
      uint32_t gsi;
    }) nmi;

    PACK(struct
    {
      uint8_t cpu_id;
      uint16_t flags;
      uint8_t lintn;
    }) lnmi;

    PACK(struct
    {
      uint16_t reserved;
      uint64_t addr;
    }) lapic_addr;

    PACK(struct
    {
      uint16_t reserved;
      uint32_t x2apic_id;
      uint32_t flags;
      uint32_t cpu_uid;
    }) lx2apic;

    PACK(struct
    {
      uint16_t flags;
      uint32_t cpu_uid;
      uint8_t lintn;
      uint8_t reserved[3];
    }) lx2nmi;
  };
}) madt_entry_t;

typedef PACK(struct
{
  acpi_header_t header;
  uint32_t lapic_addr;
  uint32_t flags;
  madt_entry_t entries[1];
}) madt_t;

#endif

