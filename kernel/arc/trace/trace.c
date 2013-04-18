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

#include <arc/trace.h>
#include <arc/trace/tty.h>
#include <arc/trace/uart.h>
#include <arc/trace/e9.h>
#include <arc/lock/spinlock.h>
#include <arc/cmdline.h>
#include <arc/panic.h>
#include <string.h>
#include <stdint.h>

#define max(x,y) (((x) > (y)) ? (x) : (y))

/* backends */
#define TRACE_TTY  0x1
#define TRACE_UART 0x2
#define TRACE_E9   0x4

/* tty_vprintf() flags */
#define FLAG_JUSTIFY 0x1
#define FLAG_PLUS    0x2
#define FLAG_SPACE   0x4
#define FLAG_HASH    0x8
#define FLAG_ZERO    0x10

static spinlock_t trace_lock = SPIN_UNLOCKED;
static int trace_backends = 0;

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

static void _trace_putch(char c) {
  if (trace_backends & TRACE_TTY)
    tty_putch(c);
  if (trace_backends & TRACE_UART)
    uart_putch(c);
  if (trace_backends & TRACE_E9)
    e9_putch(c);
}

static void _trace_puts(const char *str) {
  if (trace_backends & TRACE_TTY)
    tty_puts(str);
  if (trace_backends & TRACE_UART)
    uart_puts(str);
  if (trace_backends & TRACE_E9)
    e9_puts(str);
}

void trace_init(void)
{
  const char *trace = cmdline_get("trace");
  if (trace)
  {
    for (const char *c = trace, *backend = trace;; c++)
    {
      if (*c == ',' || *c == 0)
      {
        if (memcmp(backend, "tty", max(c - backend, 3)) == 0)
          trace_backends |= TRACE_TTY;
        if (memcmp(backend, "uart", max(c - backend, 4)) == 0)
          trace_backends |= TRACE_UART;
        if (memcmp(backend, "e9", max(c - backend, 2)) == 0)
          trace_backends |= TRACE_E9;

        if (*c)
          backend = c + 1;
        else
          break;
      }
    }
  }

  if (trace_backends & TRACE_TTY)
    tty_init();
  if (trace_backends & TRACE_UART)
    uart_init();
}

void trace_putch(char c)
{
  spin_lock(&trace_lock);

  _trace_putch(c);
  if (trace_backends & TRACE_TTY)
    tty_sync();

  spin_unlock(&trace_lock);
}

void trace_puts(const char *str)
{
  spin_lock(&trace_lock);

  _trace_puts(str);
  if (trace_backends & TRACE_TTY)
    tty_sync();

  spin_unlock(&trace_lock);
}

void trace_printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  trace_vprintf(fmt, args);
  va_end(args);
}

void trace_vprintf(const char *fmt, va_list args)
{
  spin_lock(&trace_lock);

  char c;
  while ((c = *fmt++))
  {
    if (c != '%')
    {
      _trace_putch(c);
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
          _trace_putch('%');
          break;
        case 'c':
          _trace_putch((char) va_arg(args, int));
          break;
        case 's':
          _trace_puts(va_arg(args, const char *));
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
              _trace_puts("0x");
              len += 2;
            }

            /* add a plus or a space if the number is not negative */
            if (((flags & FLAG_PLUS) || (flags & FLAG_SPACE)) && buf[0] != '-' && c != 'u' && base != 6)
            {
              _trace_putch((flags & FLAG_PLUS) ? '+' : ' ');
              len++;
            }

            /* perform left justification / zero padding */
            if (width > len && ((flags & FLAG_JUSTIFY) || (flags & FLAG_ZERO)))
            {
              int pad = width - len;
              for (int i = 0; i < pad; i++)
                _trace_putch((flags & FLAG_ZERO) ? '0' : ' ');
              len += pad;
            }

            /* print the actual buffer */
            _trace_puts(buf);

            /* perform right justification */
            if (width > len && !((flags & FLAG_JUSTIFY) || (flags & FLAG_ZERO)))
            {
              int pad = width - len;
              for (int i = 0; i < pad; i++)
                _trace_putch(' ');
              len += pad;
            }
          }
      }
    }
  }

  if (trace_backends & TRACE_TTY)
    tty_sync();

  spin_unlock(&trace_lock);
}
