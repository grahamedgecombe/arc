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

#include <arc/cpu/idt.h>
#include <arc/intr/common.h>
#include <arc/intr/stub.h>
#include <arc/types.h>
#include <string.h>

#define IDT_PRESENT   0x80
#define IDT_USER      0x60
#define IDT_INTERRUPT 0x0E
#define IDT_TRAP      0x0F

typedef struct
{
  uint16_t addr_low;
  uint16_t cs_sel;
  uint8_t  ist;
  uint8_t  flags;
  uint16_t addr_mid;
  uint32_t addr_high;
  uint32_t reserved2;
} __attribute__((__packed__)) idt_descriptor_t;

static idt_descriptor_t idt_descriptors[INTERRUPTS];
static idtr_t idtr;

static void idt_encode_descriptor(intr_t id, void (*handler)(void), uint8_t flags)
{
  idt_descriptor_t *descriptor = &idt_descriptors[id];

  descriptor->cs_sel = 0x08;

  uintptr_t handler_addr = (uintptr_t) handler;
  descriptor->addr_low = handler_addr & 0xFFFF;
  descriptor->addr_mid = (handler_addr >> 16) & 0xFFFF;
  descriptor->addr_high = (handler_addr >> 32) & 0xFFFFFFFF;

  descriptor->flags = flags;
}

void idt_bsp_init(void)
{
  memset(idt_descriptors, 0, sizeof(idt_descriptors));

  idt_encode_descriptor(FAULT0,    &fault0,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT1,    &fault1,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT2,    &fault2,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT3,    &fault3,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT4,    &fault4,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT5,    &fault5,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT6,    &fault6,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT7,    &fault7,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT8,    &fault8,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT9,    &fault9,    IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT10,   &fault10,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT11,   &fault11,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT12,   &fault12,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT13,   &fault13,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT14,   &fault14,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT15,   &fault15,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT16,   &fault16,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT17,   &fault17,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT18,   &fault18,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT19,   &fault19,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT20,   &fault20,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT21,   &fault21,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT22,   &fault22,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT23,   &fault23,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT24,   &fault24,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT25,   &fault25,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT26,   &fault26,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT27,   &fault27,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT28,   &fault28,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT29,   &fault29,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT30,   &fault30,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(FAULT31,   &fault31,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ0,      &irq0,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ1,      &irq1,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ2,      &irq2,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ3,      &irq3,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ4,      &irq4,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ5,      &irq5,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ6,      &irq6,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ7,      &irq7,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ8,      &irq8,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ9,      &irq9,      IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ10,     &irq10,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ11,     &irq11,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ12,     &irq12,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ13,     &irq13,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ14,     &irq14,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ15,     &irq15,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ16,     &irq16,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ17,     &irq17,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ18,     &irq18,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ19,     &irq19,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ20,     &irq20,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ21,     &irq21,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ22,     &irq22,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IRQ23,     &irq23,     IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IPI_PANIC, &ipi_panic, IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(IPI_TLB,   &ipi_tlb,   IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(LVT_TIMER, &lvt_timer, IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(LVT_ERROR, &lvt_error, IDT_PRESENT | IDT_INTERRUPT);
  idt_encode_descriptor(SPURIOUS,  &spurious,  IDT_PRESENT | IDT_INTERRUPT);

  idtr.addr = (uint64_t) idt_descriptors;
  idtr.len = sizeof(idt_descriptors) - 1;
  idtr_install(&idtr);
}

void idt_ap_init(void)
{
  idtr_install(&idtr);
}
