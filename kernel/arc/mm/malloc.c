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

#include <arc/mm/malloc.h>
#include <arc/mm/heap.h>
#include <string.h>

spinlock_t malloc_lock = SPIN_UNLOCKED;

/*
 * note: these are only functions required for dlmalloc to work, for the bulk
 * of the allocator see dlmalloc.c
 */

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
  vm_acc_t vm_flags = 0;
  if (prot & PROT_READ)
    vm_flags |= VM_R;
  if (prot & PROT_WRITE)
    vm_flags |= VM_W;
  if (prot & PROT_EXEC)
    vm_flags |= VM_X;

  void *ptr = heap_alloc(len, vm_flags);
  if (!ptr)
    return MAP_FAILED;

  memclr(ptr, len);
  return ptr;
}

int munmap(void *addr, size_t len)
{
  heap_free(addr);
  return 0;
}
