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

#ifndef ARC_MM_VMM_H
#define ARC_MM_VMM_H

#include <arc/mm/common.h>
#include <stdbool.h>
#include <stdint.h>

void vmm_init(void);

void vmm_touch(uintptr_t virt, int size);

bool vmm_map(uintptr_t virt, uintptr_t phy, uint64_t flags);
bool vmm_maps(uintptr_t virt, uintptr_t phy, uint64_t flags, int size);

void vmm_unmap(uintptr_t virt);
void vmm_unmaps(uintptr_t virt, int size);

#endif

