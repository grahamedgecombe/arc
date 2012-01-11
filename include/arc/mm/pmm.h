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

#ifndef ARC_MM_PMM_H
#define ARC_MM_PMM_H

#include <arc/mm/map.h>
#include <arc/pack.h>

/* where the stacks start in virtual memory */
#define VM_STACK_OFFSET 0xFFFFFF7EFFFFF000

/* the number of entries in a PMM stack page */
#define PMM_STACK_SIZE 510

/* the structure of a PMM stack page */
typedef PACK(struct pmm_stack
{
  uint64_t next_stack;
  uint64_t count;
  uint64_t stack[PMM_STACK_SIZE];
}) pmm_stack_t;

void pmm_init(mm_map_t *map);
void *pmm_alloc(void);
void pmm_free(void *ptr);

#endif

