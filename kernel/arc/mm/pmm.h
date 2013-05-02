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

#ifndef ARC_MM_PMM_H
#define ARC_MM_PMM_H

#include <arc/util/list.h>
#include <stdint.h>

/* where the stacks start in virtual memory */
#define VM_STACK_OFFSET 0xFFFFFEFEFFFF7000

/* the ids of the memory zones */
#define ZONE_DMA   0
#define ZONE_DMA32 1
#define ZONE_STD   2
#define ZONE_COUNT 3

/* the limits of the DMA and DMA32 memory zones */
#define ZONE_LIMIT_DMA   0xFFFFFF   /* 2^24 - 1 */
#define ZONE_LIMIT_DMA32 0xFFFFFFFF /* 2^32 - 1 */

void pmm_init(list_t *map);
uintptr_t pmm_alloc(void);
uintptr_t pmm_allocs(int size);
uintptr_t pmm_allocz(int zone);
uintptr_t pmm_allocsz(int size, int zone);
void pmm_free(uintptr_t addr);
void pmm_frees(int size, uintptr_t addr);

#endif
