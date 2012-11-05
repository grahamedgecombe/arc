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

#ifndef ARC_UTIL_HASHTAB_H
#define ARC_UTIL_HASHTAB_H

#include <stdbool.h>
#include <stddef.h>

typedef struct hashtab_node
{
  struct hashtab_node *next;
  void *key, *value;
} hashtab_node_t;

typedef size_t (*hashtab_hash_t)(void *key);
typedef bool (*hashtab_eql_t)(void *key1, void *key2);

typedef struct
{
  hashtab_node_t **buckets;
  size_t bucket_count;
  hashtab_hash_t hash_func;
  hashtab_eql_t eql_func;
} hashtab_t;

bool hashtab_init(hashtab_t *hashtab, hashtab_hash_t hash_func, hashtab_eql_t eql_func, size_t bucket_count);
hashtab_node_t *hashtab_get(hashtab_t *hashtab, void *key);
bool hashtab_put(hashtab_t *hashtab, void *key, void *value);
void hashtab_remove(hashtab_t *hashtab, void *key);

#endif
