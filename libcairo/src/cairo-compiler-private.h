/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#ifndef CAIRO_COMPILER_PRIVATE_H
#define CAIRO_COMPILER_PRIVATE_H

#include "cairo.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if __GNUC__ >= 3 && defined(__ELF__) && !defined(__sun)
# define slim_hidden_proto(name)		slim_hidden_proto1(name, slim_hidden_int_name(name)) cairo_private
# define slim_hidden_proto_no_warn(name)	slim_hidden_proto1(name, slim_hidden_int_name(name)) cairo_private_no_warn
# define slim_hidden_def(name)			slim_hidden_def1(name, slim_hidden_int_name(name))
# define slim_hidden_int_name(name) INT_##name
# define slim_hidden_proto1(name, internal)				\
  extern __typeof (name) name						\
	__asm__ (slim_hidden_asmname (internal))
# define slim_hidden_def1(name, internal)				\
  extern __typeof (name) EXT_##name __asm__(slim_hidden_asmname(name))	\
	__attribute__((__alias__(slim_hidden_asmname(internal))))
# define slim_hidden_ulp		slim_hidden_ulp1(__USER_LABEL_PREFIX__)
# define slim_hidden_ulp1(x)		slim_hidden_ulp2(x)
# define slim_hidden_ulp2(x)		#x
# define slim_hidden_asmname(name)	slim_hidden_asmname1(name)
# define slim_hidden_asmname1(name)	slim_hidden_ulp #name
#else
# define slim_hidden_proto(name)		int _cairo_dummy_prototype(void)
# define slim_hidden_proto_no_warn(name)	int _cairo_dummy_prototype(void)
# define slim_hidden_def(name)			int _cairo_dummy_prototype(void)
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define CAIRO_PRINTF_FORMAT(fmt_index, va_index) \
	__attribute__((__format__(__printf__, fmt_index, va_index)))
#else
#define CAIRO_PRINTF_FORMAT(fmt_index, va_index)
#endif

/* slim_internal.h */
#define CAIRO_HAS_HIDDEN_SYMBOLS 1
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && defined(__ELF__) && !defined(__sun)
#define cairo_private_no_warn	__attribute__((__visibility__("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define cairo_private_no_warn	__hidden
#else /* not gcc >= 3.3 and not Sun Studio >= 8 */
#define cairo_private_no_warn
#undef CAIRO_HAS_HIDDEN_SYMBOLS
#endif

#ifndef WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT
#endif
/* Add attribute(warn_unused_result) if supported */
#define cairo_warn	    WARN_UNUSED_RESULT
#define cairo_private	    cairo_private_no_warn cairo_warn

/* This macro allow us to deprecate a function by providing an alias
   for the old function name to the new function name. With this
   macro, binary compatibility is preserved. The macro only works on
   some platforms --- tough.

   Meanwhile, new definitions in the public header file break the
   source code so that it will no longer link against the old
   symbols. Instead it will give a descriptive error message
   indicating that the old function has been deprecated by the new
   function.
*/
#if __GNUC__ >= 2 && defined(__ELF__)
# define CAIRO_FUNCTION_ALIAS(old, new)		\
	extern __typeof (new) old		\
	__asm__ ("" #old)			\
	__attribute__((__alias__("" #new)))
#else
# define CAIRO_FUNCTION_ALIAS(old, new)
#endif

/*
 * Cairo uses the following function attributes in order to improve the
 * generated code (effectively by manual inter-procedural analysis).
 *
 *   'cairo_pure': The function is only allowed to read from its arguments
 *                 and global memory (i.e. following a pointer argument or
 *                 accessing a shared variable). The return value should
 *                 only depend on its arguments, and for an identical set of
 *                 arguments should return the same value.
 *
 *   'cairo_const': The function is only allowed to read from its arguments.
 *                  It is not allowed to access global memory. The return
 *                  value should only depend its arguments, and for an
 *                  identical set of arguments should return the same value.
 *                  This is currently the most strict function attribute.
 *
 * Both these function attributes allow gcc to perform CSE and
 * constant-folding, with 'cairo_const 'also guaranteeing that pointer contents
 * do not change across the function call.
 */
#if __GNUC__ >= 3
#define cairo_pure __attribute__((pure))
#define cairo_const __attribute__((const))
#else
#define cairo_pure
#define cairo_const
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _CAIRO_BOOLEAN_EXPR(expr)                   \
 __extension__ ({                               \
   int _cairo_boolean_var_;                         \
   if (expr)                                    \
      _cairo_boolean_var_ = 1;                      \
   else                                         \
      _cairo_boolean_var_ = 0;                      \
   _cairo_boolean_var_;                             \
})
#define likely(expr) (__builtin_expect (_CAIRO_BOOLEAN_EXPR(expr), 1))
#define unlikely(expr) (__builtin_expect (_CAIRO_BOOLEAN_EXPR(expr), 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

#ifndef __GNUC__
#undef __attribute__
#define __attribute__(x)
#endif

#if (defined(__WIN32__) && !defined(__WINE__)) || defined(_MSC_VER)
#define snprintf _snprintf
#define popen _popen
#define pclose _pclose
#define hypot _hypot
#endif

#ifdef _MSC_VER
#undef inline
#define inline __inline
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && 0
/* When compiling with /Gy and /OPT:ICF identical functions will be folded in together.
   The CAIRO_ENSURE_UNIQUE macro ensures that a function is always unique and
   will never be folded into another one. Something like this might eventually
   be needed for GCC but it seems fine for now. */
#define CAIRO_ENSURE_UNIQUE                       \
    do {                                          \
	char func[] = __FUNCTION__;               \
	char file[] = __FILE__;                   \
	__asm {                                   \
	    __asm jmp __internal_skip_line_no     \
	    __asm _emit (__LINE__ & 0xff)         \
	    __asm _emit ((__LINE__>>8) & 0xff)    \
	    __asm _emit ((__LINE__>>16) & 0xff)   \
	    __asm _emit ((__LINE__>>24) & 0xff)   \
	    __asm lea eax, func                   \
	    __asm lea eax, file                   \
	    __asm __internal_skip_line_no:        \
	};                                        \
    } while (0)
#else
#define CAIRO_ENSURE_UNIQUE    do { } while (0)
#endif

#ifdef __STRICT_ANSI__
#undef inline
#define inline __inline__
#endif

#endif
