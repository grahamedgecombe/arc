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

#include <arc/util/hashtab.h>
#include <stdlib.h>

bool hashtab_init(hashtab_t *hashtab, hashtab_hash_t hash_func, hashtab_eql_t eql_func, size_t bucket_count)
{
  hashtab->buckets = malloc(sizeof(*hashtab->buckets) * bucket_count);
  if (!hashtab->buckets)
    return false;

  hashtab->bucket_count = bucket_count;
  hashtab->hash_func = hash_func;
  hashtab->eql_func = eql_func;
  return true;
}

hashtab_node_t *hashtab_get(hashtab_t *hashtab, void *key)
{
  size_t hash = (*hashtab->hash_func)(key) % hashtab->bucket_count;
  for (hashtab_node_t *node = hashtab->buckets[hash]; node; node = node->next)
  {
    if ((*hashtab->eql_func)(key, node->key))
      return node;
  }

  return 0;
}

bool hashtab_put(hashtab_t *hashtab, void *key, void *value)
{
  size_t hash = (*hashtab->hash_func)(key) % hashtab->bucket_count;
  for (hashtab_node_t *node = hashtab->buckets[hash]; node; node = node->next)
  {
    if ((*hashtab->eql_func)(key, node->key))
    {
      node->value = value;
      return true;
    }
  }

  hashtab_node_t *node = malloc(sizeof(*node));
  if (!node)
    return false;

  node->key = key;
  node->value = value;
  node->next = hashtab->buckets[hash];
  hashtab->buckets[hash] = node;
  return true;
}

void hashtab_remove(hashtab_t *hashtab, void *key)
{
  size_t hash = (*hashtab->hash_func)(key) % hashtab->bucket_count;
  for (hashtab_node_t *prev = 0, *node = hashtab->buckets[hash]; node; prev = node, node = node->next)
  {
    if ((*hashtab->eql_func)(key, node->key))
    {
      if (prev == 0)
        hashtab->buckets[hash] = node->next;
      else
        prev->next = node->next;

      free(node);
      return;
    }
  }
}
