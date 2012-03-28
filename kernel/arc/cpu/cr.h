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

#ifndef ARC_CPU_CR_H
#define ARC_CPU_CR_H

#include <stdint.h>

/* cr0 flags */
#define CR0_PE 0x00000001 /* protected mode */
#define CR0_MP 0x00000002 /* monitor co-processor */
#define CR0_EM 0x00000004 /* fpu emulation */
#define CR0_TS 0x00000008 /* task switched */
#define CR0_ET 0x00000010 /* co-processor extension type */
#define CR0_NE 0x00000020 /* fpu numeric error */
#define CR0_WP 0x00010000 /* write protect */
#define CR0_AM 0x00040000 /* alignment mask */
#define CR0_NW 0x20000000 /* no write-through */
#define CR0_CD 0x40000000 /* cache disable */
#define CR0_PG 0x80000000 /* paging */

/* cr3 flags */
#define CR3_PWT 0x00000008 /* page write through */
#define CR3_PCD 0x00000010 /* page cache disable */

/* cr4 flags */
#define CR4_VME        0x00000001 /* vm86 virtual interrupts */
#define CR4_PVI        0x00000002 /* pmode virtual interrupts */
#define CR4_TSD        0x00000004 /* time stamp disable */
#define CR4_DE         0x00000008 /* debugging extensions */
#define CR4_PSE        0x00000010 /* page size extension */
#define CR4_PAE        0x00000020 /* physical address extensions */
#define CR4_MCE        0x00000040 /* machine check enable */
#define CR4_PGE        0x00000080 /* enable global pages */
#define CR4_PCE        0x00000100 /* enable performance counters */
#define CR4_OSFXSR     0x00000200 /* fast fpu save */
#define CR4_OSXMMEXCPT 0x00000400 /* unmask SSE exceptions */
#define CR4_VMXE       0x00002000 /* enable VMX */
#define CR4_RDWRGSFS   0x00010000 /* enable RDWRGSFS */
#define CR4_OSXSAVE    0x00040000 /* enable xsave and xrestore */
#define CR4_SMEP       0x00100000 /* enable SMEP */

uint64_t cr0_read(void);
void cr0_write(uint64_t cr0);

uint64_t cr2_read(void);

uint64_t cr3_read(void);
void cr3_write(uint64_t cr3);

uint64_t cr4_read(void);
void cr4_write(uint64_t cr4);

#endif

