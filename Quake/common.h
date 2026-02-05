/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _Q_COMMON_H
#define _Q_COMMON_H

// comndef.h  -- general definitions

#if defined(_WIN32)
#ifdef _MSC_VER
#  pragma warning(disable:4244)
	/* 'argument'	: conversion from 'type1' to 'type2',
			  possible loss of data */
#  pragma warning(disable:4305)
	/* 'identifier'	: truncation from 'type1' to 'type2' */
	/*  in our case, truncation from 'double' to 'float' */
#  pragma warning(disable:4267)
	/* 'var'	: conversion from 'size_t' to 'type',
			  possible loss of data (/Wp64 warning) */
#  pragma warning(error:4431)
	/* missing type specifier - int assumed */
#endif	/* _MSC_VER */
#endif	/* _WIN32 */

#undef	min
#undef	max

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))
#define GENERIC_TYPES(x, separator) \
	x(int, i) separator \
	x(unsigned int, u) separator \
	x(long, l) separator \
	x(unsigned long, ul) separator \
	x(long long, ll) separator \
	x(unsigned long long, ull) separator \
	x(float, f) separator \
	x(double, d)

#define COMMA ,
#define NO_COMMA

#define IMPL_GENERIC_FUNCS(type, suffix) \
static inline type q_min_##suffix (type a, type b) { \
	return (a < b) ? a : b; \
} \
static inline type q_max_##suffix (type a, type b) { \
	return (a > b) ? a : b; \
} \
static inline type clamp_##suffix (type minval, type val, type maxval) { \
	return (val < minval) ? minval : ((val > maxval) ? maxval : val); \
}

GENERIC_TYPES (IMPL_GENERIC_FUNCS, NO_COMMA)

#define SELECT_Q_MIN(type, suffix) type: q_min_##suffix
#define q_min(a, b) _Generic((a) + (b), GENERIC_TYPES (SELECT_Q_MIN, COMMA))(a, b)

#define SELECT_Q_MAX(type, suffix) type: q_max_##suffix
#define q_max(a, b) _Generic((a) + (b), GENERIC_TYPES (SELECT_Q_MAX, COMMA))(a, b)

#define SELECT_CLAMP(type, suffix) type: clamp_##suffix
#define CLAMP(minval, val, maxval) _Generic((minval) + (val) + (maxval), \
	GENERIC_TYPES (SELECT_CLAMP, COMMA))(minval, val, maxval)

#elif defined(__GNUC__)
#define q_max(a,b) ({                   \
	const __typeof((a) + (b)) a_ = (a); \
	const __typeof((a) + (b)) b_ = (b); \
	(a_ > b_) ? a_ : b_;                \
})
#define q_min(a,b) ({                   \
	const __typeof((a) + (b)) a_ = (a); \
	const __typeof((a) + (b)) b_ = (b); \
	(a_ < b_) ? a_ : b_;                \
})
#define CLAMP(_minval, x, _maxval) ({                                \
	const __typeof((_minval) + (x) + (_maxval)) x_      = (x);       \
	const __typeof((_minval) + (x) + (_maxval)) valmin_ = (_minval); \
	const __typeof((_minval) + (x) + (_maxval)) valmax_ = (_maxval); \
	(x_ < valmin_) ? valmin_ :                                       \
	(x_ > valmax_) ? valmax_ : x_;                                   \
})

#else
#define	q_min(a, b)	(((a) < (b)) ? (a) : (b))
#define	q_max(a, b)	(((a) > (b)) ? (a) : (b))
#define	CLAMP(_minval, x, _maxval)		\
	((x) < (_minval) ? (_minval) :		\
	 (x) > (_maxval) ? (_maxval) : (x))
#endif

#define LERP(a, b, t) ((a) + ((b)-(a))*(t))

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte		*data;
	int32_t		maxsize;
	int32_t		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize);
void SZ_Free (sizebuf_t *buf);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);
void SZ_Print (sizebuf_t *buf, const char *data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - offsetof(t,m)))

//============================================================================

#define PTR_IN_RANGE(ptr, begin, end)			(((uintptr_t)(ptr)-(uintptr_t)(begin)) < ((uintptr_t)(end)-(uintptr_t)(begin)))

//============================================================================

typedef struct vec_header_t {
	size_t capacity;
	size_t size;
} vec_header_t;

#define VEC_HEADER(v)			(((vec_header_t*)(v))[-1])

#define VEC_PUSH(v,n)			do { Vec_Grow((void**)&(v), sizeof((v)[0]), 1); (v)[VEC_HEADER(v).size++] = (n); } while (0)
#define VEC_POP(v)				do { SDL_assert(v && VEC_HEADER(v).size >= 1); VEC_HEADER(v).size--; } while (0)
#define VEC_POP_N(v,n)			do { SDL_assert(v && VEC_HEADER(v).size >= (n)); VEC_HEADER(v).size -= (n); } while (0)
#define VEC_SIZE(v)				((v) ? VEC_HEADER(v).size : 0)
#define VEC_LAST(v)				((v)[VEC_SIZE(v)-1])
#define VEC_FREE(v)				Vec_Free((void**)&(v))
#define VEC_CLEAR(v)			Vec_Clear((void**)&(v))

