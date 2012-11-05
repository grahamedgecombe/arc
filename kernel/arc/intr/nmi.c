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

#include <arc/intr/nmi.h>
#include <arc/intr/route.h>
#include <arc/util/container.h>
#include <arc/util/list.h>
#include <arc/panic.h>
#include <stdlib.h>

static list_t nmi_list;

typedef struct
{
  irq_tuple_t tuple;
  list_node_t node;
} nmi_t;

static void nmi_handle(intr_state_t *state)
{
  panic("non-maskable interrupt - possible hardware failure?");
}

void nmi_init(void)
{
  if (!intr_route_intr(FAULT2, &nmi_handle))
    panic("failed to route NMI");

  list_for_each(&nmi_list, node)
  {
    nmi_t *nmi = container_of(node, nmi_t, node);

    /*
     * route the NMI to interrupt 2, this must be done using a raw IRQ entry
     * this way to avoid collisions with another IRQ - the NMI handler *always*
     * panic()s, so it cannot be shared
     */
    if (!intr_route_irq_to(&nmi->tuple, FAULT2))
      panic("failed to route NMI");
  }

  // TODO: free unused nmi structures?
}

bool nmi_add(irq_tuple_t tuple)
{
  nmi_t *nmi = malloc(sizeof(*nmi));
  if (!nmi)
    return false;

  nmi->tuple = tuple;
  list_add_tail(&nmi_list, &nmi->node);
  return true;
}
