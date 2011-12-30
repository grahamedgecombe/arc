/*
 * Copyright (c) 2011 Graham Edgecombe <graham@grahamedgecombe.com>
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

#include <arc/init.h>
#include <arc/tty.h>
#include <arc/mm/map.h>
#include <arc/mm/phy32.h>
#include <arc/intr/idt.h>
#include <string.h>

static void print_banner(void)
{
  const char *banner = "Arc Operating System";
  int gap_len = TTY_WIDTH / 2 - strlen(banner) / 2;

  for (int i = 0; i < TTY_WIDTH; i++)
    tty_putch('-');

  for (int i = 0; i < gap_len; i++)
    tty_putch(' ');

  tty_puts(banner);

  for (int i = 0; i < gap_len; i++)
    tty_putch(' ');

  for (int i = 0; i < TTY_WIDTH; i++)
    tty_putch('-');
}

void init(uint32_t magic, multiboot_t *multiboot)
{
  /* initialise tty driver */
  tty_init();

  /* print banner */
  print_banner();
  
  /* check the multiboot magic number */
  if (magic != MULTIBOOT_MAGIC)
  {
    tty_printf("ERROR: invalid multiboot magic (expected 0x%x, got 0x%x)\n",
      MULTIBOOT_MAGIC, magic);
    return;
  }

  /* convert physical 32-bit multiboot address to virtual address */
  multiboot = phy32_to_virt(multiboot);
  if (!(multiboot->flags & MULTIBOOT_FLAGS_MMAP))
  {
    tty_printf("ERROR: no memory map passed by boot loader\n");
    return;
  }

  /* map physical memory */
  mm_map_init(multiboot);

  /* set up the IDT */
  idt_init();
}

