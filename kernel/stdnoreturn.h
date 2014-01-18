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

#ifndef STDNORETURN_H
#define STDNORETURN_H

#if defined(__clang__) && __clang_major__ == 3 && __clang_minor__ == 0
#define noreturn
#elif !defined(__OPTIMIZE__)
/*
 * _Noreturn confuses stack traces as the CALL places RIP+n on the stack (where
 * n is the number of bytes of the encoded CALL instruction). However, as gcc
 * optimizes away the function's exit routine, this actually makes RIP+n be the
 * start of the _following_ function, making us show the incorrect symbol name.
 *
 * Therefore if optimization is disabled we don't add _Noreturn to functions,
 * which makes debugging easier.
 */
#define noreturn
#else
#define noreturn _Noreturn
#endif

#endif
