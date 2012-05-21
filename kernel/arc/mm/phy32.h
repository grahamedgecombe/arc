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

#ifndef ARC_MM_PHY32_H
#define ARC_MM_PHY32_H

#include <stdint.h>

#define PHY32_OFFSET 0xFFFFFF7F00000000

/*
 * The kernel maps the first 4 gigabytes of physical memory into virtual
 * memory, this vastly simplifies dealing with the multiboot and ACPI
 * structures, as the RSDP and multiboot structures are always within the
 * 32-bit physical address space.
 *
 * This function converts a physical 32-bit pointer into a virtual 64-bit
 * pointer.
 */
void *phy32_to_virt(void *ptr);

/* Identical to the above, but works with integers instead, to avoid casts. */
uintptr_t aphy32_to_virt(uintptr_t addr);

#endif

