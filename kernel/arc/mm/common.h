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

#ifndef ARC_MM_COMMON_H
#define ARC_MM_COMMON_H

/* user-space virtual memory end address (inclusive) */
#define VM_USER_END 0x00007FFFFFFFFFFF

/* virtual memory offset of the higher half */
#define VM_HIGHER_HALF 0xFFFF800000000000

/* virtual memory offset of the kernel image */
#define VM_KERNEL_IMAGE 0xFFFFFFFF80000000

/* the number of entries in a page table */
#define TABLE_SIZE 512

/* actual page sizes */
#define FRAME_SIZE     4096
#define FRAME_SIZE_2M (512 * FRAME_SIZE)
#define FRAME_SIZE_1G (512 * FRAME_SIZE_2M)
/* 512G pages don't exist (yet) but this is useful for the vmm anyway */
#define FRAME_SIZE_512G (512L * FRAME_SIZE_1G)

/* page size types */
#define SIZE_4K    0
#define SIZE_2M    1
#define SIZE_1G    2
#define SIZE_COUNT 3

/* memory access flags */
typedef enum
{
  VM_R = 0x1, /* readable (on x86 it is not possible to deny read access) */
  VM_W = 0x2, /* writable */
  VM_X = 0x4  /* executable (on x86 lack of this flag sets the NX bit) */
} vm_acc_t;

/* page table flags */
#define PG_PRESENT   0x1
#define PG_WRITABLE  0x2
#define PG_USER      0x4
#define PG_BIG       0x80
#define PG_NO_EXEC   0x8000000000000000
#define PG_ADDR_MASK 0xFFFFFFFFFF000

#endif
