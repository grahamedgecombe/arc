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

#include <arc/intr/lapic.h>
#include <arc/cpu/msr.h>
#include <arc/mm/common.h>
#include <arc/mm/mmio.h>
#include <arc/intr/common.h>
#include <arc/tty.h>

#define LAPIC_ID        0x08
#define LAPIC_VER       0x0C
#define LAPIC_TPR       0x20
#define LAPIC_APR       0x24
#define LAPIC_PPR       0x28
#define LAPIC_EOI       0x2C
#define LAPIC_RRD       0x30
#define LAPIC_LDR       0x34
#define LAPIC_DFR       0x38
#define LAPIC_SVR       0x3C
#define LAPIC_ISR0      0x40
#define LAPIC_ISR1      0x44
#define LAPIC_ISR2      0x48
#define LAPIC_ISR3      0x4C
#define LAPIC_ISR4      0x50
#define LAPIC_ISR5      0x54
#define LAPIC_ISR6      0x58
#define LAPIC_ISR7      0x5C
#define LAPIC_TMR0      0x60
#define LAPIC_TMR1      0x64
#define LAPIC_TMR2      0x68
#define LAPIC_TMR3      0x6C
#define LAPIC_TMR4      0x70
#define LAPIC_TMR5      0x74
#define LAPIC_TMR6      0x78
#define LAPIC_TMR7      0x7C
#define LAPIC_IRR0      0x80
#define LAPIC_IRR1      0x84
#define LAPIC_IRR2      0x88
#define LAPIC_IRR3      0x8C
#define LAPIC_IRR4      0x90
#define LAPIC_IRR5      0x94
#define LAPIC_IRR6      0x98
#define LAPIC_IRR7      0x9C
#define LAPIC_ESR       0xA0
#define LAPIC_LVT_CMCI  0xBC
#define LAPIC_ICRL      0xC0
#define LAPIC_ICRH      0xC4
#define LAPIC_LVT_TIMER 0xC8
#define LAPIC_LVT_LINT0 0xD4
#define LAPIC_LVT_LINT1 0xD8
#define LAPIC_LVT_ERROR 0xDC
#define LAPIC_TIMER_ICR 0xE0
#define LAPIC_TIMER_CCR 0xE4
#define LAPIC_TIMER_DCR 0xF8

#define SVR_ENABLED 0x100

#define LVT_MASKED 0x00010000

static volatile uint32_t *lapic;
static uintptr_t lapic_phy_addr;

static uint32_t lapic_read(size_t reg)
{
  return lapic[reg];
}

static void lapic_write(size_t reg, uint32_t val)
{
  lapic[reg] = val;
}

void lapic_print_info(void)
{
  tty_printf(" => Using local APICs (at %0#18x)\n", lapic_phy_addr);
}

bool lapic_mmio_init(uintptr_t addr)
{
  lapic_phy_addr = addr;
  lapic = (volatile uint32_t *) mmio_map(addr, FRAME_SIZE, MMIO_R | MMIO_W);
  if (!lapic)
    return false;

  return true;
}

void lapic_init(void)
{
  uint64_t apic_base = (msr_read(MSR_APIC_BASE) & APIC_BASE_BSP) | lapic_phy_addr | APIC_BASE_ENABLED;
  msr_write(MSR_APIC_BASE, apic_base);

  /* enable the local APIC and set the spurious vector */
  lapic_write(LAPIC_SVR, SVR_ENABLED | SPURIOUS);

  /* reset the error status register */
  lapic_write(LAPIC_ESR, 0);
  lapic_write(LAPIC_ESR, 0);

  /* mask LVT entries */
  lapic_write(LAPIC_LVT_LINT0, LVT_MASKED);
  lapic_write(LAPIC_LVT_LINT1, LVT_MASKED);
  lapic_write(LAPIC_LVT_TIMER, LVT_MASKED);
  lapic_write(LAPIC_LVT_ERROR, LVT_MASKED);

  /* reset the priority so we accept all interrupts */
  lapic_write(LAPIC_TPR, 0);

  /* perform an ack just to make sure we can receive interrupts from now on */
  lapic_ack();
}

void lapic_ack(void)
{
  lapic_write(LAPIC_EOI, 0);
}

void lapic_ipi(uint8_t dest, uint16_t mode, uint8_t vector)
{
  /* format the ICR high and low dwords */
  uint32_t icrh = dest << 24;
  uint32_t icrl = vector | (mode << 8);

  /* write the high (must be first!) and low parts of the ICR */
  lapic_write(LAPIC_ICRH, icrh);
  lapic_write(LAPIC_ICRL, icrl);
}

