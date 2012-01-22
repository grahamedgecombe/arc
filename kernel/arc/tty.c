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

#include <arc/tty.h>
#include <arc/bda.h>
#include <arc/cpu/port.h>
#include <arc/lock/spinlock.h>
#include <arc/mm/phy32.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* the physical addresses of the video buffers */
#define VIDEO_BUFFER_MGA 0xB0000
#define VIDEO_BUFFER_CGA 0xB8000

/* CRTC addresses */
#define CRTC_CURSOR_LOW  0x0F
#define CRTC_CURSOR_HIGH 0x0E

/* the tab width */
#define TAB_WIDTH 4

/* turns row and column into an index into the video buffer */
#define TTY_POS(r, c) ((r) * TTY_WIDTH + (c))

/* tty_vprintf() flags */
#define FLAG_JUSTIFY 0x1
#define FLAG_PLUS    0x2
#define FLAG_SPACE   0x4
#define FLAG_HASH    0x8
#define FLAG_ZERO    0x10

/* the tty lock */
static spinlock_t tty_lock = SPIN_UNLOCKED;

/* the VGA port base */
static uint16_t vga_port_base;

/* a pointer to the video buffer */
static uint16_t *video_buf;
static uint16_t shadow_video_buf[TTY_WIDTH * TTY_HEIGHT];

/* cursor row and column */
static uint8_t row, col;

/* cursor attributes */
static uint8_t attrib;

/* flags which indicate what is dirty */
static bool dirty_cursor, dirty_text;

/* forward declarations for some internal functions */
static void _tty_putch(char ch);
static void _tty_puts(const char *str);

/* reverses a string */
static char *reverse(char *str)
{
  for (int i = 0, j = strlen(str) - 1; i < j; i++, j--)
  {
    char c = str[i];
    str[i] = str[j];
    str[j] = c;
  }
  return str;
}

/* converts an integer to a string */
static char *itoa_64(int64_t signed_value, char *str, int base)
{
  if (base < 1 || base > 36)
    return 0;

  uint64_t value;
  if (signed_value >= 0)
    value = signed_value;
  else
    value = -signed_value;

  int pos = 0;
  do
  {
    str[pos] = value % base + '0';
    if (str[pos] > '9')
      str[pos] += 7;
    pos++;
  } while ((value /= base) > 0);

  if (signed_value < 0)
    str[pos++] = '-';

  str[pos] = '\0';
  reverse(str);

  return str;
}

/* converts an unsigned integer to a string */
static char *uitoa_64(uint64_t value, char *str, int base)
{
  if (base < 1 || base > 36)
    return 0;

  int pos = 0;
  do
  {
    str[pos] = value % base + '0';
    if (str[pos] > '9')
      str[pos] += 7;
    pos++;
  } while ((value /= base) > 0);

  str[pos] = '\0';
  reverse(str);

  return str;
}

/* checks if the character is a digit (so we do not need ctype) */
static int is_ascii_digit(int c)
{
  return c >= '0' && c <= '9';
}

/* converts the string to an integer and increments the pointer */
static int skip_atoi(const char **str)
{
  int i = 0;
  while (is_ascii_digit(**str))
    i = i * 10 + *((*str)++) - '0';
  return i;
}

/* updates the position of the hardware cursor */
static void crtc_sync(void)
{
  /* get the index of the cursor */
  uint16_t index = TTY_POS(row, col);

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

/* copys the shadow buffer to the real buffer */
static void tty_sync(void)
{
  if (dirty_text)
    memcpy(video_buf, shadow_video_buf, sizeof(shadow_video_buf));

  if (dirty_cursor)
    crtc_sync();
}

/* initializes the terminal */
void tty_init(void)
{
  /* grab the VGA mode and port base from the BIOS data area */
  uint8_t vga_mode = bda_read(BDA_VGA_MODE);
  vga_port_base = bda_reads(BDA_VGA_PORT);

  /* grab the appropriate pointer to the video buffer */
  bool mga = (vga_mode & 0x30) == 0x30;
  video_buf = (uint16_t *) aphy32_to_virt(mga ? VIDEO_BUFFER_MGA : VIDEO_BUFFER_CGA);

  /* reset the cursor and the attributes */
  row = 0;
  col = 0;
  attrib = 0x07;

  /* clear the screen */
  for (int i = 0; i < (TTY_WIDTH * TTY_HEIGHT); i++)
    shadow_video_buf[i] = (attrib << 8);

  /* set flags and sync */
  dirty_text = true;
  dirty_cursor = true;
  tty_sync();
}

/* atomically writes a string */
void tty_puts(const char *str)
{
  spin_lock(&tty_lock);
  _tty_puts(str);
  tty_sync();
  spin_unlock(&tty_lock);
}

/* atomically writes a character */
void tty_putch(char ch)
{
  spin_lock(&tty_lock);
  _tty_putch(ch);
  tty_sync();
  spin_unlock(&tty_lock);
}

/* internal function for writing a string */
static void _tty_puts(const char *str)
{
  for (char c; (c = *str++);)
    _tty_putch(c);
}

/* internal function for writing a character */
static void _tty_putch(char c)
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
    shadow_video_buf[TTY_POS(row, col)] = (attrib << 8);
    dirty_text = true;
    break;
  default:
    shadow_video_buf[TTY_POS(row, col++)] = (attrib << 8) | c;
    dirty_text = true;
    dirty_cursor = true;
    break;
  }

  /* perform wrapping */
  if (col >= TTY_WIDTH)
  {
    col = 0;
    row++;

    dirty_cursor = true;
  }

  /* perform scrolling */
  if (row >= TTY_HEIGHT)
  {
    /* shift all lines up */
    size_t size = (TTY_HEIGHT - 1) * TTY_WIDTH * sizeof(*shadow_video_buf);
    memmove(shadow_video_buf, shadow_video_buf + TTY_WIDTH, size);

    /* clear the last line */
    for (int i = 0; i < TTY_WIDTH; i++)
      shadow_video_buf[TTY_POS(TTY_HEIGHT - 1, i)] = (attrib << 8);

    /* update the cursor position */
    row--;

    /* the text and cursor must be refreshed */
    dirty_text = true;
    dirty_cursor = true;
  }
}

