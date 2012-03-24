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

#include <arc/util/list.h>
#include <assert.h>

void list_init(list_t *list)
{
  list->head = 0;
  list->tail = 0;
  list->size = 0;
}

void list_add_head(list_t *list, list_node_t *node)
{
  if (!list->head)
  {
    assert(list->tail == 0);

    list->head = list->tail = node;
    node->next = node->prev = 0;
  }
  else
  {
    node->prev = 0;
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }

  list->size++;
}

void list_add_tail(list_t *list, list_node_t *node)
{
  if (!list->tail)
  {
    assert(list->head == 0);

    list->head = list->tail = node;
    node->next = node->prev = 0;
  }
  else
  {
    node->next = 0;
    node->prev = list->tail;
    list->tail->next = node;
    list->tail = node;
  }

  list->size++;
}

void list_remove(list_t *list, list_node_t *node)
{
  if (!node->prev)
    list->head = node->next;
  else
    node->prev->next = node->next;

  if (!node->next)
    list->tail = node->prev;
  else
    node->next->prev = node->prev;

  list->size--;
}

