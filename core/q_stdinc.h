/* q_stdinc.h - includes the minimum necessary stdc headers,
 *		defines common and / or missing types.
 *
 * NOTE:	for net stuff use net_sys.h,
 *		for byte order use q_endian.h,
 *		for math stuff use mathlib.h,
 *		for locale-insensitive ctype.h functions use q_ctype.h.
 *
 * Copyright (C) 1996-1997  Id Software, Inc.
 * Copyright (C) 2007-2011  O.Sezer <sezero@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef QSTDINC_H
#define QSTDINC_H

#if defined(_WIN32) && !defined(MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS)
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0
#endif

#include <sys/types.h>
#include <stddef.h>
#include <limits.h>
#ifndef _WIN32 /* others we support without sys/param.h? */
#include <sys/param.h>
#endif

#include <stdio.h>

/* NOTES on TYPE SIZES:
   Quake/Hexen II engine relied on 32 bit int type size
   with ILP32 (not LP32) model in mind.  We now support
   LP64 and LLP64, too. We expect:
   sizeof (char)	== 1
   sizeof (short)	== 2
   sizeof (int)		== 4
   sizeof (float)	== 4
   sizeof (long)	== 4 / 8
   sizeof (pointer *)	== 4 / 8
   For this, we need stdint.h (or inttypes.h)
   FIXME: On some platforms, only inttypes.h is available.
   FIXME: Properly replace certain short and int usage
	  with int16_t and int32_t.
 */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
/* MS Visual Studio provides stdint.h only starting with
 * version 2010.  Even in VS2010, there is no inttypes.h.. */
#include "msinttypes/stdint.h"
#else
#include <stdint.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <SDL_endian.h>


/*==========================================================================*/

#ifndef NULL
#if defined(__cplusplus)
#define	NULL		0
#else
#define	NULL		((void *)0)
#endif
#endif

#define	Q_MAXCHAR	((char)0x7f)
#define	Q_MAXSHORT	((short)0x7fff)
#define	Q_MAXINT	((int)0x7fffffff)
#define	Q_MAXLONG	((int)0x7fffffff)

#define	Q_MINCHAR	((char)0x80)
#define	Q_MINSHORT	((short)0x8000)
#define	Q_MININT	((int)0x80000000)
#define	Q_MINLONG	((int)0x80000000)

#ifndef COMPILE_TIME_ASSERT
#if defined(__cplusplus)
/* Keep C++ case alone: Some versions of gcc will define __STDC_VERSION__ even when compiling in C++ mode. */
#if (__cplusplus >= 201103L)
#define COMPILE_TIME_ASSERT(name, x)  static_assert(x, #x)
#endif
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
#define COMPILE_TIME_ASSERT(name, x)  static_assert(x, #x)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define COMPILE_TIME_ASSERT(name, x) _Static_assert(x, #x)
#endif
#endif /**/
#ifndef COMPILE_TIME_ASSERT
/* universal, but may trigger -Wunused-local-typedefs */
#define COMPILE_TIME_ASSERT(name, x) \
	typedef int dummy_ ## name[(x) * 2 - 1]
#endif

COMPILE_TIME_ASSERT(char, sizeof(char) == 1);
COMPILE_TIME_ASSERT(float, sizeof(float) == 4);
COMPILE_TIME_ASSERT(long, sizeof(long) >= 4);
COMPILE_TIME_ASSERT(int, sizeof(int) == 4);
COMPILE_TIME_ASSERT(short, sizeof(short) == 2);

/* make sure enums are the size of ints for structure packing */
typedef enum {
	THE_DUMMY_VALUE
} THE_DUMMY_ENUM;
COMPILE_TIME_ASSERT(enum, sizeof(THE_DUMMY_ENUM) == sizeof(int));


/* for array size: */
#define Q_COUNTOF(x) (sizeof(x) / sizeof((x)[0]))

/* Provide a substitute for offsetof() if we don't have one.
 * This variant works on most (but not *all*) systems...
 */
