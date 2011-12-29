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
#include <stdbool.h>

/* virtual memory offset for transforming _start/_end to physical addresses */
#define VM_OFFSET 0xFFFF800000000000

static int mm_map_count = 0;
static mm_map_entry_t mm_map_entries[MM_MAP_MAX_ENTRIES];

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
  mm_map_entries[mm_map_count].type = type;
  mm_map_entries[mm_map_count].addr_start = addr_start;
  mm_map_entries[mm_map_count++].addr_end = addr_end;
}

static void mm_map_remove(int id)
{
  int shift = mm_map_count - 1 - id;
  memmove(&mm_map_entries[id], &mm_map_entries[id + 1], sizeof(mm_map_entries) * shift);
  mm_map_count--;
}

static void mm_map_sanitize(void)
{
  /* we don't need to do anything if there are zero or one entries */
  if (mm_map_count <= 1)
    return;

  /* create a temporary, read-only copy of the original map */
  int tmp_count = mm_map_count;
  static mm_map_entry_t tmp_map[MM_MAP_MAX_ENTRIES];
  memcpy(tmp_map, mm_map_entries, sizeof(mm_map_entries));

  /* fix overlapping entries */
  for (int id = 0; id < tmp_count; id++)
  {
    uintptr_t start = tmp_map[id].addr_start;
    uintptr_t end = tmp_map[id].addr_end;
    int type = tmp_map[id].type;

    for (int inner_id = 0; inner_id < tmp_count; inner_id++)
    {
      uintptr_t inner_start = tmp_map[inner_id].addr_start;
      uintptr_t inner_end = tmp_map[inner_id].addr_end;
      int inner_type = tmp_map[inner_id].type;

      /*
       * higher type ids override lower ids (implemented like this in Linux),
       * also ignore ourself
       */
      if (inner_type <= type || id == inner_id)
        continue;

      /*
       * check for overlaps
       *
       * outer overlaps all of the inner condition:
       *
       *    s      e
       *    v      v
       *   -11111111-
       *   ---2222---
       *      ^  ^
       *     is  ie
       *
       * outer overlaps inner end condition:
       *       s  e
       *       v  v
       *   ----1111--
       *   --2222----
       *     ^  ^
       *    is  ie
       *
       * outer overlaps inner start condition:
       *
       *     s  e
       *     v  v
       *   --1111----
       *   ----2222--
       *       ^  ^
       *      is  ie
       */
      bool outer_overlaps_inner_end = inner_end >= start && inner_end <= end;
      bool outer_overlaps_inner_start = inner_start >= start && inner_start <= end;

      /* fix the situation where both conditions are true */
      if (outer_overlaps_inner_end && outer_overlaps_inner_start)
      {
        /* mark the outer entry as a candidate for deletion */
        mm_map_entries[id].type = 0;

        /* create a new 'prefix' outer entry */
        if (start != inner_start && start != (inner_start - 1))
          mm_map_add(type, start, inner_start - 1);

        /* create a new 'postfix' outer entry */
        if (end != inner_end && end != (inner_end + 1))
          mm_map_add(type, inner_end + 1, end);
      }
      /* fix the first kind of overlap */
      else if (outer_overlaps_inner_end)
      {
        mm_map_entries[id].addr_start = mm_map_entries[inner_id].addr_end + 1;
      }
      /* fix the second kind of overlap */
      else if (outer_overlaps_inner_start)
      {
        mm_map_entries[id].addr_end = mm_map_entries[inner_id].addr_start - 1;
      }
    }
  }

  /*
   * remove entries where the length is zero or if they have been marked as a
   * candidate for deletion
   */
  for (int id = tmp_count; id > 0; id--)
  {
    mm_map_entry_t *entry = &mm_map_entries[id - 1];
    if (entry->type == 0 || entry->addr_start == entry->addr_end)
      mm_map_remove(id - 1);
  }

  /* if there are one or less entries we don't need to coalesce */
  if (mm_map_count <= 1)
    return;

  /* make another copy of the temporary map */
  tmp_count = mm_map_count;
  memcpy(tmp_map, mm_map_entries, sizeof(mm_map_entries));

  /* coalesce adjacent entries */
  for (int id = 0; id < tmp_count; id++)
  {
    uintptr_t end = tmp_map[id].addr_end;
    int type = tmp_map[id].type;

    for (int inner_id = 0; inner_id < tmp_count; inner_id++)
    {
      uintptr_t inner_start = tmp_map[inner_id].addr_start;
      uintptr_t inner_end = tmp_map[inner_id].addr_end;
      int inner_type = tmp_map[inner_id].type;

      /* ignore entries not of the same type or the same entry */
      if (type != inner_type || id == inner_id)
        continue;

      /* check for adjacency and merge adjacent entries */
      if ((end + 1) == inner_start)
      {
        mm_map_entries[inner_id].type = 0;
        mm_map_entries[id].addr_end = inner_end;
      }
    }
  }

  /* if there are one or less entries we don't need to coalesce */
  if (mm_map_count <= 1)
    return;

  /*
   * remove entries if they have been marked as a candidate for deletion
   */
  for (int id = tmp_count; id > 0; id--)
  {
    mm_map_entry_t *entry = &mm_map_entries[id - 1];
    if (entry->type == 0)
      mm_map_remove(id - 1);
  }


  /* if there are one or less entries we don't need to sort */
  if (mm_map_count <= 1)
    return;

  /* sort all the entries */
  for (;;)
  {
    /* indicates if any swaps were made in this pass */
    bool swapped = false;

    /* loop through all pairs of entries */
    for (int id = 0; id < mm_map_count - 1; id++)
    {
      /* check if we need to swap this pair */
      uintptr_t first = mm_map_entries[id].addr_start;
      uintptr_t second = mm_map_entries[id + 1].addr_start;
      if (first > second)
      {
        /* actually swap them */
        mm_map_entry_t tmp;
        memcpy(&tmp, &mm_map_entries[id], sizeof(tmp));
        memcpy(&mm_map_entries[id], &mm_map_entries[id + 1], sizeof(tmp));
        memcpy(&mm_map_entries[id + 1], &tmp, sizeof(tmp));
        swapped = true;
      }
    }

    /* if no swaps were made we can break out of the loop right now */
    if (!swapped)
      break;
  }
}

