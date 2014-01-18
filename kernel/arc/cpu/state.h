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

#ifndef ARC_CPU_STATE_H
#define ARC_CPU_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdnoreturn.h>

/* CPU state passed to intr_dispatch() (and various other places) */
typedef struct
{
  /* the register file */
  uint64_t regs[15];

  /* the error code and interrupt id */
  uint64_t id;
  uint64_t error;

  /* these are pushed automatically by the CPU */
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((__packed__)) cpu_state_t;

#define RAX 0
#define RBX 1
#define RCX 2
#define RDX 3
#define RSI 4
#define RDI 5
#define RBP 6
/* RSP is stored in a separate field */
#define R8  7
#define R9  8
#define R10 9
#define R11 10
#define R12 11
#define R13 12
#define R14 13
#define R15 14

#endif
