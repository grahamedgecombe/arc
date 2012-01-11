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

#ifndef ARC_MM_MALLOC_H
#define ARC_MM_MALLOC_H

#include <arc/panic.h>
#include <arc/mm/common.h>
#include <arc/mm/heap.h>
#include <stddef.h>

/* make it so when dlmalloc aborts a kernel panic is triggered */
#define ABORT boot_panic("dlmalloc abort()")

/* these headers do not exist (or are not complete) in the kernel */
#define LACKS_ERRNO_H
#define LACKS_STDLIB_H
#define LACKS_TIME_H
#define LACKS_STRINGS_H
#define LACKS_SYS_TYPES_H
#define LACKS_SYS_MMAN_H
#define LACKS_SYS_PARAM_H
#define LACKS_FCNTL_H
#define LACKS_UNISTD_H
#define LACKS_SCHED_H

/* stats require stdio.h which we don't support */
#define NO_MALLOC_STATS 1

/* use spinlocks */
#define USE_LOCKS           1
#define USE_SPIN_LOCKS      1

/* prefix function names with dl */
#define USE_DL_PREFIX 1

/* we don't support errno so just nop instead */
#define MALLOC_FAILURE_ACTION

/* we only support mmap-like calls */
#define HAVE_MORECORE 0
#define HAVE_MMAP     1
#define HAVE_MREMAP   0
#define MMAP_CLEARS   0

/* hard-coded page size of 4k */
#define malloc_getpagesize FRAME_SIZE

/* some error codes */
#define ENOMEM 1
#define EINVAL 2

/* minimal definitions for translating mmap() to heap_*() */
#define O_RDWR 1
#define MAP_ANONYMOUS 1
#define MAP_PRIVATE 2
#define PROT_READ 1
#define PROT_WRITE 2

typedef size_t off_t;

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);

/* include the actual dlmalloc header file */
#include <dlmalloc.h>

#endif
