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

#include <arc/intr/lx2apic.h>
#include <arc/cpu/msr.h>
#include <arc/intr/common.h>

#define LX2APIC_ID        0x802
#define LX2APIC_VER       0x803
#define LX2APIC_TPR       0x808
#define LX2APIC_PPR       0x80A
#define LX2APIC_EOI       0x80B
#define LX2APIC_SVR       0x80F
#define LX2APIC_LDR       0x80D
#define LX2APIC_ISR0      0x810
#define LX2APIC_ISR1      0x811
#define LX2APIC_ISR2      0x812
#define LX2APIC_ISR3      0x813
#define LX2APIC_ISR4      0x814
#define LX2APIC_ISR5      0x815
#define LX2APIC_ISR6      0x816
#define LX2APIC_ISR7      0x817
#define LX2APIC_TMR0      0x818
#define LX2APIC_TMR1      0x819
#define LX2APIC_TMR2      0x81A
#define LX2APIC_TMR3      0x81B
#define LX2APIC_TMR4      0x81C
#define LX2APIC_TMR5      0x81D
#define LX2APIC_TMR6      0x81E
#define LX2APIC_TMR7      0x81F
#define LX2APIC_IRR0      0x820
#define LX2APIC_IRR1      0x821
#define LX2APIC_IRR2      0x822
#define LX2APIC_IRR3      0x823
#define LX2APIC_IRR4      0x824
#define LX2APIC_IRR5      0x825
#define LX2APIC_IRR6      0x826
#define LX2APIC_IRR7      0x827
#define LX2APIC_ESR       0x828
#define LX2APIC_LVT_CMCI  0x82F
#define LX2APIC_ICR       0x830
#define LX2APIC_LVT_TIMER 0x832
#define LX2APIC_LVT_LINT0 0x835
#define LX2APIC_LVT_LINT1 0x836
#define LX2APIC_LVT_ERROR 0x837
#define LX2APIC_TIMER_ICR 0x838
#define LX2APIC_TIMER_CCR 0x839
#define LX2APIC_TIMER_DCR 0x83E
#define LX2APIC_SELF_IPI  0x83F

#define SVR_ENABLED 0x100

static uint64_t lx2apic_read(uint32_t reg)
{
  return msr_read(reg);
}

static void lx2apic_write(uint32_t reg, uint64_t val)
{
  msr_write(reg, val);
}

void lx2apic_init(void)
{
  uint64_t apic_base = (msr_read(MSR_APIC_BASE) & APIC_BASE_BSP) | APIC_BASE_ENABLED | APIC_BASE_X2_MODE;
  msr_write(MSR_APIC_BASE, apic_base);

  /* enable the local APIC and set the spurious vector */
  lx2apic_write(LX2APIC_SVR, SVR_ENABLED | SPURIOUS);

  /* reset the priority so we accept all interrupts */
  lx2apic_write(LX2APIC_TPR, 0);

  /* perform an ack just to make sure we can receive interrupts from now on */
  lx2apic_ack();
}

void lx2apic_ack(void)
{
  lx2apic_write(LX2APIC_EOI, 0);
}

void lx2apic_ipi(uint32_t dest, uint16_t mode, uint8_t vector)
{
  /* format the ICR high and low dwords */
  uint64_t icrh = dest;
  uint64_t icrl = vector | (mode << 8);

  /* write the ICR */
  lx2apic_write(LX2APIC_ICR, (icrh << 32) | icrl);
}

