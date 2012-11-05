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

#ifndef ARC_MM_MMIO_H
#define ARC_MM_MMIO_H

#include <arc/mm/common.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Maps an arbitrary physical address into the kernel's virtual address space.
 * This address and length do not need to be page aligned, this is dealt with
 * by the function itself. However, aligned memory is preferable to avoid
 * wasted virtual address space.
 *
 * The main purpose of this function is for mapping memory-mapped I/O devices
 * into the kernel's virtual memory, however, it is also used in a few other
 * miscellaneous ways e.g. for mapping another task's PML4 table into virtual
 * memory temporarily to copy from it.
 */
void *mmio_map(uintptr_t phy, size_t len, vm_acc_t flags);

/* Unmaps a memory-mapped I/O area. */
void mmio_unmap(void *virt, size_t len);

#endif
