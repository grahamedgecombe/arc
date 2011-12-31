/*
 * Copyright (c) 2011 Graham Edgecombe <graham@grahamedgecombe.com>
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

#ifndef ARC_MULTIBOOT_H
#define ARC_MULTIBOOT_H

#include <arc/pack.h>
#include <stdint.h>

/* the multiboot magic number */
#define MULTIBOOT_MAGIC 0x36D76289

/* the multiboot info tag numbers */
#define MULTIBOOT_TAG_TERMINATOR 0
#define MULTIBOOT_TAG_CMD_LINE   1
#define MULTIBOOT_TAG_BOOT_LDR   2
#define MULTIBOOT_TAG_MODULE     3
#define MULTIBOOT_TAG_MEM        4
#define MULTIBOOT_TAG_BOOT_DEV   5
#define MULTIBOOT_TAG_MMAP       6
#define MULTIBOOT_TAG_VBE        7
#define MULTIBOOT_TAG_FRAMEBUF   8
#define MULTIBOOT_TAG_ELF        9
#define MULTIBOOT_TAG_APM        10

/* indicates the type of a memory region */
#define MULTIBOOT_MMAP_AVAILABLE    1
#define MULTIBOOT_MMAP_RESERVED     2
#define MULTIBOOT_MMAP_ACPI_RECLAIM 3
#define MULTIBOOT_MMAP_ACPI_NVS     4
#define MULTIBOOT_MMAP_BAD          5

/* multiboot information structure */
typedef PACK(struct
{
  uint32_t total_size;
  uint32_t reserved;
}) multiboot_t;

typedef PACK(struct
{
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t reserved;
}) multiboot_mmap_entry_t;

/* multiboot tag structure */
typedef PACK(struct
{
  /* general information */
  uint32_t type;
  uint32_t size;

  /* type-specific information */
  union
  {
    /* mmap tag */
    struct
    {
      uint32_t entry_size;
      uint32_t entry_version;
      /* entries follow here */
    } mmap;

    /* module tag */
    struct
    {
      uint32_t mod_start;
      uint32_t mod_end;
      char string[1];
    } module;
  };
}) multiboot_tag_t;

multiboot_tag_t *multiboot_get(multiboot_t *multiboot, uint32_t type);
multiboot_tag_t *multiboot_get_after(multiboot_t *multiboot, multiboot_tag_t *start, uint32_t type);

#endif

