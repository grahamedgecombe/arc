/*
 * Copyright (c) 2011-2014 Graham Edgecombe <graham@grahamedgecombe.com>
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

#ifndef ARC_MM_HEAP_H
#define ARC_MM_HEAP_H

#include <arc/mm/common.h>
#include <stddef.h>

/*
 * Initializes the kernel heap by allocating an initial free block which covers
 * all of the free virtual address space from the end of the kernel image up to
 * the miscallenous reserved space at the end (which is used for physical
 * memory manager stacks, mapping the 4GB physical address space into virtual
 * memory and the recursive page directory trick.)
 */
void heap_init(void);

/*
 * Reserves 'size' bytes of memory, rounded up to the nearest page size, on the
 * kernel heap. The virtual memory is not automatically mapped to any physical
 * memory.
 */
void *heap_reserve(size_t size);

/*
 * Reserves 'size' bytes of memory on the kernel heap, like the function above,
 * and then allocates physical frames for this memory, and maps the physical
 * frames into the virtual memory.
 */
void *heap_alloc(size_t size, vm_acc_t flags);

/*
 * Frees some memory from the kernel heap. If the memory was allocated with
 * heap_alloc(), the physical frames will automatically be freed and unmapped
 * from virtual memory.
 */
void heap_free(void *ptr);

/* Prints out the kernel heap blocks for debugging purposes. */
void heap_trace(void);

#endif
