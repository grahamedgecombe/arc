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

#include <arc/mm/map.h>
#include <arc/mm/phy32.h>
#include <arc/tty.h>
#include <string.h>

/* virtual memory offset for transforming _start/_end to physical addresses */
#define VM_OFFSET 0xFFFF800000000000

static const char *mm_map_type_desc(int type)
{
  switch (type)
  {
    case MULTIBOOT_MMAP_AVAILABLE:
      return "available";
    case MULTIBOOT_MMAP_RESERVED:
      return "reserved";
    default:
      return "unknown";
  }
}

static void mm_map_append(int type, uintptr_t addr, uintptr_t len)
{
  /* actually add the entry to the table */
  mm_map_entries[mm_map_count].type = type;
  mm_map_entries[mm_map_count].addr = addr;
  mm_map_entries[mm_map_count++].len = len;

  /* print out the entry */
  tty_printf("   => 0x%016x -> 0x%016x (%s)\n", addr, addr + len, mm_map_type_desc(type));
}

static void mm_map_sanitize(void)
{
  /* we don't need to do anything if there are zero or one entries */
  if (mm_map_count <= 1)
    return;

  /* create a temporary copy of the original map */
  static mm_map_entry_t tmp_map[MM_MAP_MAX_ENTRIES];
  memcpy(tmp_map, mm_map_entries, sizeof(mm_map_entries));
}

void mm_map_init(multiboot_t *multiboot)
{
  /* let's go! */
  tty_puts("Mapping memory...\n");

  /* read entries from the e820 map */
  tty_puts("  BIOS e820 memory map:\n");
  uint32_t mmap_addr = multiboot->mmap_addr;
  while (mmap_addr < (multiboot->mmap_addr + multiboot->mmap_len))
  {
    multiboot_mmap_t *mmap = (multiboot_mmap_t *) aphy32_to_virt(mmap_addr);
    mm_map_append(mmap->type, mmap->addr, mmap->len);
    mmap_addr += mmap->size + sizeof(mmap->size);
  }

  /* reserve kernel memory */
  tty_puts("  Static kernel memory map:\n");
  extern int _start, _end; /* these are placed at the start and end of the kernel image */
  uintptr_t start_addr = (uintptr_t) &_start - VM_OFFSET;
  uintptr_t end_addr   = (uintptr_t) &_end   - VM_OFFSET;
  mm_map_append(MULTIBOOT_MMAP_RESERVED, start_addr, end_addr - start_addr);

  /* fix memory map */
  tty_puts("Sanitizing memory map...\n");
  mm_map_sanitize();
}