void Vec_Grow (void **pvec, size_t element_size, size_t count);
void Vec_Append (void **pvec, size_t element_size, const void *data, size_t count);
void Vec_Clear (void **pvec);
void Vec_Free (void **pvec);

void MultiString_Append (char **pvec, const char *str);
void MultiString_AppendN (char **pvec, const char *str, size_t len);

//============================================================================

#define BITARRAY_DWORDS(bits)		(((bits)+31)/32)
#define BITARRAY_MEM_SIZE(bits)		(BITARRAY_DWORDS (bits) * sizeof (uint32_t))

static inline qboolean GetBit (const uint32_t *arr, uint32_t i)
{
	return (arr[i/32u] & (1u<<(i%32u))) != 0u;
}

static inline void SetBit (uint32_t *arr, uint32_t i)
{
	arr[i/32u] |= 1u<<(i%32u);
}

static inline void ClearBit (uint32_t *arr, uint32_t i)
{
	arr[i/32u] &= ~(1u<<(i%32u));
}

static inline void ToggleBit (uint32_t *arr, uint32_t i)
{
	arr[i/32u] ^= 1u<<(i%32u);
}

//============================================================================

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#define host_bigendian 1
#else
#define host_bigendian 0
#endif

#define BigShort(s)    ((int16_t)SDL_SwapBE16((s)))
#define LittleShort(s) ((int16_t)SDL_SwapLE16((s)))
#define BigLong(l)     ((int32_t)SDL_SwapBE32((l)))
#define LittleLong(l)  ((int32_t)SDL_SwapLE32((l)))
#define BigFloat(f)    SDL_SwapFloatBE((f))
#define LittleFloat(f) SDL_SwapFloatLE((f))