void mm_map_init(multiboot_t *multiboot)
{
  /* let's go! */
  tty_puts("Mapping memory...\n");

  /* read entries from the e820 map */
  uint32_t mmap_addr = multiboot->mmap_addr;
  while (mmap_addr < (multiboot->mmap_addr + multiboot->mmap_len))
  {
    multiboot_mmap_t *mmap = (multiboot_mmap_t *) aphy32_to_virt(mmap_addr);
    mm_map_add(mmap->type, mmap->addr, mmap->addr + mmap->len - 1);
    mmap_addr += mmap->size + sizeof(mmap->size);
  }

  /* reserve kernel memory */
  extern int _start, _end; /* these are placed at the start and end of the kernel image */
  uintptr_t start_addr = (uintptr_t) &_start - VM_OFFSET;
  uintptr_t end_addr   = (uintptr_t) &_end   - VM_OFFSET - 1;
  mm_map_add(MULTIBOOT_MMAP_RESERVED, start_addr, end_addr);

  /* fix memory map */
  mm_map_sanitize();

  /* print the final map */
  for (int id = 0; id < mm_map_count; id++)
  {
    uintptr_t start = mm_map_entries[id].addr_start;
    uintptr_t end = mm_map_entries[id].addr_end;
    int type = mm_map_entries[id].type;
    tty_printf(" => 0x%016x -> 0x%016x (%s)\n", start, end, mm_map_type_desc(type));
  }
}

