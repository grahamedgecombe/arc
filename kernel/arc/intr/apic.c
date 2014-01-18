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

#include <arc/intr/apic.h>
#include <arc/time/apic.h>
#include <arc/time/pit.h>
#include <arc/cpu/msr.h>
#include <arc/cpu/state.h>
#include <arc/intr/common.h>
#include <arc/lock/spinlock.h>
#include <arc/lock/intr.h>
#include <arc/mm/common.h>
#include <arc/mm/mmio.h>
#include <arc/smp/cpu.h>

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

/* SVR flags */
#define SVR_ENABLED 0x100

/* ICR flags */
#define ICR_TYPE_FIXED        0x00000000
#define ICR_TYPE_SMI          0x00000200
#define ICR_TYPE_NMI          0x00000400
#define ICR_TYPE_INIT         0x00000500
#define ICR_TYPE_STARTUP      0x00000600
#define ICR_PHYSICAL          0x00000000
#define ICR_LOGICAL           0x00000800
#define ICR_DELIVS            0x00001000
#define ICR_DEASSERT          0x00000000
#define ICR_ASSERT            0x00004000
#define ICR_TRIGGER_EDGE      0x00000000
#define ICR_TRIGGER_LEVEL     0x00008000
#define ICR_DEST_SELF         0x00040000
#define ICR_DEST_ALL          0x00080000
#define ICR_DEST_ALL_EXC_SELF 0x000C0000

/* DCR values */
#define DCR_1   0xB
#define DCR_2   0x0
#define DCR_4   0x1
#define DCR_8   0x2
#define DCR_16  0x3
#define DCR_32  0x8
#define DCR_64  0x9
#define DCR_128 0xA

typedef enum
{
  MODE_XAPIC,
  MODE_X2APIC
} apic_mode_t;

static apic_mode_t apic_mode;
static uintptr_t apic_phy_addr;
static volatile uint32_t *apic_mmio;

uint64_t apic_read(size_t reg)
{
  if (apic_mode == MODE_X2APIC)
    return msr_read(MSR_X2APIC_MMIO + reg);
  else
    return apic_mmio[reg * 4];
}

void apic_write(size_t reg, uint64_t val)
{
  if (apic_mode == MODE_X2APIC)
    msr_write(MSR_X2APIC_MMIO + reg, val);
  else
    apic_mmio[reg * 4] = val;
}

static void apic_timer_calibrate(void)
{
  cpu_t *cpu = cpu_get();

  static spinlock_t apic_calibrate_lock = SPIN_UNLOCKED;
  spin_lock(&apic_calibrate_lock);

  apic_write(APIC_LVT_TIMER, LVT_MASKED);
  apic_write(APIC_TIMER_ICR, 0xFFFFFFFF);
  apic_write(APIC_TIMER_DCR, DCR_16);
  pit_mdelay(10);
  uint32_t ticks = 0xFFFFFFFF - apic_read(APIC_TIMER_CCR);

  cpu->apic_ticks_per_ms = ticks * 16 / 10;

  spin_unlock(&apic_calibrate_lock);
}

void apic_monotonic(int ms, intr_handler_t handler)
{
  cpu_t *cpu = cpu_get();

  intr_route_intr(LVT_TIMER, handler);

  apic_write(APIC_LVT_TIMER, LVT_TIMER_PERIODIC | LVT_TYPE_FIXED | LVT_TIMER);
  apic_write(APIC_TIMER_ICR, cpu->apic_ticks_per_ms * ms / 16);
  apic_write(APIC_TIMER_DCR, DCR_16);
}

