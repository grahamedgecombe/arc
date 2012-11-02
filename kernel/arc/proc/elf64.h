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

#ifndef ARC_PROC_ELF64_H
#define ARC_PROC_ELF64_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ehdr ident indexes */
#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_ABIVERSION 8
#define EI_PAD        9
#define EI_NIDENT     16

/* EI_MAG* values */
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

/* EI_CLASS values */
#define ELFCLASS64 2

/* EI_DATA values */
#define ELFDATA2LSB 1

/* EI_OSABI values */
#define ELFOSABI_SYSV 0

/* ehdr machine values */
#define EM_X86_64 62

/* ehdr types */
#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3
#define ET_CORE 4

/* ehdr phnum special value */
#define PN_XNUM 0xFFFF // TODO: read the real phnum from shdr[0].sh_info

/* phdr types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

/* phdr flags */
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef uint64_t elf64_addr_t;
typedef uint64_t elf64_off_t;
typedef uint16_t elf64_half_t;
typedef uint32_t elf64_word_t;
typedef int32_t  elf64_sword_t;
typedef uint64_t elf64_xword_t;
typedef int64_t  elf64_sxword_t;

typedef struct
{
  unsigned char e_ident[EI_NIDENT];
  elf64_half_t  e_type;
  elf64_half_t  e_machine;
  elf64_word_t  e_version;
  elf64_addr_t  e_entry;
  elf64_off_t   e_phoff;
  elf64_off_t   e_shoff;
  elf64_word_t  e_flags;
  elf64_half_t  e_ehsize;
  elf64_half_t  e_phentsize;
  elf64_half_t  e_phnum;
  elf64_half_t  e_shentsize;
  elf64_half_t  e_shnum;
  elf64_half_t  e_shstrndx;
} __attribute__((__packed__)) elf64_ehdr_t;

typedef struct
{
  elf64_word_t  p_type;
  elf64_word_t  p_flags;
  elf64_off_t   p_offset;
  elf64_addr_t  p_vaddr;
  elf64_addr_t  p_paddr;
  elf64_xword_t p_filesz;
  elf64_xword_t p_memsz;
  elf64_xword_t p_align;
} __attribute__((__packed__)) elf64_phdr_t;

bool elf64_load(elf64_ehdr_t *elf, size_t size);

#endif

