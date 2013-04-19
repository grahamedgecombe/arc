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

#include <arc/trace/uart.h>
#include <arc/cpu/port.h>
#include <arc/bda.h>
#include <arc/panic.h>
#include <stdint.h>

/* the frequency of the UART's clock in Hz */
#define UART_FREQ 115200

/* the desired baud rate */
#define UART_BAUD 9600

/* register port offsets */
#define UART_RXTX 0x0 /* transmit/receive register (also low divisor byte) */
#define UART_IER  0x1 /* interrupt enable register (also high divisor byte) */
#define UART_IIR  0x2 /* interrupt identification register (also fifo control register) */
#define UART_LCR  0x3 /* line control register */
#define UART_MCR  0x4 /* modem control register */
#define UART_LSR  0x5 /* line status register */
#define UART_MSR  0x6 /* modem status register */

/* line control register bits */
#define LCR_8BIT 0x03
#define LCR_DLAB 0x80 /* divisor latch access bit */

/* line status register bits */
#define LSR_TX_READY 0x20

static uint16_t uart_port_base = 0x3F8;

void uart_init(void)
{
  /* find the first serial port */
  uart_port_base = bda_reads(BDA_COM1_PORT);
  if (!uart_port_base)
    panic("no COM1 port present");

  /* disable interrupts */
  outb_p(uart_port_base + UART_IER, 0);

  /* set the baud rate */
  uint16_t divisor = (uint16_t) (UART_FREQ / UART_BAUD);
  outb_p(uart_port_base + UART_LCR, LCR_DLAB);
  outb_p(uart_port_base + UART_RXTX, divisor);
  outb_p(uart_port_base + UART_IER,  divisor >> 8);

  /* set frame format (8 data bits, no parity, 1 stop bit) */
  outb_p(uart_port_base + UART_LCR, LCR_8BIT);
}

void uart_putch(char c)
{
  if (c == '\n')
    uart_putch('\r');

  while (!(inb_p(uart_port_base + UART_LSR) & LSR_TX_READY));
  outb_p(uart_port_base + UART_RXTX, c);
}

void uart_puts(const char *str)
{
  for (char c; (c = *str++);)
    uart_putch(c);
}
