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

#include <arc/intr/apic.h>
#include <arc/cpu/msr.h>
#include <arc/intr/common.h>
#include <arc/mm/common.h>
#include <arc/mm/mmio.h>

#define MSR_X2APIC_MMIO 0x800

/* common registers */
#define APIC_TPR        0x08
#define APIC_EOI        0x0B
#define APIC_SVR        0x0F
#define APIC_ESR        0x28
#define APIC_LVT_TIMER  0x32
#define APIC_LVT_LINT0  0x35
#define APIC_LVT_LINT1  0x36
#define APIC_LVT_ERROR  0x37
#define APIC_TIMER_ICR  0x38
#define APIC_TIMER_CCR  0x39
#define APIC_TIMER_DCR  0x3E

/* xAPIC only registers */
#define XAPIC_ICRL      0x30
#define XAPIC_ICRH      0x31

/* x2APIC only registers */
#define X2APIC_ICR      0x30
#define X2APIC_SELF_IPI 0x3F

/* SVR flags */
#define SVR_ENABLED 0x100

/* LVT flags */
#define LVT_MASKED         0x00010000
#define LVT_TYPE_FIXED     0x00000000
#define LVT_TYPE_SMI       0x00000200
#define LVT_TYPE_NMI       0x00000400
#define LVT_TYPE_EXTINT    0x00000700
#define LVT_DELIVS         0x00001000
#define LVT_REMOTE_IRR     0x00004000
#define LVT_TRIGGER_LEVEL  0x00008000
#define LVT_TRIGGER_EDGE   0x00000000
#define LVT_TIMER_PERIODIC 0x00020000
#define LVT_TIMER_ONE_SHOT 0x00000000

typedef enum
{
  MODE_XAPIC,
  MODE_X2APIC
} apic_mode_t;

static apic_mode_t apic_mode;
static uintptr_t apic_phy_addr;
static volatile uint32_t *apic_mmio;

static uint64_t apic_read(size_t reg)
{
  if (apic_mode == MODE_X2APIC)
    return msr_read(MSR_X2APIC_MMIO + reg);
  else
    return apic_mmio[reg * 4];
}

static void apic_write(size_t reg, uint64_t val)
{
  if (apic_mode == MODE_X2APIC)
    msr_write(MSR_X2APIC_MMIO + reg, val);
  else
    apic_mmio[reg * 4] = val;
}

bool xapic_init(uintptr_t addr)
{
  apic_phy_addr = addr;
  apic_mmio = (volatile uint32_t *) mmio_map(addr, FRAME_SIZE, MMIO_R | MMIO_W);
  if (!apic_mmio)
    return false;

  apic_mode = MODE_XAPIC;
  return true;
}

void x2apic_init(void)
{
  apic_mode = MODE_X2APIC;
}

void apic_init(void)
{
  /* program the APIC base register on this CPU */
  if (apic_mode == MODE_XAPIC)
  {
    uint64_t apic_base = (msr_read(MSR_APIC_BASE) & APIC_BASE_BSP) | apic_phy_addr | APIC_BASE_ENABLED;
    msr_write(MSR_APIC_BASE, apic_base);
  }
  else
  {
    uint64_t apic_base = (msr_read(MSR_APIC_BASE) & APIC_BASE_BSP) | APIC_BASE_ENABLED | APIC_BASE_X2_MODE;
    msr_write(MSR_APIC_BASE, apic_base);
  }

  /* set the spurious interrupt vector */
  apic_write(APIC_SVR, SVR_ENABLED | SPURIOUS);

  /* reset the error status */
  apic_write(APIC_ESR, 0);
  apic_write(APIC_ESR, 0);

  /* program LVT entries */
  apic_write(APIC_LVT_LINT0, LVT_MASKED);
  apic_write(APIC_LVT_LINT1, LVT_MASKED);
  apic_write(APIC_LVT_TIMER, LVT_MASKED);
  apic_write(APIC_LVT_ERROR, LVT_TYPE_FIXED | LVT_ERROR);

  /* reset the priority so we accept all interrupts */
  apic_write(APIC_TPR, 0);

  /* ack any outstounding interrupts */
  apic_ack();
}

void apic_ack(void)
{
  apic_write(APIC_EOI, 0);
}

