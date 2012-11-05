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

#include <arc/multiboot.h>

multiboot_tag_t *multiboot_get(multiboot_t *multiboot, uint32_t type)
{
  return multiboot_get_after(multiboot, 0, type);
}

multiboot_tag_t *multiboot_get_after(multiboot_t *multiboot, multiboot_tag_t *start, uint32_t type)
{
  /* find the base tag */
  uintptr_t tag_base = (uintptr_t) multiboot + sizeof(multiboot->total_size)
    + sizeof(multiboot->reserved);

  /* find the address of the tag to start searching at */
  uintptr_t tag_addr;
  if (start)
  {
    uintptr_t size = MULTIBOOT_ALIGN(start->size);
    tag_addr = (uintptr_t) start + size;
  }
  else
  {
    tag_addr = tag_base;
  }

  /* calculate where the end of the structure is */
  uintptr_t tag_limit = tag_addr + multiboot->total_size;

  /* loop through the tags */
  while (tag_addr < tag_limit)
  {
    /* check for the terminator */
    multiboot_tag_t *tag = (multiboot_tag_t *) tag_addr;
    if (tag->type == MULTIBOOT_TAG_TERMINATOR)
      return 0;

    /* check if this is the tag we are looking for */
    if (tag->type == type)
      return tag;

    /* now look at the next tag */
    uintptr_t size = MULTIBOOT_ALIGN(tag->size);
    tag_addr += size;
  }

  return 0;
}
