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

/* remaps the PICs so IRQ0 -> IRQ15 do not clash with CPU exceptions */
void pic_init(void)
{
  /* initialise the slave and master */
  outb_p(PIC1_CMD, 0x11);
  outb_p(PIC2_CMD, 0x11);

  /* set the offsets */
  outb_p(PIC1_DATA, 0x20);
  outb_p(PIC2_DATA, 0x28);

  /* tell the PICs how they are wired to each other */
  outb_p(PIC1_DATA, 0x04);
  outb_p(PIC2_DATA, 0x02);

  /* change the PICs to use 8086 mode */
  outb_p(PIC1_DATA, 0x01);
  outb_p(PIC2_DATA, 0x01);

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
      outb_p(PIC1_CMD, 0x20);
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
    outb_p(PIC2_CMD, 0x20);
  outb_p(PIC1_CMD, 0x20);
}
