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

#include <arc/panic.h>
#include <arc/trace.h>
#include <arc/smp/mode.h>
#include <arc/cpu/intr.h>
#include <arc/cpu/halt.h>
#include <arc/intr/common.h>
#include <arc/intr/route.h>
#include <arc/intr/apic.h>

void panic_init(void)
{
  if (!intr_route_intr(IPI_PANIC, &panic_handle_ipi))
    panic("failed to route panic IPI");
}

void panic_handle_ipi(cpu_state_t *state)
{
  intr_disable();
  halt_forever();
}

void panic(const char *message, ...)
{
  va_list args;
  va_start(args, message);
  vpanic(message, args);
  va_end(args);
}

void vpanic(const char *message, va_list args)
{
  if (smp_mode == MODE_SMP)
    apic_ipi_all_exc_self(IPI_PANIC);

  trace_puts("PANIC: ");
  trace_vprintf(message, args);
  trace_puts("\n");

  intr_disable();
  halt_forever();
}
