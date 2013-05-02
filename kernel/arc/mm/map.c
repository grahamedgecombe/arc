/*
 * Copyright (c) 2011-2013 Graham Edgecombe <graham@grahamedgecombe.com>
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
#include <arc/mm/seq.h>
#include <arc/panic.h>
#include <arc/trace.h>
#include <arc/util/container.h>
#include <string.h>
#include <stdbool.h>

static list_t entry_list = LIST_EMPTY;

static int mm_map_entry_compare(const void *left, const void *right)
{
  uintptr_t left_addr = ((mm_map_entry_t *) container_of(left, mm_map_entry_t, node))->addr_start;
  uintptr_t right_addr = ((mm_map_entry_t *) container_of(right, mm_map_entry_t, node))->addr_start;

  if (left_addr < right_addr)
    return -1;
  else if (left_addr == right_addr)
    return 0;
  else
    return 1;
}

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
  mm_map_entry_t *entry = seq_alloc(sizeof(*entry));
  if (!entry)
    panic("failed to allocate memory map entry");

  entry->type = type;
  entry->addr_start = addr_start;
  entry->addr_end = addr_end;
  list_add_tail(&entry_list, &entry->node);
}

/* a rather naive function that sanitizes the memory map */
static void mm_map_sanitize(void)
{
  /* do as many passes as required to fully simplify the map */
next_pass:
  list_for_each(&entry_list, node)
  {
    mm_map_entry_t *entry = container_of(node, mm_map_entry_t, node);

    list_for_each(&entry_list, node)
    {
      mm_map_entry_t *inner_entry = container_of(node, mm_map_entry_t, node);

      /* don't compare an entry with itself */
      if (entry == inner_entry)
        continue;

      /* try to coalesce adjacent entries of the same type */
      if (entry->type == inner_entry->type && (entry->addr_end + 1) == inner_entry->addr_start)
      {
        entry->addr_end = inner_entry->addr_end;
        list_remove(&entry_list, &inner_entry->node);
        goto next_pass;
      }

      /* deal with overlapping entries - higher type ids take precedence */
      if (inner_entry->type >= entry->type)
      {
        /* inner encloses */
        if (entry->addr_start >= inner_entry->addr_start && entry->addr_end <= inner_entry->addr_end)
        {
          list_remove(&entry_list, &entry->node);
          goto next_pass;
        }
        /* inner enclosed */
        else if (inner_entry->addr_start >= entry->addr_start && inner_entry->addr_end <= entry->addr_end)
        {
          /* TODO repurpose an existing entry to save memory? */
          if (entry->addr_start != inner_entry->addr_start)
            mm_map_add(entry->type, entry->addr_start, inner_entry->addr_start - 1);

          if (entry->addr_end != inner_entry->addr_end)
            mm_map_add(entry->type, inner_entry->addr_end + 1, entry->addr_end);

          list_remove(&entry_list, &entry->node);
          goto next_pass;
        }
        /* inner overlaps left side check */
        else if (entry->addr_start >= inner_entry->addr_start && entry->addr_start <= inner_entry->addr_end)
        {
          entry->addr_start = inner_entry->addr_end + 1;
          goto next_pass;
        }
        /* inner overlaps right side check */
        else if (entry->addr_end >= inner_entry->addr_start && entry->addr_end <= inner_entry->addr_end)
        {
          entry->addr_end = inner_entry->addr_start - 1;
          goto next_pass;
        }
      }
    }
  }

  /* sort the entries */
  list_sort(&entry_list, &mm_map_entry_compare);
}

list_t *mm_map_init(multiboot_t *multiboot)
{
  /* find the mmap multiboot tag */
  multiboot_tag_t *mmap_tag = multiboot_get(multiboot, MULTIBOOT_TAG_MMAP);
  if (!mmap_tag)
    panic("no multiboot mmap tag");

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
   * reserve the IVT and BIOS data area - it runs from 0x000-0x4FF inclusive,
   * however, 0x000-0xFFF is reserved as we can't allocate a page from this
   * area anyway as it conflicts with the null pointer value
   *
   * some values from the BDA are useful e.g. we need it to find the EBDA,
   * which can then be used to find the ACPI or MP tables
   */
  mm_map_add(MULTIBOOT_MMAP_RESERVED, 0x000000, 0x000FFF);

  /* reserve kernel memory */
  extern int _start, _end;
  uintptr_t start_addr = (uintptr_t) &_start - VM_KERNEL_IMAGE;
  uintptr_t end_addr   = (uintptr_t) &_end   - VM_KERNEL_IMAGE - 1;
  mm_map_add(MULTIBOOT_MMAP_RESERVED, start_addr, end_addr);

  /* reserve SMP trampoline area */
  extern int trampoline_start, trampoline_end;
  size_t trampoline_len = (uintptr_t) &trampoline_end - (uintptr_t) &trampoline_start;
  mm_map_add(MULTIBOOT_MMAP_RESERVED, 0x001000, 0x001000 + trampoline_len - 1);

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
  list_for_each(&entry_list, node)
  {
    mm_map_entry_t *entry = container_of(node, mm_map_entry_t, node);
    uintptr_t start = entry->addr_start;
    uintptr_t end = entry->addr_end;
    int type = entry->type;
    trace_printf(" => %0#18x -> %0#18x (%s)\n", start, end, mm_map_type_desc(type));
  }

  /* and return a pointer to it */
  return &entry_list;
}
