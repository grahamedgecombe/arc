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

#include <arc/proc/elf64.h>

bool elf64_ehdr_valid(elf64_ehdr_t *ehdr)
{
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    return false;

  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return false;

  if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    return false;

  if (ehdr->e_machine != EM_X86_64)
    return false;

  if (ehdr->e_version != 1)
    return false;

  return true;
}

