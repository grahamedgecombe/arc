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

#ifndef ARC_MULTIBOOT_H
#define ARC_MULTIBOOT_H

#include <stdint.h>

/* the multiboot magic number */
#define MULTIBOOT_MAGIC 0x36D76289

/* the multiboot info tag numbers */
#define MULTIBOOT_TAG_TERMINATOR 0
#define MULTIBOOT_TAG_CMDLINE    1
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

/* align the given address up to the next 8 byte boundary */
#define MULTIBOOT_ALIGN(x) (((x) + 7) & 0xFFFFFFFFFFFFFFF8)

/* multiboot information structure */
typedef struct
{
  uint32_t total_size;
  uint32_t reserved;
} __attribute__((__packed__)) multiboot_t;

typedef struct
{
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t reserved;
} __attribute__((__packed__)) multiboot_mmap_entry_t;

/* multiboot tag structure */
typedef struct
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
    } __attribute__((__packed__)) mmap;

    /* module tag */
    struct
    {
      uint32_t mod_start;
      uint32_t mod_end;
      char string[1];
    } __attribute__((__packed__)) module;

    /* cmdline tag */
    struct
    {
      char string[1];
    } __attribute__((__packed__)) cmdline;

    /* elf tag */
    struct
    {
      uint32_t size;
      uint16_t sh_num;
      uint16_t sh_entsize;
      uint16_t sh_shstrndx;
      uint16_t reserved;
      char data[1];
    } __attribute__((__packed__)) elf;
  };
} __attribute__((__packed__)) multiboot_tag_t;

multiboot_tag_t *multiboot_get(multiboot_t *multiboot, uint32_t type);
multiboot_tag_t *multiboot_get_after(multiboot_t *multiboot, multiboot_tag_t *start, uint32_t type);

#endif
