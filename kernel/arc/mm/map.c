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

#include <arc/mm/map.h>
#include <arc/mm/common.h>
#include <arc/mm/phy32.h>
#include <arc/panic.h>
#include <arc/tty.h>
#include <string.h>
#include <stdbool.h>

/* unused type id (prefixed to fit in with other #defines from multiboot.h) */
#define MULTIBOOT_MMAP_UNUSED 0

static mm_map_t map;

static const char *mm_map_type_desc(int type)
{
  switch (type)
  {
    case MULTIBOOT_MMAP_AVAILABLE:
      return "available";
    case MULTIBOOT_MMAP_RESERVED:
      return "reserved";
    case MULTIBOOT_MMAP_ACPI_RECLAIM:
      return "ACPI reclaimable";
    case MULTIBOOT_MMAP_ACPI_NVS:
      return "ACPI NVS";
    case MULTIBOOT_MMAP_BAD:
      return "bad";
    default:
      return "unknown";
  }
}

static void mm_map_add(int type, uintptr_t addr_start, uintptr_t addr_end)
{
  for (size_t id = 0; id < map.count; id++)
  {
    mm_map_entry_t *entry = &map.entries[id];
    if (entry->type == MULTIBOOT_MMAP_UNUSED)
    {
      entry->type = type;
      entry->addr_start = addr_start;
      entry->addr_end = addr_end;
      return;
    }
  }

  if (map.count == MM_MAP_MAX_ENTRIES)
    boot_panic("memory map is full (max entries = %d)", MM_MAP_MAX_ENTRIES);

  mm_map_entry_t *entry = &map.entries[map.count++];
  entry->type = type;
  entry->addr_start = addr_start;
  entry->addr_end = addr_end;
}

/* a rather naive function that sanitizes the memory map */
static void mm_map_sanitize(void)
{
  /* do as many passes as required to fully simplify the map */
next_pass:
  for (size_t id = 0; id < map.count; id++)
  {
    /* get entry, skip if it is unused */
    mm_map_entry_t entry = map.entries[id];
    if (entry.type == MULTIBOOT_MMAP_UNUSED)
      continue;

    for (size_t inner_id = 0; inner_id < map.count; inner_id++)
    {
      /* don't compare an entry with itself */
      if (id == inner_id)
        continue;

      /* get inner entry, skip if it is unused */
      mm_map_entry_t inner_entry = map.entries[inner_id];
      if (inner_entry.type == MULTIBOOT_MMAP_UNUSED)
        continue;

      /* try to coalesce adjacent entries of the same type */
      if (entry.type == inner_entry.type && (entry.addr_end + 1) == inner_entry.addr_start)
      {
        map.entries[id].addr_end = inner_entry.addr_end;
        map.entries[inner_id].type = MULTIBOOT_MMAP_UNUSED;
        goto next_pass;
      }

      /* deal with overlapping entries - higher type ids take precedence */
      if (inner_entry.type >= entry.type)
      {
        /* inner encloses */
        if (entry.addr_start >= inner_entry.addr_start && entry.addr_end <= inner_entry.addr_end)
        {
          map.entries[id].type = MULTIBOOT_MMAP_UNUSED;
          goto next_pass;
        }
        /* inner enclosed */
        else if (inner_entry.addr_start >= entry.addr_start && inner_entry.addr_end <= entry.addr_end)
        {
          if (entry.addr_start != inner_entry.addr_start)
            mm_map_add(entry.type, entry.addr_start, inner_entry.addr_start - 1);

          if (entry.addr_end != inner_entry.addr_end)
            mm_map_add(entry.type, inner_entry.addr_end + 1, entry.addr_end);

          map.entries[id].type = MULTIBOOT_MMAP_UNUSED;
          goto next_pass;
        }
        /* inner overlaps left side check */
        else if (entry.addr_start >= inner_entry.addr_start && entry.addr_start <= inner_entry.addr_end)
        {
          map.entries[id].addr_start = inner_entry.addr_end + 1;
          goto next_pass;
        }
        /* inner overlaps right side check */
        else if (entry.addr_end >= inner_entry.addr_start && entry.addr_end <= inner_entry.addr_end)
        {
          map.entries[id].addr_end = inner_entry.addr_start - 1;
          goto next_pass;
        }
      }
    }
  }

  /* delete unused entries by making a copy then re-adding them all */
  mm_map_t tmp_map;
  memcpy(&tmp_map, &map, sizeof(map));

  map.count = 0;

  for (size_t id = 0; id < tmp_map.count; id++)
  {
    mm_map_entry_t *entry = &tmp_map.entries[id];
    if (entry->type != MULTIBOOT_MMAP_UNUSED)
      mm_map_add(entry->type, entry->addr_start, entry->addr_end);
  }

  /* bail out right now if a sort isn't required */
  if (map.count <= 1)
    return;

  /* sort the entries */
  for (;;)
  {
    /* a flag that indicates if any swaps were performed this pass */
    bool swap = false;

    /* iterate through the list swapping adjacent pairs if required */
    for (size_t id = 0; id < map.count - 1; id++)
    {
      if (map.entries[id].addr_start > map.entries[id + 1].addr_start)
      {
        mm_map_entry_t tmp = map.entries[id];
        map.entries[id] = map.entries[id + 1];
        map.entries[id + 1] = tmp;
        swap = true;
      }
    }

    /* stop when the list is sorted */
    if (!swap)
      break;
  }
}

