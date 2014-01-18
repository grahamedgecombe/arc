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

#include <arc/stacktrace.h>
#include <arc/cpu/rbp.h>
#include <arc/mm/common.h>
#include <arc/mm/phy32.h>
#include <arc/proc/elf64.h>
#include <arc/trace.h>
#include <stdbool.h>
#include <string.h>

static uintptr_t strtab; // TODO check if this points to a non-reserved section of memory
static size_t strtabsz;

static elf64_sym_t *symtab; // TODO as above
static size_t symtabsz;

#if defined(__OPTIMIZE__)
/* not used when optimizations enabled - see below */
static const char *stacktrace_lookup_sym(uint64_t rip) __attribute__((__unused__));
#endif

static const char *stacktrace_lookup_sym(uint64_t rip)
{
  if (strtab && symtab)
  {
    elf64_sym_t *sym = symtab;
    for (size_t i = 0; i < (symtabsz / sizeof(*symtab)); i++, sym++)
    {
      if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC)
        continue;

      uint64_t addr = (uint64_t) sym->st_value;
      if (rip >= addr && rip < (addr + sym->st_size))
        return (const char *) (strtab + sym->st_name);
    }
  }

  return "<unknown>";
}

void stacktrace_init(multiboot_t *multiboot)
{
  multiboot_tag_t *tag = multiboot_get(multiboot, MULTIBOOT_TAG_ELF);
  if (tag)
  {
    elf64_shdr_t *shdr = (elf64_shdr_t *) tag->elf.data;
    uintptr_t shstrtab = aphy32_to_virt(shdr[tag->elf.sh_shstrndx].sh_addr);

    for (size_t i = 0; i < tag->elf.sh_num; i++, shdr++)
    {
      if (!shdr->sh_addr)
        continue;

      const char *name = (const char *) (shstrtab + shdr->sh_name);
      if (!strcmp(name, ".strtab"))
      {
        strtab = aphy32_to_virt(shdr->sh_addr);
        strtabsz = shdr->sh_size;
      }
      else if (!strcmp(name, ".symtab"))
      {
        symtab = (elf64_sym_t *) aphy32_to_virt(shdr->sh_addr);
        symtabsz = shdr->sh_size;
      }
    }
  }
}

void stacktrace_emit(void)
{
#if defined(__OPTIMIZE__)
  /*
   * x86-64 uses RBP as a regular register if optimizations are enabled,
   * meaning the code below does not work.
   */
  trace_puts("  (Unavailable as optimizations are enabled.)\n");
#else
  uint64_t *rbp = (uint64_t *) rbp_read();
  while (rbp)
  {
    uint64_t rip = rbp[1];
    trace_printf("  [%0#18x]: %s\n", rip, stacktrace_lookup_sym(rip));

    rbp = (uint64_t *) rbp[0];
  }
#endif
}
