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

#include <arc/mm/seq.h>
#include <arc/trace.h>
#include <stdint.h>

#define SEQ_SIZE 8192

static uint8_t seq_pool[SEQ_SIZE];
static size_t seq_pos = 0;

void *seq_alloc(size_t len)
{
  if ((seq_pos + len) > SEQ_SIZE)
    return 0;

  void *ptr = &seq_pool[seq_pos];
  seq_pos += len;
  return ptr;
}

void seq_trace(void)
{
  trace_printf("Tracing sequential allocator... %d/%d bytes used.\n", seq_pos, SEQ_SIZE);
}
