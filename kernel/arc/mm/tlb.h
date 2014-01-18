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

#ifndef ARC_MM_TLB_H
#define ARC_MM_TLB_H

#include <stdint.h>

#define TLB_OP_INVLPG 0x0
#define TLB_OP_FLUSH  0x1

typedef struct
{
  int type;
  uintptr_t addr;
} tlb_op_t;

void tlb_init(void);

void tlb_transaction_init(void);

void tlb_transaction_queue_invlpg(uintptr_t addr);
void tlb_transaction_queue_flush(void);

void tlb_transaction_rollback(void);
void tlb_transaction_commit(void);

#endif
