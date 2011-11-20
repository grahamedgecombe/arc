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

#ifndef _ARC_MULTIBOOT
#define _ARC_MULTIBOOT

#include <arc/pack.h>
#include <stdint.h>

/* the multiboot magic number */
#define MULTIBOOT_MAGIC 0x2BADB002

/* indicates basic lower/upper memory info is available */
#define MULTIBOOT_FLAGS_MEM     0x1

/* indicates the boot device is available */
#define MULTIBOOT_FLAGS_BOOTDEV 0x2 

/* indicates the command line is available */
#define MULTIBOOT_FLAGS_CMDLINE 0x4

/* indicates that modules are available */
#define MULTIBOOT_FLAGS_MODULES 0x8

/* indicates that ELF symbols are available */
#define MULTIBOOT_FLAGS_ELF_SYM 0x20

/* indicates that a memory map is available */
#define MULTIBOOT_FLAGS_MMAP    0x40

/* indicates if the memory is available */
#define MULTIBOOT_MMAP_AVAILABLE 1

/* indicates if the memory is reserved */
#define MULTIBOOT_MMAP_RESERVED  2

/* multiboot information structure */
typedef PACK(struct
{
  /* multiboot flags */
  uint32_t flags;

  /* info from the BIOS */
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;

  /* kernel command line and modules */
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;

  /* kernel ELF symbols */
  uint32_t elf_num;
  uint32_t elf_size;
  uint32_t elf_addr;
  uint32_t elf_shndx;

  /* memory map */
  uint32_t mmap_len;
  uint32_t mmap_addr;
}) multiboot_t;

/* multiboot memory map structure */
typedef PACK(struct
{
  /* size of this entry */
  uint32_t size;

  /* address and length of the memory */
  uint64_t addr;
  uint64_t len;

  /* type of memory (available=1) */
  uint32_t type;
}) multiboot_mmap_t;

/* multiboot module structure */
typedef PACK(struct
{
  /* memory goes from mod_start to mod_end-1 inclusive */
  uint32_t mod_start;
  uint32_t mod_end;

  /* the command line */
  uint32_t cmdline;

  /* padding - must be zero */
  uint32_t pad;
}) multiboot_mod_t;

#endif

