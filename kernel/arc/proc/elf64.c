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

static bool elf64_ehdr_valid(elf64_ehdr_t *ehdr)
{
  if (ehdr->e_ident[EI_MAG0] != ELFMAG0)
    return false;

  if (ehdr->e_ident[EI_MAG1] != ELFMAG1)
    return false;

  if (ehdr->e_ident[EI_MAG2] != ELFMAG2)
    return false;

  if (ehdr->e_ident[EI_MAG3] != ELFMAG3)
    return false;

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    return false;

  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return false;

  if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    return false;

  if (ehdr->e_type != ET_EXEC)
    return false;

  if (ehdr->e_machine != EM_X86_64)
    return false;

  if (ehdr->e_version != 1)
    return false;

  return true;
}

bool elf64_load(elf64_ehdr_t *elf, size_t size)
{
  if (!elf64_ehdr_valid(elf))
    return false;

  elf64_phdr_t *phdrs = (elf64_phdr_t *) ((uintptr_t) elf + elf->e_phoff);
  for (size_t i = 0; i < elf->e_phnum; i++) {
    elf64_phdr_t *phdr = &phdrs[i];
    if (phdr->p_type != PT_LOAD)
      continue;


  }

  return true;
}