/* print a formatted string to the terminal */
void tty_printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  tty_vprintf(fmt, args);
  va_end(args);
}

/* print a formatted string to the terminal using the given va_list */
void tty_vprintf(const char *fmt, va_list args)
{
  spin_lock(&tty_lock);
  char c;
  while ((c = *fmt++))
  {
    if (c != '%')
    {
      _tty_putch(c);
    }
    else
    {
      /* process flags */
      int flags = 0;
      bool more_flags = true;
      while (more_flags)
      {
        switch (*fmt++)
        {
        case '-':
          flags |= FLAG_JUSTIFY;
          break;
        case '+':
          flags |= FLAG_PLUS;
          break;
        case ' ':
          flags |= FLAG_SPACE;
          break;
        case '#':
          flags |= FLAG_HASH;
          break;
        case '0':
          flags |= FLAG_ZERO;
          break;
        default:
          fmt--;
          more_flags = false;
          break;
        }
      }

      /* process width */
      int width = -1;
      if (is_ascii_digit(*fmt))
      {
        width = skip_atoi(&fmt);
      }
      else if (*fmt == '*')
      {
        fmt++;
        width = va_arg(args, int);
        if (width < 0)
          width = 0;
      }

      /* process precision */
      int precision = -1;
      if (*fmt == '.')
      {
        fmt++;
        if (is_ascii_digit(*fmt))
          precision = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
          fmt++;
          precision = va_arg(args, int);
          if (precision < 0)
            precision = 0;
        }
      }

      /* process qualifier */
      int qualifier = -1;
      if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        qualifier = *fmt++;

      /* process specifier */
      c = *fmt++;
      switch (c)
      {
      case '%':
        _tty_putch('%');
        break;
      case 'c':
        _tty_putch((char) va_arg(args, int));
        break;
      case 's':
        _tty_puts(va_arg(args, const char *));
        break;
      case 'u':
      case 'd':
      case 'i':
      case 'x':
      case 'X':
        {
          /* check if we are dealing with a signed value */
          int base = (c == 'x' || c == 'X') ? 16 : 10;
          bool sign = base != 16 && c != 'u';

          /* grab the value */
          int64_t value = va_arg(args, int64_t);

          /* cast the value to a smaller one if the qualifier is specified */
          if (qualifier == 'h')
          {
            if (sign)
              value = (signed short int) value;
            else
              value = (unsigned short int) value;
          }
          else if (qualifier == 'l')
          {
            if (sign)
              value = (signed long int) value;
            else
              value = (unsigned long int) value;
          }

          /* convert the value to a string */
          char buf[64];
          if (sign)
            itoa_64(value, buf, base);
          else
            uitoa_64(value, buf, base);
          int len = strlen(buf);

          /* add the 0x prefix */
          if (flags & FLAG_HASH && base == 16)
          {
            _tty_puts("0x");
            len += 2;
          }

          /* add a plus or a space if the number is not negative */
          if (((flags & FLAG_PLUS) || (flags & FLAG_SPACE)) && buf[0] != '-' && c != 'u' && base != 6)
          {
            _tty_putch((flags & FLAG_PLUS) ? '+' : ' ');
            len++;
          }

          /* perform left justification / zero padding */
          if (width > len && ((flags & FLAG_JUSTIFY) || (flags & FLAG_ZERO)))
          {
            int pad = width - len;
            for (int i = 0; i < pad; i++)
              _tty_putch((flags & FLAG_ZERO) ? '0' : ' ');
            len += pad;
          }

          /* print the actual buffer */
          _tty_puts(buf);

          /* perform right justification */
          if (width > len && !((flags & FLAG_JUSTIFY) || (flags & FLAG_ZERO)))
          {
            int pad = width - len;
            for (int i = 0; i < pad; i++)
              _tty_putch(' ');
            len += pad;
          }
        }
      }
    }
  }

  tty_sync();
  spin_unlock(&tty_lock);
}