//============================================================================

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f, unsigned int flags);
void MSG_WriteAngle (sizebuf_t *sb, float f, unsigned int flags);
void MSG_WriteAngle16 (sizebuf_t *sb, float f, unsigned int flags); //johnfitz

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading (void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
const char *MSG_ReadString (void);

float MSG_ReadCoord (unsigned int flags);
float MSG_ReadAngle (unsigned int flags);
float MSG_ReadAngle16 (unsigned int flags); //johnfitz

//============================================================================

void Q_memset (void *dest, int fill, size_t count);
void Q_memcpy (void *dest, const void *src, size_t count);
int Q_memcmp (const void *m1, const void *m2, size_t count);
void Q_strcpy (char *dest, const char *src);
void Q_strncpy (char *dest, const char *src, int count);
int Q_strlen (const char *str);
char *Q_strrchr (const char *s, char c);
void Q_strcat (char *dest, const char *src);
int Q_strcmp (const char *s1, const char *s2);
int Q_strncmp (const char *s1, const char *s2, int count);
int	Q_atoi (const char *str);
float Q_atof (const char *str);


#include "strl_fn.h"

/* locale-insensitive strcasecmp replacement functions: */
extern int q_strcasecmp (const char * s1, const char * s2);
extern int q_strncasecmp (const char *s1, const char *s2, size_t n);

/* locale-insensitive natural string comparison function */
int q_strnaturalcmp (const char *s1, const char *s2);

/* locale-insensitive case-insensitive alternative to strstr */
extern char *q_strcasestr(const char *haystack, const char *needle);

/* locale-insensitive strlwr/upr replacement functions: */
extern char *q_strlwr (char *str);
extern char *q_strupr (char *str);

/* snprintf, vsnprintf : always use our versions. */
extern int q_snprintf (char *str, size_t size, const char *format, ...) FUNC_PRINTF(3,4);
extern int q_vsnprintf(char *str, size_t size, const char *format, va_list args) FUNC_PRINTF(3,0);

/* q_strascii : check if a string contains only ascii */
extern qboolean q_strascii (const char* str);

#define PLURAL(count)	((int)(count)), ((int)(count) == 1 ? "" : "s")

//============================================================================

extern	THREAD_LOCAL char		com_token[1024];
extern	qboolean	com_eof;

typedef enum
{
	CPE_NOTRUNC,					// return parse error in case of overflow
	CPE_ALLOWTRUNC,					// truncate com_token in case of overflow
} cpe_mode;

const char *COM_Parse (const char *data);
const char *COM_ParseEx (const char *data, cpe_mode mode);

typedef struct
{
	const char	*data;
	size_t		len;
} stringview_t;

qboolean COM_ParseLine (const char **str, stringview_t *line);

extern	int			com_argc;
extern	const char	**com_argv;

extern	int		safemode;
/* safe mode: in true, the engine will behave as if one
   of these arguments were actually on the command line:
   -nosound, -nocdaudio, -nomidi, -stdvid, -dibonly,
   -nomouse, -nojoy, -nolan
 */

int COM_CheckParm (const char *parm);

void COM_Init (void);
void COM_InitArgv (int argc, char **argv);
void COM_InitFilesystem (void);

void COM_ResetGameDirectories (const char *newgamedirs);
void COM_AddGameDirectory (const char *dir);
void COM_SwitchGame (const char *paths);

const char *COM_SkipPath (const char *pathname);
const char *COM_SkipSpace (const char *str);
void COM_StripExtension (const char *in, char *out, size_t outsize);
void COM_FileBase (const char *in, char *out, size_t outsize);
void COM_AddExtension (char *path, const char *extension, size_t len);
#if 0 /* COM_DefaultExtension can be dangerous */
void COM_DefaultExtension (char *path, const char *extension, size_t len);
#endif
const char *COM_FileGetExtension (const char *in); /* doesn't return NULL */
void COM_ExtractExtension (const char *in, char *out, size_t outsize);
char *COM_TempSuffix (unsigned seq);
void COM_CreatePath (char *path);

// Describes the given duration, e.g. "3 minutes"
void COM_DescribeDuration (char *out, size_t outsize, double seconds);

char *va (const char *format, ...) FUNC_PRINTF(1,2);
// does a varargs printf into a temp buffer

char *COM_TintSubstring (const char *in, const char *substr, char *out, size_t outsize);
char *COM_TintString (const char *in, char *out, size_t outsize);

unsigned COM_HashString (const char *str);
unsigned COM_HashBlock (const void *data, size_t size);

// localization support for 2021 rerelease version:
void LOC_Init (void);
void LOC_Shutdown (void);
const char* LOC_GetRawString (const char *key);
const char* LOC_GetString (const char *key);
qboolean LOC_HasPlaceholders (const char *str);
size_t LOC_Format (const char *format, const char* (*getarg_fn)(int idx, void* userdata), void* userdata, char* out, size_t len);

// Unicode
size_t UTF8_WriteCodePoint (char *dst, size_t maxbytes, uint32_t codepoint);
uint32_t UTF8_ReadCodePoint (const char **src);
size_t UTF8_FromQuake (char *dst, size_t maxbytes, const char *src);
size_t UTF8_ToQuake (char *dst, size_t maxbytes, const char *src);

size_t UTF8_FromIBM437 (char* dst, size_t maxbytes, const char* src);

#define UNICODE_UNKNOWN		0xFFFD
#define UNICODE_MAX			0x10FFFF

#define QCHAR_BOX			11
#define QCHAR_COLOR_MASK	((char)0x80)
#define QCHAR_COLORED(x)	((char)((x) | QCHAR_COLOR_MASK))

//============================================================================

// QUAKEFS
typedef struct searchpath_s
{
	unsigned int path_id;	// identifier assigned to the game directory
					// Note that <install_dir>/game1 and
					// <userdir>/game1 have the same id.
	char	filename[MAX_OSPATH];
	int 	pack;			// only one of filename / pack will be used
	struct searchpath_s	*next;
} searchpath_t;

extern searchpath_t *com_searchpaths;
extern searchpath_t *com_base_searchpaths;

struct cache_user_s;

#define MAX_BASEDIRS 64

extern	int		com_numbasedirs;
extern	char	com_basedirs[MAX_BASEDIRS][MAX_OSPATH];
extern	char	com_gamedir[MAX_OSPATH];
extern	char	com_nightdivedir[MAX_OSPATH];

void COM_WriteFile (const char *filename, const void *data, int len);
qboolean COM_WriteFile_OSPath (const char *filename, const void *data, size_t len);

// Opens the given path directly, ignoring search paths.
// Returns NULL on failure, or else a '\0'-terminated malloc'ed buffer.
// Loads in "t" mode so CRLF to LF translation is performed on Windows.
byte *COM_LoadMallocFile_TextMode_OSPath (const char *path, long *len_out);

// Replaces CR/CRLF with LF.
char *COM_NormalizeLineEndings (char *buffer);

// Attempts to parse an int, followed by a newline.
// Returns advanced buffer position.
// Doesn't signal parsing failure, but this is not needed for savegame loading.
const char *COM_ParseIntNewline(const char *buffer, int *value);

// Attempts to parse a float followed by a newline.
// Returns advanced buffer position.
const char *COM_ParseFloatNewline(const char *buffer, float *value);

// Parse a string of non-whitespace into com_token, then tries to consume a
// newline. Returns advanced buffer position.
const char *COM_ParseStringNewline(const char *buffer);


#define	FS_ENT_NONE		(0)
#define	FS_ENT_FILE		(1 << 0)
#define	FS_ENT_DIRECTORY	(1 << 1)

extern struct cvar_s	registered;
extern qboolean		standard_quake, rogue, hipnotic;
extern qboolean		fitzmode;
	/* if true, run in fitzquake mode disabling custom quakespasm hacks */

#endif	/* _Q_COMMON_H */