bool xapic_init(uintptr_t addr)
{
  apic_phy_addr = addr;
  apic_mmio = (volatile uint32_t *) mmio_map(addr, FRAME_SIZE, VM_R | VM_W);
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
  cpu_t *cpu = cpu_get();

  /* program the APIC base register on this CPU */
  if (apic_mode == MODE_X2APIC)
  {
    uint64_t apic_base = (msr_read(MSR_APIC_BASE) & APIC_BASE_BSP) | APIC_BASE_ENABLED | APIC_BASE_X2_MODE;
    msr_write(MSR_APIC_BASE, apic_base);
  }
  else
  {
    uint64_t apic_base = (msr_read(MSR_APIC_BASE) & APIC_BASE_BSP) | apic_phy_addr | APIC_BASE_ENABLED;
    msr_write(MSR_APIC_BASE, apic_base);
  }

  /* set the spurious interrupt vector */
  apic_write(APIC_SVR, SVR_ENABLED | SPURIOUS);

  /* reset the error status */
  apic_write(APIC_ESR, 0);
  apic_write(APIC_ESR, 0);

  /* program LVT entries */
  apic_write(APIC_LVT_LINT0, cpu->apic_lint_nmi[0] ? LVT_TYPE_NMI : LVT_MASKED);
  apic_write(APIC_LVT_LINT1, cpu->apic_lint_nmi[1] ? LVT_TYPE_NMI : LVT_MASKED);
  apic_write(APIC_LVT_TIMER, LVT_MASKED);
  apic_write(APIC_LVT_ERROR, LVT_TYPE_FIXED | LVT_ERROR);

  /* reset the priority so we accept all interrupts */
  apic_write(APIC_TPR, 0);

  /* ack any outstounding interrupts */
  apic_ack();

  /* calibrate this APIC's timer */
  apic_timer_calibrate();
}

void apic_ack(void)
{
  apic_write(APIC_EOI, 0);
}

static void apic_ipi(uint64_t icr)
{
  if (apic_mode == MODE_X2APIC)
  {
    /* in x2APIC mode the ICRL and ICRH are combined */
    apic_write(X2APIC_ICR, icr);
  }
  else
  {
    /* write the high (must be first!) and low parts of the ICR */
    apic_write(XAPIC_ICRH, (icr >> 32) & 0xFFFFFFFF);
    apic_write(XAPIC_ICRL, icr & 0xFFFFFFFF);
  }
}

void apic_ipi_init(cpu_lapic_id_t id)
{
  uint64_t icr = ICR_TYPE_INIT;

  if (apic_mode == MODE_X2APIC)
    icr |= ((uint64_t) id) << 32;
  else
    icr |= ((uint64_t) id) << 56;

  apic_ipi(icr);
}

void apic_ipi_startup(cpu_lapic_id_t id, uint8_t trampoline_addr)
{
  uint64_t icr = ICR_TYPE_STARTUP | trampoline_addr;

  if (apic_mode == MODE_X2APIC)
    icr |= ((uint64_t) id) << 32;
  else
    icr |= ((uint64_t) id) << 56;

  apic_ipi(icr);
}

void apic_ipi_fixed(cpu_lapic_id_t id, intr_t intr)
{
  uint64_t icr = ICR_TYPE_FIXED | intr;

  if (apic_mode == MODE_X2APIC)
    icr |= ((uint64_t) id) << 32;
  else
    icr |= ((uint64_t) id) << 56;

  apic_ipi(icr);
}

void apic_ipi_all(intr_t intr)
{
  uint64_t icr = ICR_TYPE_FIXED | ICR_DEST_ALL | intr;
  apic_ipi(icr);
}

void apic_ipi_all_exc_self(intr_t intr)
{
  uint64_t icr = ICR_TYPE_FIXED | ICR_DEST_ALL_EXC_SELF | intr;
  apic_ipi(icr);
}

void apic_ipi_self(intr_t intr)
{
  if (apic_mode == MODE_X2APIC)
  {
    apic_write(X2APIC_SELF_IPI, intr);
  }
  else
  {
    uint64_t icr = ICR_TYPE_FIXED | ICR_DEST_SELF | intr;
    apic_ipi(icr);
  }
}
