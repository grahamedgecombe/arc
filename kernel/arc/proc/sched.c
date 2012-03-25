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

#include <arc/proc/sched.h>
#include <arc/smp/mode.h>
#include <arc/time/apic.h>
#include <arc/time/pit.h>
#include <arc/tty.h>

#define SCHED_TIMESLICE 50 /* 10ms = 100Hz */

void sched_init(void)
{
  if (smp_mode == MODE_SMP)
    apic_monotonic(SCHED_TIMESLICE, &sched_tick);
  else
    pit_monotonic(SCHED_TIMESLICE, &sched_tick);
}

void sched_tick(intr_state_t *state)
{
  tty_putch('.');
}

