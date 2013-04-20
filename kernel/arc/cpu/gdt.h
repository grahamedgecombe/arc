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

#ifndef ARC_CPU_GDT_H
#define ARC_CPU_GDT_H

#include <stdint.h>

#define GDT_DESCRIPTORS 7

#define SLTR_NULL        0x0000
#define SLTR_KERNEL_CODE 0x0008
#define SLTR_KERNEL_DATA 0x0010
#define SLTR_USER_DATA   0x0018
#define SLTR_USER_CODE   0x0020
#define SLTR_TSS         0x0028 /* occupies two GDT descriptors */

#define RPL0 0x0
#define RPL1 0x1
#define RPL2 0x2
#define RPL3 0x3

typedef struct
{
  uint16_t len;
  uint64_t addr;
} __attribute__((__packed__)) gdtr_t;

typedef struct
{
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t  base_mid;
  uint8_t  flags;
  uint8_t  granularity; /* and high limit */
  uint8_t  base_high;
} __attribute__((__packed__)) gdt_descriptor_t;

typedef struct
{
  gdt_descriptor_t low;
  struct
  {
    uint32_t base_xhigh;
    uint32_t reserved;
  } high;
} __attribute__((__packed__)) gdt_xdescriptor_t;

void gdt_init(void);
void gdtr_install(gdtr_t *gdtr, uint16_t cs, uint16_t ds);

#endif
