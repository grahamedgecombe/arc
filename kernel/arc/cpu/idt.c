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

#include <arc/cpu/idt.h>
#include <arc/intr/common.h>
#include <arc/intr/stub.h>
#include <arc/types.h>
#include <string.h>

#define GATE_PRESENT   0x80
#define GATE_USER      0x60
#define GATE_INTERRUPT 0x0E
#define GATE_TRAP      0x0F

typedef PACK(struct
{
  uint16_t addr_low;
  uint16_t cs_sel;
  uint8_t  ist;
  uint8_t  flags;
  uint16_t addr_mid;
  uint32_t addr_high;
  uint32_t reserved2;
}) idt_gate_t;

static idt_gate_t idt_gates[INTERRUPTS];
static idtr_t idtr;

static void idt_encode_gate(intr_t id, void (*handler)(void), uint8_t flags)
{
  idt_gate_t *gate = &idt_gates[id];

  gate->cs_sel = 0x08;

  uintptr_t handler_addr = (uintptr_t) handler;
  gate->addr_low = handler_addr & 0xFFFF;
  gate->addr_mid = (handler_addr >> 16) & 0xFFFF;
  gate->addr_high = (handler_addr >> 32) & 0xFFFFFFFF;

  gate->flags = flags;
}

void idt_bsp_init(void)
{
  memset(idt_gates, 0, sizeof(idt_gates));

  idt_encode_gate(FAULT0,    &fault0,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT1,    &fault1,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT2,    &fault2,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT3,    &fault3,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT4,    &fault4,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT5,    &fault5,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT6,    &fault6,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT7,    &fault7,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT8,    &fault8,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT9,    &fault9,    GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT10,   &fault10,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT11,   &fault11,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT12,   &fault12,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT13,   &fault13,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT14,   &fault14,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT15,   &fault15,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT16,   &fault16,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT17,   &fault17,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT18,   &fault18,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT19,   &fault19,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT20,   &fault20,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT21,   &fault21,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT22,   &fault22,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT23,   &fault23,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT24,   &fault24,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT25,   &fault25,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT26,   &fault26,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT27,   &fault27,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT28,   &fault28,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT29,   &fault29,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT30,   &fault30,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(FAULT31,   &fault31,   GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ0,      &irq0,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ1,      &irq1,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ2,      &irq2,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ3,      &irq3,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ4,      &irq4,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ5,      &irq5,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ6,      &irq6,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ7,      &irq7,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ8,      &irq8,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ9,      &irq9,      GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ10,     &irq10,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ11,     &irq11,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ12,     &irq12,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ13,     &irq13,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ14,     &irq14,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ15,     &irq15,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ16,     &irq16,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ17,     &irq17,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ18,     &irq18,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ19,     &irq19,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ20,     &irq20,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ21,     &irq21,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ22,     &irq22,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(IRQ23,     &irq23,     GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(LVT_TIMER, &lvt_timer, GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(LVT_ERROR, &lvt_error, GATE_PRESENT | GATE_INTERRUPT);
  idt_encode_gate(SPURIOUS,  &spurious,  GATE_PRESENT | GATE_INTERRUPT);

  idtr.addr = (uint64_t) idt_gates;
  idtr.len = sizeof(idt_gates) - 1;
  idtr_install(&idtr);
}

void idt_ap_init(void)
{
  idtr_install(&idtr);
}

