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

#ifndef ARC_MM_COMMON_H
#define ARC_MM_COMMON_H

/*
 * virtual memory offset for transforming virtual addresses between
 * _start/_end to physical addresses
 */
#define VM_OFFSET 0xFFFF800000000000

/* page sizes */
#define FRAME_SIZE     4096
#define FRAME_SIZE_2M (512 * FRAME_SIZE)
#define FRAME_SIZE_1G (512 * FRAME_SIZE_2M)

/* page table flags */
#define PG_PRESENT   0x1
#define PG_WRITABLE  0x2
#define PG_USER      0x4
#define PG_BIG       0x80
#define PG_NO_EXEC   0x8000000000000000
#define PG_ADDR_MASK 0xFFFFFFFFFF000

#endif

