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

#include <arc/acpi/scan.h>
#include <arc/acpi/rsdp.h>
#include <arc/acpi/rsdt.h>
#include <arc/acpi/xsdt.h>
#include <arc/acpi/fadt.h>
#include <arc/mm/mmio.h>
#include <arc/panic.h>
#include <arc/tty.h>
#include <stddef.h>

static acpi_header_t *acpi_map(uintptr_t addr)
{
  acpi_header_t *table = mmio_map(addr, sizeof(*table));
  if (!table)
    return 0;

  uint32_t len = table->len;
  mmio_unmap(table, sizeof(*table));
  return mmio_map(addr, len);
}

static void acpi_print(acpi_header_t *table, uintptr_t addr)
{
  /* convert the signature to a null-terminated string */
  char sig[5];
  sig[0] = table->signature & 0xFF;
  sig[1] = (table->signature >> 8) & 0xFF;
  sig[2] = (table->signature >> 16) & 0xFF;
  sig[3] = (table->signature >> 24) & 0xFF;
  sig[4] = 0;

  /* print some information about the structure */
  tty_printf(" => Found %s (at %0#18x) \n", sig, addr);
}

static void acpi_scan_table(uintptr_t addr)
{
  acpi_header_t *table = acpi_map(addr); 
  if (!table)
    boot_panic("couldn't map table");

  /* print some information about the structure */
  acpi_print(table, addr);

  /* look up the FACS and DSDT if this table is the FADT */
  if (table->signature == FADT_SIGNATURE)
  {
    fadt_t *fadt = (fadt_t *) table;

    if (fadt->facs_addr)
      acpi_scan_table(fadt->facs_addr);

    acpi_scan_table(fadt->dsdt_addr);
  }
}

bool acpi_scan(void)
{
  /* try to find the RSDP structure */
  rsdp_t *rsdp = rsdp_scan();
  if (!rsdp)
  {
    tty_puts(" => ACPI not supported\n");
    return false;
  }

  /* check the ACPI revision */
  if (rsdp->revision >= 2)
  {
    /* map it into virtual memory */
    xsdt_t *xsdt = (xsdt_t *) acpi_map(rsdp->xsdt_addr);
    if (!xsdt)
      boot_panic("couldn't map XSDT");

    /* print some info about the XSDT */
    acpi_print((acpi_header_t *) xsdt, rsdp->xsdt_addr);

    /* calculate the number of entries */
    size_t len = (xsdt->header.len - sizeof(xsdt->header)) / sizeof(xsdt->entries[0]);

    /* iterate through all of the tables */
    for (size_t i = 0; i < len; i++)
      acpi_scan_table((uintptr_t) xsdt->entries[i]);
  }
  else
  {
    /* map it into virtual memory */
    rsdt_t *rsdt = (rsdt_t *) acpi_map(rsdp->rsdt_addr);
    if (!rsdt)
      boot_panic("couldn't map RSDT");

    /* print some info about the RSDT */
    acpi_print((acpi_header_t *) rsdt, rsdp->rsdt_addr);

    /* calculate the number of entries */
    size_t len = (rsdt->header.len - sizeof(rsdt->header)) / sizeof(rsdt->entries[0]);

    /* iterate through all of the tables */
    for (size_t i = 0; i < len; i++)
      acpi_scan_table((uintptr_t) rsdt->entries[i]);
  }

  return true;
}

