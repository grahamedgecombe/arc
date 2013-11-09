#
#  Copyright (c) 2011-2013 Graham Edgecombe <graham@grahamedgecombe.com>
#
#  Permission to use, copy, modify, and/or distribute this software for any
#  purpose with or without fee is hereby granted, provided that the above
#  copyright notice and this permission notice appear in all copies.
#
#  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

ARCH := x86_64-pc-elf

CC := $(ARCH)-gcc
CFLAGS := -std=c1x -O3 -Wall -Wextra -pedantic -Wno-unused-parameter

AS := nasm
ASFLAGS := -f elf64

LD := $(ARCH)-ld
LDFLAGS := -z max-page-size=0x1000

AR := $(ARCH)-ar

# if clang is used, we must specify the architecture on its command line rather
# than in the name of the executable itself
ifeq ($(CC),clang)
  CFLAGS += -ccc-host-triple $(ARCH)
endif

# nasm supports the -Ox command for performing multi-pass optimizations
ifeq ($(AS),nasm)
  ASFLAGS += -Ox
endif
