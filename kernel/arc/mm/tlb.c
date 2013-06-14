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

#include <arc/mm/tlb.h>
#include <arc/cpu/tlb.h>
#include <arc/cpu/intr.h>
#include <arc/intr/common.h>
#include <arc/intr/route.h>
#include <arc/intr/apic.h>
#include <arc/lock/spinlock.h>
#include <arc/smp/cpu.h>
#include <arc/smp/mode.h>
#include <arc/panic.h>
#include <stdbool.h>
#include <stddef.h>

#define TLB_OP_QUEUE_SIZE 16

/* flag which indicates if tlb_init() has been called */
static bool tlb_initialised = false;

/* queue of pending TLB operations */
static tlb_op_t tlb_op_queue[TLB_OP_QUEUE_SIZE];
static size_t tlb_op_ptr = 0;

/*
 * a lock which must be held for a processor before it starts to send IPIs to
 * co-ordinate all the other processors for a TLB shootdown
 */
static spinlock_t tlb_transaction_lock = SPIN_UNLOCKED;

/* number of cpus being waited for */
static int tlb_wait_cpus;
static spinlock_t tlb_wait_lock = SPIN_UNLOCKED;

/* ready to commit flag */
static bool tlb_commit;
static spinlock_t tlb_commit_lock = SPIN_UNLOCKED;

static void tlb_handle_ops(void)
{
  for (size_t i = 0; i < tlb_op_ptr; i++)
  {
    tlb_op_t *op = &tlb_op_queue[i];
    switch (op->type)
    {
      case TLB_OP_INVLPG:
        tlb_invlpg(op->addr);
        break;

      case TLB_OP_FLUSH:
        tlb_flush();
        break;
    }
  }
}

static void tlb_handle_ipi(cpu_state_t *state)
{
  /* acknowledge that this CPU is ready to flush its TLB */
  spin_lock(&tlb_wait_lock);
  tlb_wait_cpus--;
  spin_unlock(&tlb_wait_lock);

  /* wait for the commit flag to be set */
  bool commit;
  do
  {
    spin_lock(&tlb_commit_lock);
    commit = tlb_commit;
    spin_unlock(&tlb_commit_lock);
  } while (!commit);

  /* iterate through the op queue */
  tlb_handle_ops();

  /* acknowledge that this CPU has flushed its TLB */
  spin_lock(&tlb_wait_lock);
  tlb_wait_cpus--;
  spin_unlock(&tlb_wait_lock);
}

void tlb_init(void)
{
  if (!intr_route_intr(IPI_TLB, &tlb_handle_ipi))
    panic("failed to route TLB shootdown IPI");

  tlb_initialised = true;
}

void tlb_transaction_init(void)
{
  spin_lock(&tlb_transaction_lock);

  /* reset some values */
  tlb_commit = false;
  tlb_wait_cpus = cpu_list.size - 1;

  /* if we're in SMP mode, stop all the other CPUs */
  if (smp_mode == MODE_SMP)
  {
    if (!tlb_initialised)
      panic("TLB shootdown in SMP mode before IPI routed");

    /* send the IPIs to all CPUs but this one */
    apic_ipi_all_exc_self(IPI_TLB);

    /* wait for all CPUs to respond to the IPI */
    int wait_cpus;
    do
    {
      spin_lock(&tlb_wait_lock);
      wait_cpus = tlb_wait_cpus;
      spin_unlock(&tlb_wait_lock);
    } while (wait_cpus != 0);
  }
}

void tlb_transaction_queue_invlpg(uintptr_t addr)
{
  if (tlb_op_ptr >= TLB_OP_QUEUE_SIZE)
  {
    tlb_transaction_queue_flush();
  }
  else
  {
    tlb_op_t *op = &tlb_op_queue[tlb_op_ptr++];
    op->type = TLB_OP_INVLPG;
    op->addr = addr;
  }
}

void tlb_transaction_queue_flush(void)
{
  tlb_op_ptr = 1;
  tlb_op_queue[0].type = TLB_OP_FLUSH;
}

void tlb_transaction_rollback(void)
{
  /* resetting the op queue and commiting has the same effect as a rollback */
  tlb_op_ptr = 0;
  tlb_transaction_commit();
}

void tlb_transaction_commit(void)
{
  /* reset cpu waiting counter */
  tlb_wait_cpus = cpu_list.size - 1;

  /* set the commit flag to make other processors handle the op queue */
  if (smp_mode == MODE_SMP)
  {
    spin_lock(&tlb_commit_lock);
    tlb_commit = true;
    spin_unlock(&tlb_commit_lock);
  }

  /* iterate through the op queue on the calling processor */
  tlb_handle_ops();

  /* wait for all cpus to finish dealing with their tlb */
  if (smp_mode == MODE_SMP)
  {
    int wait_cpus;
    do
    {
      spin_lock(&tlb_wait_lock);
      wait_cpus = tlb_wait_cpus;
      spin_unlock(&tlb_wait_lock);
    } while (wait_cpus != 0);
  }

  /* reset the queue */
  tlb_op_ptr = 0;

  /* all done, we can release the master lock */
  spin_unlock(&tlb_transaction_lock);
}
