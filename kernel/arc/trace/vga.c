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

#include <arc/trace/vga.h>
#include <arc/bda.h>
#include <arc/cpu/port.h>
#include <arc/mm/phy32.h>
#include <stdbool.h>
#include <string.h>

/* the dimensions of the terminal */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* the tab width */
#define TAB_WIDTH 4

/* the physical address of the video buffers */
#define VIDEO_BUFFER 0xB8000

/* CRTC addresses */
#define CRTC_CURSOR_LOW  0x0F
#define CRTC_CURSOR_HIGH 0x0E

/* turns row and column into an index into the video buffer */
#define VGA_POS(r, c) ((r) * VGA_WIDTH + (c))

/* the VGA port base */
static uint16_t vga_port_base;

/* a pointer to the video buffer */
static uint16_t *video_buf;
static uint16_t shadow_video_buf[VGA_WIDTH * VGA_HEIGHT];

/* cursor row and column */
static uint8_t row, col;

/* cursor attributes */
static uint8_t attrib;

/* flags which indicate what is dirty */
static bool dirty_cursor, dirty_text;

/* updates the position of the hardware cursor */
static void crtc_sync(void)
{
  /* get the index of the cursor */
  uint16_t index = VGA_POS(row, col);

  /* get the addresses of the CRTC ports */
  uint16_t crtc_addr = vga_port_base;
  uint16_t crtc_data = vga_port_base + 1;

  /* set the low cursor byte */
  outb_p(crtc_addr, CRTC_CURSOR_LOW);
  outb_p(crtc_data, (uint8_t) (index & 0xFF));

  /* set the high cursor byte */
  outb_p(crtc_addr, CRTC_CURSOR_HIGH);
  outb_p(crtc_data, (uint8_t) ((index >> 8) & 0xFF));
}

/* initializes the terminal */
void vga_init(void)
{
  /* grab the VGA port base from the BIOS data area */
  vga_port_base = bda_reads(BDA_VGA_PORT);

  /* set the pointer to the video buffer */
  video_buf = (uint16_t *) aphy32_to_virt(VIDEO_BUFFER);
  
  /* reset the cursor and the attributes */
  row = 0;
  col = 0;
  attrib = 0x07;

  /* clear the screen */
  for (int i = 0; i < (VGA_WIDTH * VGA_HEIGHT); i++)
    shadow_video_buf[i] = (attrib << 8);

  /* set flags and sync */
  dirty_text = true;
  dirty_cursor = true;
  vga_sync();
}

void vga_puts(const char *str)
{
  for (char c; (c = *str++);)
    vga_putch(c);
}

void vga_putch(char c)
{
  /* allocate some room for temp vars */
  int tmp;

  /* process the character */
  switch (c)
  {
    case '\0':
    case '\f':
    case '\v':
    case '\a':
      /* don't even bother with these */
      break;
    case '\n':
      row++;
      /* fall through */
    case '\r':
      col = 0;
      dirty_cursor = true;
      break;
    case '\t':
      tmp = col % TAB_WIDTH;
      if (tmp != 0)
      {
        col += (TAB_WIDTH - tmp);
        dirty_cursor = true;
      }
      break;
    case '\b':
      if (col > 0)
      {
        col--;
        dirty_cursor = true;
      }
      shadow_video_buf[VGA_POS(row, col)] = (attrib << 8);
      dirty_text = true;
      break;
    default:
      shadow_video_buf[VGA_POS(row, col++)] = (attrib << 8) | c;
      dirty_text = true;
      dirty_cursor = true;
      break;
  }

  /* perform wrapping */
  if (col >= VGA_WIDTH)
  {
    col = 0;
    row++;

    dirty_cursor = true;
  }

  /* perform scrolling */
  if (row >= VGA_HEIGHT)
  {
    /* shift all lines up */
    size_t size = (VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(*shadow_video_buf);
    memmove(shadow_video_buf, shadow_video_buf + VGA_WIDTH, size);

    /* clear the last line */
    for (int i = 0; i < VGA_WIDTH; i++)
      shadow_video_buf[VGA_POS(VGA_HEIGHT - 1, i)] = (attrib << 8);

    /* update the cursor position */
    row--;

    /* the text and cursor must be refreshed */
    dirty_text = true;
    dirty_cursor = true;
  }
}

/* copys the shadow buffer to the real buffer */
void vga_sync(void)
{
  if (dirty_text)
    memcpy(video_buf, shadow_video_buf, sizeof(shadow_video_buf));

  if (dirty_cursor)
    crtc_sync();
}
