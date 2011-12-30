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
#include <arc/panic.h>
#include <arc/tty.h>
#include <string.h>
#include <stdbool.h>

/* virtual memory offset for transforming _start/_end to physical addresses */
#define VM_OFFSET 0xFFFF800000000000

/* unused type id (prefixed to fit in with other #defines from multiboot.h) */
#define MULTIBOOT_MMAP_UNUSED 0

static int entry_count = 0;
static mm_map_entry_t entries[MM_MAP_MAX_ENTRIES];

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

static void mm_map_add(int type, uintptr_t addr_start, uintptr_t addr_end)
{
  for (int id = 0; id < entry_count; id++)
  {
    mm_map_entry_t *entry = &entries[id];
    if (entry->type == MULTIBOOT_MMAP_UNUSED)
    {
      entry->type = type;
      entry->addr_start = addr_start;
      entry->addr_end = addr_end;
      return;
    }
  }

  if (entry_count == MM_MAP_MAX_ENTRIES)
    boot_panic("memory map is full (max entries = %d)", MM_MAP_MAX_ENTRIES);

  mm_map_entry_t *entry = &entries[entry_count++];
  entry->type = type;
  entry->addr_start = addr_start;
  entry->addr_end = addr_end;
}

/* a rather naive function that sanitizes the memory map */
static void mm_map_sanitize(void)
{
  /* do as many passes as required to fully simplify the map */
next_pass:
  for (int id = 0; id < entry_count; id++)
  {
    /* get entry, skip if it is unused */
    mm_map_entry_t entry = entries[id];
    if (entry.type == MULTIBOOT_MMAP_UNUSED)
      continue;

    for (int inner_id = 0; inner_id < entry_count; inner_id++)
    {
      /* don't compare an entry with itself */
      if (id == inner_id)
        continue;

      /* get inner entry, skip if it is unused */
      mm_map_entry_t inner_entry = entries[inner_id];
      if (inner_entry.type == MULTIBOOT_MMAP_UNUSED)
        continue;

      /* try to coalesce adjacent entries of the same type */
      if (entry.type == inner_entry.type && (entry.addr_end + 1) == inner_entry.addr_start)
      {
        entries[id].addr_end = inner_entry.addr_end;
        entries[inner_id].type = MULTIBOOT_MMAP_UNUSED;
        goto next_pass;
      }

      /* deal with overlapping entries - higher type ids take precedence */
      if (inner_entry.type >= entry.type)
      {
        /* inner encloses */
        if (entry.addr_start >= inner_entry.addr_start && entry.addr_end <= inner_entry.addr_end)
        {
          entries[id].type = MULTIBOOT_MMAP_UNUSED;
          goto next_pass;
        }
        /* inner enclosed */
        else if (inner_entry.addr_start >= entry.addr_start && inner_entry.addr_end <= entry.addr_end)
        {
          if (entry.addr_start != inner_entry.addr_start)
            mm_map_add(entry.type, entry.addr_start, inner_entry.addr_start - 1);

          if (entry.addr_end != inner_entry.addr_end)
            mm_map_add(entry.type, inner_entry.addr_end + 1, entry.addr_end);

          entries[id].type = MULTIBOOT_MMAP_UNUSED;
          goto next_pass;
        }
        /* inner overlaps left side check */
        else if (entry.addr_start >= inner_entry.addr_start && entry.addr_start <= inner_entry.addr_end)
        {
          entries[id].addr_start = inner_entry.addr_end + 1;
          goto next_pass;
        }
        /* inner overlaps right side check */
        else if (entry.addr_end >= inner_entry.addr_start && entry.addr_end <= inner_entry.addr_end)
        {
          entries[id].addr_end = inner_entry.addr_start - 1;
          goto next_pass;
        }
      }
    }
  }

  /* delete unused entries by making a copy then re-adding them all */
  mm_map_entry_t tmp_entries[MM_MAP_MAX_ENTRIES];
  memcpy(tmp_entries, entries, sizeof(entries));

  int tmp_entry_count = entry_count;
  entry_count = 0;

  for (int id = 0; id < tmp_entry_count; id++)
  {
    mm_map_entry_t *entry = &tmp_entries[id];
    if (entry->type != MULTIBOOT_MMAP_UNUSED)
      mm_map_add(entry->type, entry->addr_start, entry->addr_end);
  }

  /* bail out right now if a sort isn't required */
  if (entry_count <= 1)
    return;

  /* sort the entries */
  for (;;)
  {
    /* a flag that indicates if any swaps were performed this pass */
    bool swap = false;

    /* iterate through the list swapping adjacent pairs if required */
    for (int id = 0; id < entry_count - 1; id++)
    {
      if (entries[id].addr_start > entries[id + 1].addr_start)
      {
        mm_map_entry_t tmp = entries[id];
        entries[id] = entries[id + 1];
        entries[id + 1] = tmp;
        swap = true;
      }
    }

    /* stop when the list is sorted */
    if (!swap)
      break;
  }
}

void mm_map_init(multiboot_t *multiboot)
{
  /* let's go! */
  tty_puts("Mapping physical memory...\n");

  /* read entries from the e820 map given to us by GRUB */
  uint32_t mmap_addr = multiboot->mmap_addr;
  while (mmap_addr < (multiboot->mmap_addr + multiboot->mmap_len))
  {
    multiboot_mmap_t *mmap = (multiboot_mmap_t *) aphy32_to_virt(mmap_addr);
    if (mmap->len > 0)
      mm_map_add(mmap->type, mmap->addr, mmap->addr + mmap->len - 1);
    mmap_addr += mmap->size + sizeof(mmap->size);
  }

  /*
   * reserve the IVT and BIOS data area - we can't allocate a page from this
   * anyway as it conflicts with the null pointer value
   */
  mm_map_add(MULTIBOOT_MMAP_RESERVED, 0x000000, 0x0004FF);

  /*
   * reserve the video RAM and ROM area, QEMU and Bochs don't seem to include
   * this but it allows us to coalesce it with the adjacent entries (which
   * cover the EBDA and kernel)
   */
  mm_map_add(MULTIBOOT_MMAP_RESERVED, 0x0A0000, 0x0FFFFF);

  /* reserve kernel memory */
  extern int _start, _end;
  uintptr_t start_addr = (uintptr_t) &_start - VM_OFFSET;
  uintptr_t end_addr   = (uintptr_t) &_end   - VM_OFFSET - 1;
  mm_map_add(MULTIBOOT_MMAP_RESERVED, start_addr, end_addr);

  /* reserve multiboot module(s) memory */
  uint32_t mod_addr = multiboot->mods_addr;
  for (int id = 0; id < multiboot->mods_count; id++)
  {
    multiboot_mod_t *mod = (multiboot_mod_t *) aphy32_to_virt(mod_addr);
    mm_map_add(MULTIBOOT_MMAP_RESERVED, mod->start, mod->end - 1);
    mod_addr += sizeof(*mod);
  }

  /* fix memory map */
  mm_map_sanitize();

  /* print the final map */
  for (int id = 0; id < entry_count; id++)
  {
    uintptr_t start = entries[id].addr_start;
    uintptr_t end = entries[id].addr_end;
    int type = entries[id].type;
    tty_printf(" => %0#18x -> %0#18x (%s)\n", start, end, mm_map_type_desc(type));
  }
}

