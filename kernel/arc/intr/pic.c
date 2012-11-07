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

#include <arc/intr/pic.h>
#include <arc/cpu/port.h>

/* the I/O ports for the master and slave PICs */
#define PIC1_CMD  0x0020
#define PIC1_DATA 0x0021
#define PIC2_CMD  0x00A0
#define PIC2_DATA 0x00A1

#define ICW1_IC4  0x1
#define ICW1_SNGL 0x2
#define ICW1_ADI  0x4
#define ICW1_LTIM 0x8
#define ICW1_MB1  0x10 /* must be set to 1 */

#define ICW4_8086 0x1
#define ICW4_AEOI 0x2
#define ICW4_M    0x4  /* don't care if BUF not set */
#define ICW4_BUF  0x8
#define ICW4_SFNM 0x10

#define OCW2_EOI 0x20
#define OCW2_SL  0x40
#define OCW2_R   0x80

#define OCW3_RIS  0x1
#define OCW3_RR   0x2
#define OCW3_P    0x4
#define OCW3_MB1  0x8  /* must be set to 1 */
#define OCW3_SMM  0x20
#define OCW3_ESMM 0x40

/* remaps the PICs so IRQ0 -> IRQ15 do not clash with CPU exceptions */
void pic_init(void)
{
  /* start initialising the PICs */
  outb_p(PIC1_CMD, ICW1_MB1 | ICW1_IC4);
  outb_p(PIC2_CMD, ICW1_MB1 | ICW1_IC4);

  /* set the offsets */
  outb_p(PIC1_DATA, IRQ0);
  outb_p(PIC2_DATA, IRQ8);

  /* tell the PICs how they are wired to each other */
  outb_p(PIC1_DATA, 0x04); /* IRQ2 has slave */
  outb_p(PIC2_DATA, 0x02); /* connected to IRQ2 of master */

  /* change the PICs to use 8086 mode */
  outb_p(PIC1_DATA, ICW4_8086 | ICW4_M);
  outb_p(PIC2_DATA, ICW4_8086);

  /* mask every IRQ by default except IRQ2 which is used for cascading */
  outb_p(PIC1_DATA, 0xFB);
  outb_p(PIC2_DATA, 0xFF);
}

/* checks if an IRQ is spurious, and deals with acknowledging the master PIC
 * if there is a spurious IRQ15 */
bool pic_spurious(irq_t irq)
{
  if (irq == 7)
  {
    uint8_t isr = inb_p(PIC1_CMD);
    if (~isr & 0x80)
      return true;
  }
  else if (irq == 15)
  {
    uint8_t isr = inb_p(PIC2_CMD);
    if (~isr & 0x80)
    {
      outb_p(PIC1_CMD, OCW2_EOI);
      return true;
    }
  }

  return false;
}

/* masks an IRQ */
void pic_mask(irq_t irq)
{
  uint16_t port;
  if (irq < 8)
  {
    port = PIC1_DATA;
  }
  else
  {
    port = PIC2_DATA;
    irq -= 8;
  }

  uint8_t mask = inb_p(port) | (1 << irq);
  outb_p(port, mask);
}

/* unmasks an IRQ */
void pic_unmask(irq_t irq)
{
  uint16_t port;
  if (irq < 8)
  {
    port = PIC1_DATA;
  }
  else
  {
    port = PIC2_DATA;
    irq -= 8;
  }

  uint8_t mask = inb_p(port) & ~(1 << irq);
  outb_p(port, mask);
}

/* acknowledges the handling of an interrupt */
void pic_ack(irq_t irq)
{
  if (irq >= 8)
    outb_p(PIC2_CMD, OCW2_EOI);
  outb_p(PIC1_CMD, OCW2_EOI);
}
