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

#ifndef ARC_ACPI_RSDP_H
#define ARC_ACPI_RSDP_H

#include <stdint.h>
#include <arc/acpi/common.h>

#define RSDP_SIGNATURE 0x2052545020445352 /* 'RSD PTR ' */
#define RSDP_ALIGN     16

typedef struct
{
  /* original rsdp structure */
  uint64_t signature;
  uint8_t  checksum;
  char     oem_id[OEM_ID_LEN];
  uint8_t  revision;
  uint32_t rsdt_addr;

  /* extended fields - present if revision >= 2 */
  uint32_t len;
  uint64_t xsdt_addr;
  uint8_t  ext_checksum;
  uint8_t  reserved[3];
} __attribute__((__packed__)) rsdp_t;

rsdp_t *rsdp_scan(void);

#endif
