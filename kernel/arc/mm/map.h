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

#ifndef ARC_MM_MAP_H
#define ARC_MM_MAP_H

#include <stddef.h>
#include <stdint.h>
#include <arc/multiboot.h>

/* 
 * Doing the memory map allocation dynamically is actually quite difficult
 * early during boot, especially with the way the Multiboot structures are
 * layed out (it can be absolutely anywhere in the first 4G). For simplicity
 * the physical memory map is allocated statically with this maximum size
 * instead.
 *
 * You may need to adjust this if the e820 table created by your BIOS is
 * unusually large.
 */
#define MM_MAP_MAX_ENTRIES 32

typedef struct
{
  int type;
  uintptr_t addr_start;
  uintptr_t addr_end;
} mm_map_entry_t;

typedef struct
{
  size_t count;
  mm_map_entry_t entries[MM_MAP_MAX_ENTRIES];
} mm_map_t;

mm_map_t *mm_map_init(multiboot_t *multiboot);

#endif