mm_map_t *mm_map_init(multiboot_t *multiboot)
{
  /* find the mmap multiboot tag */
  multiboot_tag_t *mmap_tag = multiboot_get(multiboot, MULTIBOOT_TAG_MMAP);
  if (!mmap_tag)
    boot_panic("no multiboot mmap tag");

  /* read entries from the e820 map given to us by GRUB */
  uintptr_t entry_addr = (uintptr_t) mmap_tag + sizeof(mmap_tag->type)
    + sizeof(mmap_tag->size) + sizeof(mmap_tag->mmap.entry_size)
    + sizeof(mmap_tag->mmap.entry_version);

  uintptr_t entry_limit = (uintptr_t) mmap_tag + mmap_tag->size;

  while (entry_addr < entry_limit)
  {
    multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *) entry_addr;
    if (entry->length > 0)
      mm_map_add(entry->type, entry->base_addr,
        entry->base_addr + entry->length - 1);
    entry_addr += mmap_tag->mmap.entry_size;
  }

  /*
   * reserve the IVT and BIOS data area - we can't allocate a page from this
   * anyway as it conflicts with the null pointer value
   */
  mm_map_add(MULTIBOOT_MMAP_RESERVED, 0x000000, 0x0004FF);

  /* reserve kernel memory */
  extern int _start, _end;
  uintptr_t start_addr = (uintptr_t) &_start - VM_OFFSET;
  uintptr_t end_addr   = (uintptr_t) &_end   - VM_OFFSET - 1;
  mm_map_add(MULTIBOOT_MMAP_RESERVED, start_addr, end_addr);

  /* reserve multiboot information structure memory */
  start_addr = (uintptr_t) multiboot - PHY32_OFFSET;
  end_addr   = start_addr + multiboot->total_size;
  mm_map_add(MULTIBOOT_MMAP_RESERVED, start_addr, end_addr);

  /* reserve multiboot module(s) memory */
  multiboot_tag_t *mod_tag = multiboot_get(multiboot, MULTIBOOT_TAG_MODULE);
  while (mod_tag)
  {
    mm_map_add(MULTIBOOT_MMAP_RESERVED, mod_tag->module.mod_start, mod_tag->module.mod_end - 1);
    mod_tag = multiboot_get_after(multiboot, mod_tag, MULTIBOOT_TAG_MODULE);
  }

  /* fix memory map */
  mm_map_sanitize();

  /* print the final map */
  for (size_t id = 0; id < map.count; id++)
  {
    uintptr_t start = map.entries[id].addr_start;
    uintptr_t end = map.entries[id].addr_end;
    int type = map.entries[id].type;
    tty_printf(" => %0#18x -> %0#18x (%s)\n", start, end, mm_map_type_desc(type));
  }

  /* and return a pointer to it */
  return &map;
}

