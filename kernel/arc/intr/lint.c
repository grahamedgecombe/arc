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

#include <arc/intr/lint.h>
#include <arc/intr/common.h>
#include <arc/intr/route.h>
#include <arc/intr/apic.h>
#include <arc/lock/spinlock.h>
#include <arc/panic.h>
#include <stdbool.h>

/* bit hacky, but it means this doesn't have to be in apic.h */
void apic_write(size_t reg, uint64_t val);

static spinlock_t lint_lock = SPIN_UNLOCKED;
static spinlock_t lint_ack_lock = SPIN_UNLOCKED;
static bool lint_ack;
static size_t lint_reg;
static uint32_t lint_val;

static void lint_route(intr_state_t *state)
{
  apic_write(lint_reg, lint_val);

  spin_lock(&lint_ack_lock);
  lint_ack = true;
  spin_unlock(&lint_ack_lock);
}

static void lint_program(cpu_t *cpu, uint8_t lintn)
{
  if (lintn == 0)
    lint_reg = APIC_LVT_LINT0;
  else if (lintn == 1)
    lint_reg = APIC_LVT_LINT1;
  else
    panic("LINTn out of range");

  cpu_t *self = cpu_get();
  if (cpu == self)
  {
    apic_write(lint_reg, lint_val);
  }
  else
  {
    lint_ack = false;
    apic_ipi_fixed(cpu->lapic_id, IPI_ROUTE);

    bool ack = false;
    do
    {
      spin_lock(&lint_ack_lock);
      ack = lint_ack;
      spin_unlock(&lint_ack_lock);
    } while (!ack);
  }
}

void lint_init(void)
{
  if (!intr_route_intr(IPI_ROUTE, &lint_route))
    panic("failed to route LINTn IRQ routing IPI");
}

void lint_route_nmi(cpu_t *cpu, uint8_t lintn, trigger_t trigger)
{
  spin_lock(&lint_lock);

  lint_val = LVT_TYPE_NMI;
  if (trigger == TRIGGER_LEVEL)
    lint_val |= LVT_TRIGGER_LEVEL;
  else
    lint_val |= LVT_TRIGGER_EDGE;
  lint_program(cpu, lintn);

  spin_unlock(&lint_lock);
}

void lint_unroute_nmi(cpu_t *cpu, uint8_t lintn)
{
  spin_lock(&lint_lock);

  lint_val = LVT_MASKED;
  lint_program(cpu, lintn);

  spin_lock(&lint_lock);
}

