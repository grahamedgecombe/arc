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

#ifndef ARC_MM_VALIDATE_H
#define ARC_MM_VALIDATE_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Check if a buffer is wholly contained within user-space. All pointers passed
 * to a system call must be checked with this function, to avoid security
 * issues, whereby a rogue user program could manipulate the kernel into
 * reading or corrupting kernel memory for it.
 */
bool valid_buffer(const void *ptr, size_t len);

/*
 * Checks if a null-terminated string is wholly contained within user-space,
 * all strings passed to a system call must be likewise checked with this
 * function.
 */
bool valid_string(const char *str);

#endif