#ifndef offsetof
#define offsetof(t,m) ((intptr_t)&(((t *)0)->m))
#endif


/*==========================================================================*/

typedef unsigned char		byte;

/* some structures have qboolean members and the x86 asm code expect
 * those members to be 4 bytes long.  i.e.: qboolean must be 32 bits.  */
typedef int	qboolean;
#undef true
#undef false
#if !defined(__cplusplus)
#if defined __STDC_VERSION__ && (__STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#else
enum {
	false = 0,
	true  = 1
};
#endif
#endif /* */
COMPILE_TIME_ASSERT(falsehood, ((1 != 1) == false));
COMPILE_TIME_ASSERT(truth, ((1 == 1) == true));
COMPILE_TIME_ASSERT(qboolean, sizeof(qboolean) == 4);

/*==========================================================================*/

/* math */
typedef float	vec_t;
typedef vec_t	vec3_t[3];
typedef vec_t	vec4_t[4];
typedef vec_t	vec5_t[5];
typedef int	fixed4_t;
typedef int	fixed8_t;
typedef int	fixed16_t;

/* natvis helpers */
typedef struct { float data[2]; } float2_t;
typedef struct { float data[3]; } float3_t;
typedef struct { float data[4]; } float4_t;

/*==========================================================================*/

/* MAX_OSPATH (max length of a filesystem pathname, i.e. PATH_MAX)
 * Note: See GNU Hurd and others' notes about brokenness of this:
 * http://www.gnu.org/software/hurd/community/gsoc/project_ideas/maxpath.html
 * http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html */

#if !defined(PATH_MAX)
/* equivalent values? */
#if defined(MAXPATHLEN)
#define PATH_MAX	MAXPATHLEN
#elif defined(_WIN32) && defined(_MAX_PATH)
#define PATH_MAX	(_MAX_PATH * 3)
#elif defined(_WIN32) && defined(MAX_PATH)
#define PATH_MAX	(MAX_PATH * 3)
#else /* fallback */
#define PATH_MAX	1024
#endif
#endif	/* PATH_MAX */

#define MAX_OSPATH	PATH_MAX

/*==========================================================================*/

/* missing types: */
#if defined(_MSC_VER)
typedef ptrdiff_t	ssize_t;
#endif

/*==========================================================================*/

/* function attributes, etc */

#if defined(__GNUC__)
#define FUNC_PRINTF(x,y)	__attribute__((__format__(__printf__,x,y)))
#else
#define FUNC_PRINTF(x,y)
#endif

/* argument format attributes for function pointers are supported for gcc >= 3.1 */
#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0))
#define FUNCP_PRINTF	FUNC_PRINTF
#else
#define FUNCP_PRINTF(x,y)
#endif

/* llvm's optnone function attribute started with clang-3.5.0 */
#if defined(__clang__) && \
           (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 5))
#define FUNC_NO_OPTIMIZE	__attribute__((__optnone__))
/* function optimize attribute is added starting with gcc 4.4.0 */
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 3))
#define FUNC_NO_OPTIMIZE	__attribute__((__optimize__("0")))
#else
#define FUNC_NO_OPTIMIZE
#endif

#if defined(__GNUC__)
#define FUNC_NORETURN	__attribute__((__noreturn__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1200)
#define FUNC_NORETURN		__declspec(noreturn)
#elif defined(__WATCOMC__)
#define FUNC_NORETURN /* use the 'aborts' aux pragma */
#else
#define FUNC_NORETURN
#endif

#if defined(__GNUC__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define FUNC_NOINLINE	__attribute__((__noinline__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#define FUNC_NOINLINE		__declspec(noinline)
#else
#define FUNC_NOINLINE
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define FUNC_NOCLONE	__attribute__((__noclone__))
#else
#define FUNC_NOCLONE
#endif

#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif	/* _MSC_VER */

#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))
#define THREAD_LOCAL _Thread_local
#elif defined(__GNUC__)
#define THREAD_LOCAL __thread
#else
#error TLS not supported
#endif

/*==========================================================================*/

#endif	/* QSTDINC_H */
