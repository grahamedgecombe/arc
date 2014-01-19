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

#ifndef ARC_PROC_SCHED_H
#define ARC_PROC_SCHED_H

#include <arc/cpu/state.h>
#include <arc/proc/thread.h>

void sched_init(void);

/*
 * add/remove a thread from the scheduler's queue
 *
 * these functions should _not_ be called directly - use thread_suspend() and
 * thread_resume() instead, as they correctly deal with locking and updating
 * the thread's state.
 */
void sched_thread_resume(thread_t *thread);
void sched_thread_suspend(thread_t *thread);

void sched_tick(cpu_state_t *state);

#endif
