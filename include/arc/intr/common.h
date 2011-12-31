/*
 * Copyright (c) 2011 Graham Edgecombe <graham@grahamedgecombe.com>
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

#ifndef ARC_INTR_COMMON_H
#define ARC_INTR_COMMON_H

#include <arc/pack.h>
#include <stdint.h>

/* the max number of interrupts */
#define INTERRUPTS 256

/* CPU exceptions */
#define FAULT0  0x00
#define FAULT1  0x01
#define FAULT2  0x02
#define FAULT3  0x03
#define FAULT4  0x04
#define FAULT5  0x05
#define FAULT6  0x06
#define FAULT7  0x07
#define FAULT8  0x08
#define FAULT9  0x09
#define FAULT10 0x0A
#define FAULT11 0x0B
#define FAULT12 0x0C
#define FAULT13 0x0D
#define FAULT14 0x0E
#define FAULT15 0x0F
#define FAULT16 0x20
#define FAULT17 0x21
#define FAULT18 0x22
#define FAULT19 0x23
#define FAULT20 0x24
#define FAULT21 0x25
#define FAULT22 0x26
#define FAULT23 0x27
#define FAULT24 0x28
#define FAULT25 0x29
#define FAULT26 0x2A
#define FAULT27 0x2B
#define FAULT28 0x2C
#define FAULT29 0x2D
#define FAULT30 0x2E
#define FAULT31 0x2F

/* master PIC IRQs */
#define IRQ0  0x20
#define IRQ1  0x21
#define IRQ2  0x22
#define IRQ3  0x23
#define IRQ4  0x24
#define IRQ5  0x25
#define IRQ6  0x26
#define IRQ7  0x27

/* slave PIC IRQs */
#define IRQ8  0x28
#define IRQ9  0x29
#define IRQ10 0x2A
#define IRQ11 0x2B
#define IRQ12 0x2C
#define IRQ13 0x2D
#define IRQ14 0x2E
#define IRQ15 0x2F

/* APIC IRQs */
#define IRQ16 0x30
#define IRQ17 0x31
#define IRQ18 0x32
#define IRQ19 0x33
#define IRQ20 0x34
#define IRQ21 0x35
#define IRQ22 0x36
#define IRQ23 0x37

/* system call */
#define SYSCALL 0xFF

/* a type suitable for storing an interrupt id */
typedef uint8_t intr_id_t;

/* interrupt state passed to intr_dispatch() */
typedef PACK(struct
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
}) intr_state_t;

#endif

