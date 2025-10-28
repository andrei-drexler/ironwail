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

// common.c -- misc functions used in client and server

#include "quakedef.h"
#include "q_ctype.h"
#include "bgmusic.h"
#include "steam.h"
#include <time.h>
#include <errno.h>
#include "miniz.h"
#include "unicode_translit.h"

static const char	*largv[MAX_NUM_ARGVS + 1];
static char	argvdummy[] = " ";

int		safemode;

cvar_t	registered = {"registered","1",CVAR_ROM}; /* set to correct value in COM_CheckRegistered() */
cvar_t	cmdline = {"cmdline","",CVAR_ROM/*|CVAR_SERVERINFO*/}; /* sending cmdline upon CCREQ_RULE_INFO is evil */
cvar_t	language = {"language","auto",CVAR_ARCHIVE}; /* for 2021 rerelease text */

static qboolean		com_modified;	// set true if using non-id files

qboolean		fitzmode;

static void COM_Path_f (void);

// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT		339	/* id1/pak0.pak - v1.0x */
#define PAK0_CRC_V100		13900	/* id1/pak0.pak - v1.00 */
#define PAK0_CRC_V101		62751	/* id1/pak0.pak - v1.01 */
#define PAK0_CRC_V106		32981	/* id1/pak0.pak - v1.06 */
#define PAK0_CRC	(PAK0_CRC_V106)
#define PAK0_COUNT_V091		308	/* id1/pak0.pak - v0.91/0.92, not supported */
#define PAK0_CRC_V091		28804	/* id1/pak0.pak - v0.91/0.92, not supported */

THREAD_LOCAL char	com_token[1024];
int			com_argc;
const char	**com_argv;

#define CMDLINE_LENGTH	256		/* johnfitz -- mirrored in cmd.c */
char	com_cmdline[CMDLINE_LENGTH];

qboolean standard_quake = true, rogue, hipnotic, quake64;

// this graphic needs to be in the pak file to use registered features
static unsigned short pop[] =
{
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000,
	0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000,
	0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600,
	0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563,
	0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564,
	0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564,
	0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563,
	0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500,
	0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200,
	0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000,
	0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*

All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.
This can be overridden with the "-basedir" command line parm to allow code
debugging in a different directory.  The base directory is only used during
filesystem initialization.

The "game directory" is the first tree on the search path and directory that all
generated files (savegames, screenshots, demos, config files) will be saved to.
This can be overridden with the "-game" command line parameter.  The game
directory can never be changed while quake is executing.  This is a precacution
against having a malicious server instruct clients to write files over areas they
shouldn't.

The "cache directory" is only used during development to save network bandwidth,
especially over ISDN / T1 lines.  If there is a cache directory specified, when
a file is found by the normal search path, it will be mirrored into the cache
directory, then opened there.

FIXME:
The file "parms.txt" will be read out of the game directory and appended to the
current command line arguments to allow different games to initialize startup
parms differently.  This could be used to add a "-sspeed 22050" for the high
quality sound edition.  Because they are added at the end, they will not
override an explicit setting on the original command line.

*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

							DYNAMIC VECTORS

============================================================================
*/

void Vec_Grow (void **pvec, size_t element_size, size_t count)
{
	vec_header_t header;
	if (*pvec)
		header = VEC_HEADER(*pvec);
	else
		header.size = header.capacity = 0;

	if (header.size + count > header.capacity)
	{
		void *new_buffer;
		size_t total_size;

		header.capacity = header.size + count;
		header.capacity += header.capacity >> 1;
		if (header.capacity < 16)
			header.capacity = 16;
		total_size = sizeof(vec_header_t) + header.capacity * element_size;

		if (*pvec)
			new_buffer = realloc (((vec_header_t*)*pvec) - 1, total_size);
		else
			new_buffer = malloc (total_size);
		if (!new_buffer)
			Sys_Error ("Vec_Grow: failed to allocate %" SDL_PRIu64 " bytes\n", (uint64_t) total_size);

		*pvec = 1 + (vec_header_t*)new_buffer;
		VEC_HEADER(*pvec) = header;
	}
}

void Vec_Append (void **pvec, size_t element_size, const void *data, size_t count)
{
	if (!count)
		return;
	Vec_Grow (pvec, element_size, count);
	memcpy ((byte *)*pvec + VEC_HEADER(*pvec).size * element_size, data, count * element_size);
	VEC_HEADER(*pvec).size += count;
}

void Vec_Clear (void **pvec)
{
	if (*pvec)
		VEC_HEADER(*pvec).size = 0;
}

void Vec_Free (void **pvec)
{
	if (*pvec)
	{
		free(&VEC_HEADER(*pvec));
		*pvec = NULL;
	}
}

void MultiString_Append (char **pvec, const char *str)
{
	Vec_Append ((void **)pvec, 1, str, strlen (str) + 1);
}

void MultiString_AppendN (char **pvec, const char *str, size_t len)
{
	Vec_Append ((void **)pvec, 1, str, len);
	VEC_PUSH (*pvec, '\0');
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int q_strnaturalcmp (const char *s1, const char *s2)
{
	qboolean neg1, neg2, sign1, sign2;

	if (s1 == s2)
		return 0;

	neg1 = *s1 == '-';
	neg2 = *s2 == '-';
	sign1 = neg1 || *s1 == '+';
	sign2 = neg2 || *s2 == '+';

	// early out if strings start with different signs followed by digits
	if (neg1 != neg2 && q_isdigit (s1[sign1]) && q_isdigit (s1[sign2]))
		return neg2 - neg1;

skip_prefix:
	while (*s1 && !q_isdigit (*s1) && q_toupper (*s1) == q_toupper (*s2))
	{
		s1++;
		s2++;
		continue;
	}

	if (q_isdigit (*s1) && q_isdigit (*s2))
	{
		const char *begin1 = s1++;
		const char *begin2 = s2++;
		int diff, sign;

		while (*begin1 == '0')
			begin1++;
		while (*begin2 == '0')
			begin2++;

		while (q_isdigit (*s1))
			s1++;
		while (q_isdigit (*s2))
			s2++;

		sign = neg1 ? -1 : 1;

		diff = (s1 - begin1) - (s2 - begin2);
		if (diff)
			return diff * sign;

		while (begin1 != s1)
		{
			diff = *begin1++ - *begin2++;
			if (diff)
				return diff * sign;
		}

		// We only support negative numbers at the beginning of strings so that
		// "-2" is sorted before "-1", but "file-2345.ext" *after* "file-1234.ext".
		neg1 = neg2 = false;

		goto skip_prefix;
	}

	return q_toupper (*s1) - q_toupper (*s2);
}

int q_strcasecmp(const char * s1, const char * s2)
{
	const char * p1 = s1;
	const char * p2 = s2;
	char c1, c2;

	if (p1 == p2)
		return 0;

	do
	{
		c1 = q_tolower (*p1++);
		c2 = q_tolower (*p2++);
		if (c1 == '\0')
			break;
	} while (c1 == c2);

	return (int)(c1 - c2);
}

int q_strncasecmp(const char *s1, const char *s2, size_t n)
{
	const char * p1 = s1;
	const char * p2 = s2;
	char c1, c2;

	if (p1 == p2 || n == 0)
		return 0;

	do
	{
		c1 = q_tolower (*p1++);
		c2 = q_tolower (*p2++);
		if (c1 == '\0' || c1 != c2)
			break;
	} while (--n > 0);

	return (int)(c1 - c2);
}

char *q_strcasestr(const char *haystack, const char *needle)
{
	const size_t len = strlen(needle);

	if (!len)
		return (char *)haystack;

	while (*haystack)
	{
		if (!q_strncasecmp(haystack, needle, len))
			return (char *)haystack;

		++haystack;
	}

	return NULL;
}

char *q_strlwr (char *str)
{
	char	*c;
	c = str;
	while (*c)
	{
		*c = q_tolower(*c);
		c++;
	}
	return str;
}

char *q_strupr (char *str)
{
	char	*c;
	c = str;
	while (*c)
	{
		*c = q_toupper(*c);
		c++;
	}
	return str;
}

/* platform dependant (v)snprintf function names: */
#if defined(_WIN32)
#define	snprintf_func		_snprintf
#define	vsnprintf_func		_vsnprintf
#else
#define	snprintf_func		snprintf
#define	vsnprintf_func		vsnprintf
#endif

int q_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int		ret;

	ret = vsnprintf_func (str, size, format, args);

	if (ret < 0)
		ret = (int)size;
	if (size == 0)	/* no buffer */
		return ret;
	if ((size_t)ret >= size)
		str[size - 1] = '\0';

	return ret;
}

int q_snprintf (char *str, size_t size, const char *format, ...)
{
	int		ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = q_vsnprintf (str, size, format, argptr);
	va_end (argptr);

	return ret;
}

void Q_memset (void *dest, int fill, size_t count)
{
	size_t		i;

	if ( (((uintptr_t)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i = 0; i < count; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i = 0; i < count; i++)
			((byte *)dest)[i] = fill;
}

void Q_memcpy (void *dest, const void *src, size_t count)
{
	size_t		i;

	if (( ( (uintptr_t)dest | (uintptr_t)src | count) & 3) == 0)
	{
		count >>= 2;
		for (i = 0; i < count; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i = 0; i < count; i++)
			((byte *)dest)[i] = ((byte *)src)[i];
}

int Q_memcmp (const void *m1, const void *m2, size_t count)
{
	while(count)
	{
		count--;
		if (((byte *)m1)[count] != ((byte *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy (char *dest, const char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, const char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen (const char *str)
{
	int		count;

	count = 0;
	while (str[count])
		count++;

	return count;
}

char *Q_strrchr(const char *s, char c)
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
	{
		if (*--s == c)
			return (char *)s;
	}
	return NULL;
}

void Q_strcat (char *dest, const char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp (const char *s1, const char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncmp (const char *s1, const char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_atoi (const char *str)
{
	int		val;
	int		sign;
	int		c;

	while (q_isspace (*str))
		++str;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}

	return 0;
}


float Q_atof (const char *str)
{
	double		val;
	int		sign;
	int		c;
	int	decimal, total;

	while (q_isspace (*str))
		++str;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte	*buf;

	buf = (byte *) SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;

	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, Q_strlen(s)+1);
}

//johnfitz -- original behavior, 13.3 fixed point coords, max range +-4096
void MSG_WriteCoord16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, Q_rint(f*8));
}

//johnfitz -- 16.8 fixed point coords, max range +-32768
void MSG_WriteCoord24 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, f);
	MSG_WriteByte (sb, (int)(f*255)%255);
}

//johnfitz -- 32-bit float coords
void MSG_WriteCoord32f (sizebuf_t *sb, float f)
{
	MSG_WriteFloat (sb, f);
}

void MSG_WriteCoord (sizebuf_t *sb, float f, unsigned int flags)
{
	if (flags & PRFL_FLOATCOORD)
		MSG_WriteFloat (sb, f);
	else if (flags & PRFL_INT32COORD)
		MSG_WriteLong (sb, Q_rint (f * 16));
	else if (flags & PRFL_24BITCOORD)
		MSG_WriteCoord24 (sb, f);
	else MSG_WriteCoord16 (sb, f);
}

void MSG_WriteAngle (sizebuf_t *sb, float f, unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		MSG_WriteFloat (sb, f);
	else if (flags & PRFL_SHORTANGLE)
		MSG_WriteShort (sb, Q_rint(f * 65536.0 / 360.0) & 65535);
	else MSG_WriteByte (sb, Q_rint(f * 256.0 / 360.0) & 255); //johnfitz -- use Q_rint instead of (int)	}
}

//johnfitz -- for PROTOCOL_FITZQUAKE
void MSG_WriteAngle16 (sizebuf_t *sb, float f, unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		MSG_WriteFloat (sb, f);
	else MSG_WriteShort (sb, Q_rint(f * 65536.0 / 360.0) & 65535);
}
//johnfitz

//
// reading functions
//
int		msg_readcount;
qboolean	msg_badread;

void MSG_BeginReading (void)
{
	msg_readcount = 0;
	msg_badread = false;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar (void)
{
	int	c;

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadByte (void)
{
	int	c;

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadShort (void)
{
	int	c;

	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (short)(net_message.data[msg_readcount]
			+ (net_message.data[msg_readcount+1]<<8));

	msg_readcount += 2;

	return c;
}

int MSG_ReadLong (void)
{
	int	c;

	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = net_message.data[msg_readcount]
			+ (net_message.data[msg_readcount+1]<<8)
			+ (net_message.data[msg_readcount+2]<<16)
			+ (net_message.data[msg_readcount+3]<<24);

	msg_readcount += 4;

	return c;
}

float MSG_ReadFloat (void)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	dat.b[0] = net_message.data[msg_readcount];
	dat.b[1] = net_message.data[msg_readcount+1];
	dat.b[2] = net_message.data[msg_readcount+2];
	dat.b[3] = net_message.data[msg_readcount+3];
	msg_readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

const char *MSG_ReadString (void)
{
	static char	string[2048];
	int		c;
	size_t		l;

	l = 0;
	do
	{
		c = MSG_ReadByte ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);

	string[l] = 0;

	return string;
}

//johnfitz -- original behavior, 13.3 fixed point coords, max range +-4096
float MSG_ReadCoord16 (void)
{
	return MSG_ReadShort() * (1.0/8);
}

//johnfitz -- 16.8 fixed point coords, max range +-32768
float MSG_ReadCoord24 (void)
{
	return MSG_ReadShort() + MSG_ReadByte() * (1.0/255);
}

//johnfitz -- 32-bit float coords
float MSG_ReadCoord32f (void)
{
	return MSG_ReadFloat();
}

float MSG_ReadCoord (unsigned int flags)
{
	if (flags & PRFL_FLOATCOORD)
		return MSG_ReadFloat ();
	else if (flags & PRFL_INT32COORD)
		return MSG_ReadLong () * (1.0 / 16.0);
	else if (flags & PRFL_24BITCOORD)
		return MSG_ReadCoord24 ();
	else return MSG_ReadCoord16 ();
}

float MSG_ReadAngle (unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		return MSG_ReadFloat ();
	else if (flags & PRFL_SHORTANGLE)
		return MSG_ReadShort () * (360.0 / 65536);
	else return MSG_ReadChar () * (360.0 / 256);
}

//johnfitz -- for PROTOCOL_FITZQUAKE
float MSG_ReadAngle16 (unsigned int flags)
{
	if (flags & PRFL_FLOATANGLE)
		return MSG_ReadFloat ();	// make sure
	else return MSG_ReadShort () * (360.0 / 65536);
}
//johnfitz

//===========================================================================

void SZ_Alloc (sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;
	buf->data = (byte *) Hunk_AllocName (startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}


void SZ_Free (sizebuf_t *buf)
{
//	Z_Free (buf->data);
//	buf->data = NULL;
//	buf->maxsize = 0;
	buf->cursize = 0;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void	*data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Host_Error ("SZ_GetSpace: overflow without allowoverflow set"); // ericw -- made Host_Error to be less annoying

		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);

		buf->overflowed = true;
		Con_Printf ("SZ_GetSpace: overflow\n");
		SZ_Clear (buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int		len = Q_strlen(data) + 1;

	if (buf->data[buf->cursize-1])
	{	/* no trailing 0 */
		Q_memcpy ((byte *)SZ_GetSpace(buf, len  )  , data, len);
	}
	else
	{	/* write over trailing 0 */
		Q_memcpy ((byte *)SZ_GetSpace(buf, len-1)-1, data, len);
	}
}


//============================================================================

/*
============
COM_FirstPathSep

Returns a pointer to the first path separator, if any, or the end of the string
============
*/
const char *COM_FirstPathSep (const char *path)
{
	while (*path && !Sys_IsPathSep (*path))
		path++;
	return path;
}

/*
============
COM_NormalizePath

Replaces all path separators with forward slashes
============
*/
void COM_NormalizePath (char *path)
{
	while (*path)
	{
		if (Sys_IsPathSep (*path))
			*path = '/';
		path++;
	}
}

/*
============
COM_SkipPath
============
*/
const char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname == '/')
			last = pathname + 1;
		pathname++;
	}
	return last;
}


/*
============
COM_SkipSpace
============
*/
const char *COM_SkipSpace (const char *str)
{
	while (q_isspace (*str))
		str++;
	return str;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension (const char *in, char *out, size_t outsize)
{
	int	length;

	if (!*in)
	{
		*out = '\0';
		return;
	}
	if (in != out)	/* copy when not in-place editing */
		q_strlcpy (out, in, outsize);
	length = (int)strlen(out) - 1;
	while (length > 0 && out[length] != '.')
	{
		--length;
		if (out[length] == '/' || out[length] == '\\')
			return;	/* no extension */
	}
	if (length > 0)
		out[length] = '\0';
}

/*
============
COM_FileGetExtension - doesn't return NULL
============
*/
const char *COM_FileGetExtension (const char *in)
{
	const char	*src;
	size_t		len;

	len = strlen(in);
	if (len < 2)	/* nothing meaningful */
		return "";

	src = in + len - 1;
	while (src != in && src[-1] != '.')
		src--;
	if (src == in || strchr(src, '/') != NULL || strchr(src, '\\') != NULL)
		return "";	/* no extension, or parent directory has a dot */

	return src;
}

/*
============
COM_ExtractExtension
============
*/
void COM_ExtractExtension (const char *in, char *out, size_t outsize)
{
	const char *ext = COM_FileGetExtension (in);
	if (! *ext)
		*out = '\0';
	else
		q_strlcpy (out, ext, outsize);
}

/*
============
COM_FileBase
take 'somedir/otherdir/filename.ext',
write only 'filename' to the output
============
*/
void COM_FileBase (const char *in, char *out, size_t outsize)
{
	const char	*dot, *slash, *s;

	s = in;
	slash = in;
	dot = NULL;
	while (*s)
	{
		if (*s == '/')
			slash = s + 1;
		if (*s == '.')
			dot = s;
		s++;
	}
	if (dot == NULL)
		dot = s;

	if (dot - slash < 2)
		q_strlcpy (out, "?model?", outsize);
	else
	{
		size_t	len = dot - slash;
		if (len >= outsize)
			len = outsize - 1;
		memcpy (out, slash, len);
		out[len] = '\0';
	}
}

/*
==================
COM_DefaultExtension
if path doesn't have a .EXT, append extension
(extension should include the leading ".")
==================
*/
#if 0 /* can be dangerous */
void COM_DefaultExtension (char *path, const char *extension, size_t len)
{
	char	*src;

	if (!*path) return;
	src = path + strlen(path) - 1;

	while (*src != '/' && *src != '\\' && src != path)
	{
		if (*src == '.')
			return; // it has an extension
		src--;
	}

	q_strlcat(path, extension, len);
}
#endif

/*
==================
COM_AddExtension
if path extension doesn't match .EXT, append it
(extension should include the leading ".")
==================
*/
void COM_AddExtension (char *path, const char *extension, size_t len)
{
	if (strcmp(COM_FileGetExtension(path), extension + 1) != 0)
		q_strlcat(path, extension, len);
}


/*
================
COM_TintSubstring
================
*/
char *COM_TintSubstring (const char *in, const char *substr, char *out, size_t outsize)
{
	int l;
	char *m = out;
	q_strlcpy(out, in, outsize);
	if (*substr)
	{
		while ((m = q_strcasestr (m, substr)))
		{
			for (l = 0; substr[l]; l++)
				if (m[l] > ' ')
					m[l] |= 0x80;
			m += l;
		}
	}
	return out;
}


/*
================
COM_TintString
================
*/
char *COM_TintString (const char *in, char *out, size_t outsize)
{
	char *ret = out;
	if (!outsize)
		return "";
	--outsize;
	while (*in && outsize > 0)
	{
		char c = *in++;
		if (c > ' ')
			c |= 0x80;
		*out++ = c;
		--outsize;
	}
	*out++ = '\0';
	return ret;
}


/*
==============
COM_ParseEx

Parse a token out of a string

The mode argument controls how overflow is handled:
- CPE_NOTRUNC:		return NULL (abort parsing)
- CPE_ALLOWTRUNC:	truncate com_token (ignore the extra characters in this token)
==============
*/
const char *COM_ParseEx (const char *data, cpe_mode mode)
{
	int		c;
	int		len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;	// end of file
		data++;
	}

// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// skip /*..*/ comments
	if (c == '/' && data[1] == '*')
	{
		data += 2;
		while (*data && !(*data == '*' && data[1] == '/'))
			data++;
		if (*data)
			data += 2;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if ((c = *data) != 0)
				++data;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			if (len < Q_COUNTOF(com_token) - 1)
				com_token[len++] = c;
			else if (mode == CPE_NOTRUNC)
				return NULL;
		}
	}

// parse single characters
	if (c == '{' || c == '}'|| c == '('|| c == ')' || c == '\'' || c == ':')
	{
		if (len < Q_COUNTOF(com_token) - 1)
			com_token[len++] = c;
		else if (mode == CPE_NOTRUNC)
			return NULL;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		if (len < Q_COUNTOF(com_token) - 1)
			com_token[len++] = c;
		else if (mode == CPE_NOTRUNC)
			return NULL;
		data++;
		c = *data;
		/* commented out the check for ':' so that ip:port works */
		if (c == '{' || c == '}'|| c == '('|| c == ')' || c == '\''/* || c == ':' */)
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}


/*
==============
COM_Parse

Parse a token out of a string

Return NULL in case of overflow
==============
*/
const char *COM_Parse (const char *data)
{
	return COM_ParseEx (data, CPE_NOTRUNC);
}


/*
================
COM_ParseLine
================
*/
qboolean COM_ParseLine (const char **str, stringview_t *line)
{
	const char *p;

	if (!str || !*str)
		return false;

	p = *str;
	if (line)
		line->data = p;
	while (*p && *p != '\n')
		p++;
	if (line)
		line->len = p - line->data;

	*str = (*p == '\n') ? p + 1 : NULL;

	return true;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParmNext (int last, const char *parm)
{
	int		i;

	for (i = last+1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp (parm,com_argv[i]))
			return i;
	}

	return 0;
}
int COM_CheckParm (const char *parm)
{
	return COM_CheckParmNext(0, parm);
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
static void COM_CheckRegistered (void)
{
	int		h;
	unsigned short	check[128];
	int		i;

	COM_OpenFile("gfx/pop.lmp", &h, NULL);

	if (h == -1)
	{
		Cvar_SetROM ("registered", "0");
		Con_Printf ("Playing shareware version.\n");
		if (com_modified)
			Sys_Error ("You must have the registered version to use modified games.\n\n"
				   "Basedir is: %s\n\n"
				   "Check that this has an " GAMENAME " subdirectory containing pak0.pak and pak1.pak, "
				   "or use the -basedir command-line option to specify another directory.",
				   com_basedirs[0]);
		return;
	}

	i = Sys_FileRead (h, check, sizeof(check));
	COM_CloseFile (h);
	if (i != (int) sizeof(check))
		goto corrupt;

	for (i = 0; i < 128; i++)
	{
		if (pop[i] != (unsigned short)BigShort (check[i]))
		{ corrupt:
			Sys_Error ("Corrupted data file.");
		}
	}

	for (i = 0; com_cmdline[i]; i++)
	{
		if (com_cmdline[i]!= ' ')
			break;
	}

	Cvar_SetROM ("cmdline", &com_cmdline[i]);
	Cvar_SetROM ("registered", "1");
	Con_Printf ("Playing registered version.\n");
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, char **argv)
{
	int		i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j = 0; (j<MAX_NUM_ARGVS) && (j< argc); j++)
	{
		i = 0;

		while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (CMDLINE_LENGTH - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}

	if (n > 0 && com_cmdline[n-1] == ' ')
		com_cmdline[n-1] = 0; //johnfitz -- kill the trailing space

	Con_Printf("Command line: %s\n", com_cmdline);

	for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc); com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!Q_strcmp ("-safe", argv[com_argc]))
			safemode = 1;
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;

	if (COM_CheckParm ("-rogue"))
	{
		rogue = true;
		standard_quake = false;
	}

	if (COM_CheckParm ("-hipnotic") || COM_CheckParm ("-quoth")) //johnfitz -- "-quoth" support
	{
		hipnotic = true;
		standard_quake = false;
	}
}

/*
================
COM_AddArg

Returns the index of the new argument or -1 on overflow
================
*/
int COM_AddArg (const char *arg)
{
	if (com_argc >= MAX_NUM_ARGVS)
		return -1;
	com_argv[com_argc] = arg;
	com_argv[com_argc + 1] = argvdummy;
	return com_argc++;
}

/*
================
COM_Init
================
*/
void COM_Init (void)
{
	if (COM_CheckParm("-fitz"))
		fitzmode = true;
}


/*
============
va

does a varargs printf into a temp buffer. cycles between
4 different static buffers. the number of buffers cycled
is defined in VA_NUM_BUFFS.
FIXME: make this buffer size safe someday
============
*/
#define	VA_NUM_BUFFS	4
#define	VA_BUFFERLEN	1024

static char *get_va_buffer(void)
{
	static char va_buffers[VA_NUM_BUFFS][VA_BUFFERLEN];
	static int buffer_idx = 0;
	buffer_idx = (buffer_idx + 1) & (VA_NUM_BUFFS - 1);
	return va_buffers[buffer_idx];
}

char *va (const char *format, ...)
{
	va_list		argptr;
	char		*va_buf;

	va_buf = get_va_buffer ();
	va_start (argptr, format);
	q_vsnprintf (va_buf, VA_BUFFERLEN, format, argptr);
	va_end (argptr);

	return va_buf;
}

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

THREAD_LOCAL qfileofs_t com_filesize;


//
// on-disk pakfile
//
typedef struct
{
	char	name[56];
	int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	char	id[4];
	int		dirofs;
	int		dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK	2048

char	com_gamenames[1024];	//eg: "hipnotic;quoth;warp" ... no id1
char	com_gamedir[MAX_OSPATH];
char	com_basedirs[MAX_BASEDIRS][MAX_OSPATH];
int		com_numbasedirs;
char	com_nightdivedir[MAX_OSPATH];
char	com_userprefdir[MAX_OSPATH];
THREAD_LOCAL int	file_from_pak;		// ZOID: global indicating that file came from a pak

searchpath_t	*com_searchpaths;
searchpath_t	*com_base_searchpaths;

/*
============
COM_Path_f
============
*/
static void COM_Path_f (void)
{
	searchpath_t	*s;

	Con_Printf ("Current search path:\n");
	for (s = com_searchpaths; s; s = s->next)
	{
		if (s->pack)
		{
			Con_Printf ("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_Printf ("%s\n", s->filename);
	}
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile (const char *filename, const void *data, int len)
{
	int		handle;
	char	name[MAX_OSPATH];

	q_snprintf (name, sizeof(name), "%s/%s", com_gamedir, filename);

	handle = Sys_FileOpenWrite (name);
	if (handle == -1)
	{
		Sys_Printf ("COM_WriteFile: failed on %s\n", name);
		return;
	}

	Sys_Printf ("COM_WriteFile: %s\n", name);
	Sys_FileWrite (handle, data, len);
	Sys_FileClose (handle);
}

/*
============
COM_WriteFile_OSPath
============
*/
qboolean COM_WriteFile_OSPath (const char *filename, const void *data, size_t len)
{
	qboolean	ret = false;
	FILE		*f = Sys_fopen (filename, "wb");

	if (f)
	{
		ret = fwrite (data, len, 1, f) == 1;
		fclose (f);
		if (!ret)
			Sys_remove (filename);
	}

	return ret;
}

/*
============
COM_TempSuffix
============
*/
char *COM_TempSuffix (unsigned seq)
{
	static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789=#";
	static char buf[64];
	time_t		now = 0;
	int			r, len = 0, maxlen = sizeof (buf) - 1;

	time (&now);
	while (now && len < maxlen)
	{
		buf[len++] = base64[now & 63];
		now >>= 6;
	}

	r = rand ();
	while (r && len < maxlen)
	{
		buf[len++] = base64[r & 63];
		r >>= 6;
	}

	while (seq && len < maxlen)
	{
		buf[len++] = base64[seq & 63];
		seq >>= 6;
	}

	buf[len] = '\0';

	return buf;
}

/*
============
COM_DescribeDuration

Describes the given duration, e.g. "3 minutes"
============
*/
void COM_DescribeDuration (char *out, size_t outsize, double seconds)
{
	const double SECOND = 1;
	const double MINUTE = 60 * SECOND;
	const double HOUR = 60 * MINUTE;
	const double DAY = 24 * HOUR;
	const double WEEK = 7 * DAY;
	const double MONTH = 30.436875 * DAY;
	const double YEAR = 365.2425 * DAY;

	seconds = fabs (seconds);

	if (seconds < 1)
		q_strlcpy (out, "moments", outsize);
	else if (seconds < 60 * SECOND)
		q_snprintf (out, outsize, "%i second%s", PLURAL (seconds));
	else if (seconds < 90 * MINUTE)
		q_snprintf (out, outsize, "%i minute%s", PLURAL (seconds / MINUTE));
	else if (seconds < DAY)
		q_snprintf (out, outsize, "%i hour%s", PLURAL (seconds / HOUR));
	else if (seconds < WEEK)
		q_snprintf (out, outsize, "%i day%s", PLURAL (seconds / DAY));
	else if (seconds < MONTH)
		q_snprintf (out, outsize, "%i week%s", PLURAL (seconds / WEEK));
	else if (seconds < YEAR)
		q_snprintf (out, outsize, "%i month%s", PLURAL (seconds / MONTH));
	else
		q_snprintf (out, outsize, "%i year%s", PLURAL (seconds / YEAR));
}

/*
============
COM_CreatePath
============
*/
void COM_CreatePath (char *path)
{
	char	*ofs;

	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}

/*
================
COM_filelength
================
*/
qfileofs_t COM_filelength (FILE *f)
{
	qfileofs_t	pos, end;

	pos = Sys_ftell (f);
	Sys_fseek (f, 0, SEEK_END);
	end = Sys_ftell (f);
	Sys_fseek (f, pos, SEEK_SET);

	return end;
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
If neither of file or handle is set, this
can be used for detecting a file's presence.
===========
*/
static int COM_FindFile (const char *filename, int *handle, FILE **file,
							unsigned int *path_id)
{
	searchpath_t	*search;
	char		netpath[MAX_OSPATH];
	pack_t		*pak;
	int			i;

	if (file && handle)
		Sys_Error ("COM_FindFile: both handle and file set");

	file_from_pak = 0;

//
// search through the path, one element at a time
//
	for (search = com_searchpaths; search; search = search->next)
	{
		if (search->pack)	/* look through all the pak file elements */
		{
			pak = search->pack;
			for (i = 0; i < pak->numfiles; i++)
			{
				if (strcmp(pak->files[i].name, filename) != 0)
					continue;
				// found it!
				com_filesize = pak->files[i].filelen;
				file_from_pak = 1;
				if (path_id)
					*path_id = search->path_id;
				if (handle)
				{
					*handle = pak->handle;
					Sys_FileSeek (pak->handle, pak->files[i].filepos);
					return com_filesize;
				}
				else if (file)
				{ /* open a new file on the pakfile */
					*file = Sys_fopen (pak->filename, "rb");
					if (*file)
						fseek (*file, pak->files[i].filepos, SEEK_SET);
					return com_filesize;
				}
				else /* for COM_FileExists() */
				{
					return com_filesize;
				}
			}
		}
		else	/* check a file in the directory tree */
		{
			if (!registered.value)
			{ /* if not a registered version, don't ever go beyond base */
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}

			q_snprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			if (! (Sys_FileType(netpath) & FS_ENT_FILE))
				continue;

			if (path_id)
				*path_id = search->path_id;
			if (handle)
			{
				com_filesize = Sys_FileOpenRead (netpath, &i);
				*handle = i;
				return com_filesize;
			}
			else if (file)
			{
				*file = Sys_fopen (netpath, "rb");
				com_filesize = (*file == NULL) ? -1 : COM_filelength (*file);
				return com_filesize;
			}
			else
			{
				return 0; /* dummy valid value for COM_FileExists() */
			}
		}
	}

	if (developer.value)
	{
		const char *ext = COM_FileGetExtension (filename);

		if (strcmp(ext, "pcx") != 0 &&
			strcmp(ext, "tga") != 0 &&
			strcmp(ext, "png") != 0 &&
			strcmp(ext, "jpg") != 0 &&
			strcmp(ext, "lmp") != 0 &&
			// music formats
			strcmp (ext, "ogg") != 0 &&
			strcmp (ext, "opus") != 0 &&
			strcmp (ext, "flac") != 0 &&
			strcmp (ext, "wav") != 0 &&
			strcmp (ext, "it") != 0 &&
			strcmp (ext, "s3m") != 0 &&
			strcmp (ext, "xm") != 0 &&
			strcmp (ext, "mod") != 0 &&
			strcmp (ext, "umx") != 0 &&
			// alternate model formats
			strcmp(ext, "md5mesh") != 0 &&
			strcmp (ext, "md3") != 0 &&
			strcmp (ext, "skin") != 0 &&
			// optional map files
			strcmp(ext, "lit") != 0 &&
			strcmp(ext, "vis") != 0 &&
			strcmp(ext, "ent") != 0)
			Con_DPrintf ("FindFile: can't find %s\n", filename);
		else
			Con_DPrintf2 ("FindFile: can't find %s\n", filename);
	}

	if (handle)
		*handle = -1;
	if (file)
		*file = NULL;
	com_filesize = -1;
	return com_filesize;
}


/*
===========
COM_FileExists

Returns whether the file is found in the quake filesystem.
===========
*/
qboolean COM_FileExists (const char *filename, unsigned int *path_id)
{
	int ret = COM_FindFile (filename, NULL, NULL, path_id);
	return (ret == -1) ? false : true;
}

/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
int COM_OpenFile (const char *filename, int *handle, unsigned int *path_id)
{
	return COM_FindFile (filename, handle, NULL, path_id);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int COM_FOpenFile (const char *filename, FILE **file, unsigned int *path_id)
{
	return COM_FindFile (filename, NULL, file, path_id);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile (int h)
{
	searchpath_t	*s;

	for (s = com_searchpaths; s; s = s->next)
		if (s->pack && s->pack->handle == h)
			return;

	Sys_FileClose (h);
}


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/
#define	LOADFILE_HUNK		0
#define	LOADFILE_MALLOC		1

byte *COM_LoadFile (const char *path, int usehunk, unsigned int *path_id)
{
	int		h;
	byte	*buf;
	char	base[32];
	int	len, nread;

	buf = NULL;	// quiet compiler warning

// look for it in the filesystem or pack files
	len = COM_OpenFile (path, &h, path_id);
	if (h == -1)
		return NULL;

// extract the filename base name for hunk tag
	COM_FileBase (path, base, sizeof(base));

	switch (usehunk)
	{
	case LOADFILE_HUNK:
		buf = (byte *) Hunk_AllocNameNoFill (len+1, base);
		break;
	case LOADFILE_MALLOC:
		buf = (byte *) malloc (len+1);
		break;
	default:
		Sys_Error ("COM_LoadFile: bad usehunk");
	}

	if (!buf)
		Sys_Error ("COM_LoadFile: not enough space for %s", path);

	((byte *)buf)[len] = 0;

	nread = Sys_FileRead (h, buf, len);
	COM_CloseFile (h);
	if (nread != len)
		Sys_Error ("COM_LoadFile: Error reading %s", path);

	return buf;
}

byte *COM_LoadHunkFile (const char *path, unsigned int *path_id)
{
	return COM_LoadFile (path, LOADFILE_HUNK, path_id);
}

// returns malloc'd memory
byte *COM_LoadMallocFile (const char *path, unsigned int *path_id)
{
	return COM_LoadFile (path, LOADFILE_MALLOC, path_id);
}

byte *COM_LoadMallocFile_TextMode_OSPath (const char *path, long *len_out)
{
	FILE	*f;
	byte	*data;
	long	len, actuallen;

	// ericw -- this is used by Host_Loadgame_f. Translate CRLF to LF on load games,
	// othewise multiline messages have a garbage character at the end of each line.
	// TODO: could handle in a way that allows loading CRLF savegames on mac/linux
	// without the junk characters appearing.
	f = Sys_fopen (path, "rt");
	if (f == NULL)
		return NULL;

	len = COM_filelength (f);
	if (len < 0)
	{
		fclose (f);
		return NULL;
	}

	data = (byte *) malloc (len + 1);
	if (data == NULL)
	{
		fclose (f);
		return NULL;
	}

	// (actuallen < len) if CRLF to LF translation was performed
	actuallen = fread (data, 1, len, f);
	if (ferror(f))
	{
		fclose (f);
		free (data);
		return NULL;
	}
	data[actuallen] = '\0';

	if (len_out != NULL)
		*len_out = actuallen;
	fclose (f);
	return data;
}

char *COM_NormalizeLineEndings (char *buffer)
{
	char *src, *dst;

	src = dst = buffer;

	while (*src)
	{
		if (src == dst)
		{
			while (*src && *src != '\r')
				src++;
			dst = src;
		}
		else
		{
			while (*src && *src != '\r')
				*dst++ = *src++;
		}

		if (!*src)
			break;

		src++;
		if (*src != '\n')
			*dst++ = '\n';
	}

	*dst = '\0';

	return buffer;
}

const char *COM_ParseIntNewline(const char *buffer, int *value)
{
	char *end;
	*value = strtol (buffer, &end, 10);
	while (q_isspace (*end))
		++end;
	return end;
}

const char *COM_ParseFloatNewline(const char *buffer, float *value)
{
	char *end;
	*value = strtof (buffer, &end);
	while (q_isspace (*end))
		++end;
	return end;
}

const char *COM_ParseStringNewline(const char *buffer)
{
	int i;
	for (i = 0; i < countof (com_token) - 1; i++)
		if (!buffer[i] || q_isspace (buffer[i]))
			break;
	memcpy (com_token, buffer, i);
	com_token[i] = '\0';
	while (q_isspace (buffer[i]))
		++i;
	return buffer + i;
}

/*
=================
COM_LoadPackFile -- johnfitz -- modified based on topaz's tutorial

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
static pack_t *COM_LoadPackFile (const char *packfile)
{
	dpackheader_t	header;
	int		i;
	packfile_t	*newfiles;
	int		numpackfiles;
	pack_t		*pack;
	int		packhandle;
	dpackfile_t	info[MAX_FILES_IN_PACK];

	if (Sys_FileOpenRead (packfile, &packhandle) == -1)
		return NULL;

	if (Sys_FileRead(packhandle, &header, sizeof(header)) != (int) sizeof(header) ||
	    header.id[0] != 'P' || header.id[1] != 'A' || header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error ("%s is not a packfile", packfile);

	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (header.dirlen < 0 || header.dirofs < 0)
	{
		Sys_Error ("Invalid packfile %s (dirlen: %i, dirofs: %i)",
					packfile, header.dirlen, header.dirofs);
	}
	if (!numpackfiles)
	{
		Sys_Printf ("WARNING: %s has no files, ignored\n", packfile);
		Sys_FileClose (packhandle);
		return NULL;
	}
	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error ("%s has %i files", packfile, numpackfiles);

	if (numpackfiles != PAK0_COUNT)
		com_modified = true;	// not the original file

	newfiles = (packfile_t *) Z_Malloc(numpackfiles * sizeof(packfile_t));

	Sys_FileSeek (packhandle, header.dirofs);
	if (Sys_FileRead(packhandle, info, header.dirlen) != header.dirlen)
		Sys_Error ("Error reading %s", packfile);

	// crc the directory to check for modifications
	if (!com_modified)
	{
		unsigned short	crc = CRC_Block (info, header.dirlen);
		if (crc != PAK0_CRC_V106 && crc != PAK0_CRC_V101 && crc != PAK0_CRC_V100)
			com_modified = true;
	}

	// parse the directory
	for (i = 0; i < numpackfiles; i++)
	{
		q_strlcpy (newfiles[i].name, info[i].name, sizeof(newfiles[i].name));
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	pack = (pack_t *) Z_Malloc (sizeof (pack_t));
	q_strlcpy (pack->filename, packfile, sizeof(pack->filename));
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;

	//Sys_Printf ("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}

const char *COM_GetGameNames(qboolean full)
{
	if (full)
	{
		if (*com_gamenames)
			return va("%s;%s", GAMENAME, com_gamenames);
		else
			return GAMENAME;
	}
	return com_gamenames;
//	return COM_SkipPath(com_gamedir);
}

/*
=================
COM_AddEnginePak
=================
*/
static void COM_AddEnginePak (void)
{
	int			i;
	char		pakfile[MAX_OSPATH];
	pack_t		*pak = NULL;
	qboolean	modified = com_modified;

	if (host_parms->exedir)
	{
		q_snprintf (pakfile, sizeof(pakfile), "%s/" ENGINE_PAK, host_parms->exedir);
		pak = COM_LoadPackFile (pakfile);
	}

	if (!pak)
	{
		q_snprintf (pakfile, sizeof(pakfile), "%s/" ENGINE_PAK, host_parms->basedir);
		pak = COM_LoadPackFile (pakfile);
	}

	if (!pak)
	{
		for (i = 0; i < com_numbasedirs; i++)
		{
			q_snprintf (pakfile, sizeof(pakfile), "%s/" ENGINE_PAK, com_basedirs[i]);
			pak = COM_LoadPackFile (pakfile);
			if (pak)
				break;
		}
	}

	if (pak)
	{
		searchpath_t *search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
		search->path_id = com_searchpaths ? com_searchpaths->path_id : 1u;
		search->pack = pak;
		search->next = com_searchpaths;
		com_searchpaths = search;
	}

	com_modified = modified;
}

/*
=================
COM_AddGameDirectory -- johnfitz -- modified based on topaz's tutorial
=================
*/
void COM_AddGameDirectory (const char *dir)
{
	const char *base;
	int i, j;
	unsigned int path_id;
	searchpath_t *search;
	pack_t *pak;
	char pakfile[MAX_OSPATH];

	if (*com_gamenames)
		q_strlcat(com_gamenames, ";", sizeof(com_gamenames));
	q_strlcat(com_gamenames, dir, sizeof(com_gamenames));

	// quakespasm enables mission pack flags automatically,
	// so e.g. -game rogue works without breaking the hud
	if (!q_strcasecmp(dir,"rogue")) {
		rogue = true;
		standard_quake = false;
	}
	if (!q_strcasecmp(dir,"hipnotic") || !q_strcasecmp(dir,"quoth")) {
		hipnotic = true;
		standard_quake = false;
	}
	if (!q_strcasecmp(dir,"q64")) {
		quake64 = true;
	}

	// assign a path_id to this game directory
	if (com_searchpaths)
		path_id = com_searchpaths->path_id << 1;
	else
		path_id = 1U;

	for (j = 0; j < com_numbasedirs; j++)
	{
		base = com_basedirs[j];
		q_snprintf (com_gamedir, sizeof (com_gamedir), "%s/%s", base, dir);

		// add the directory to the search path
		search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
		search->path_id = path_id;
		q_strlcpy (search->filename, com_gamedir, sizeof(search->filename));
		search->next = com_searchpaths;
		com_searchpaths = search;

		// add any pak files in the format pak0.pak pak1.pak, ...
		for (i = 0; ; i++)
		{
			q_snprintf (pakfile, sizeof(pakfile), "%s/pak%i.pak", com_gamedir, i);
			pak = COM_LoadPackFile (pakfile);
			if (!pak)
				break;

			search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
			search->path_id = path_id;
			search->pack = pak;
			search->next = com_searchpaths;
			com_searchpaths = search;

			// add engine pak after pak0.pak
			if (i == 0 && j == 0 && path_id == 1u && !fitzmode)
				COM_AddEnginePak ();
		}
	}
}

void COM_ResetGameDirectories(const char *newgamedirs)
{
	const char *newpath, *path;
	searchpath_t *search;
	//Kill the extra game if it is loaded
	while (com_searchpaths != com_base_searchpaths)
	{
		if (com_searchpaths->pack)
		{
			Sys_FileClose (com_searchpaths->pack->handle);
			Z_Free (com_searchpaths->pack->files);
			Z_Free (com_searchpaths->pack);
		}
		search = com_searchpaths->next;
		Z_Free (com_searchpaths);
		com_searchpaths = search;
	}
	hipnotic = false;
	rogue = false;
	quake64 = false;
	standard_quake = true;
	//wipe the list of mod gamedirs
	*com_gamenames = 0;
	//reset this too
	q_strlcpy (com_gamedir, va("%s/%s", com_basedirs[com_numbasedirs-1], GAMENAME), sizeof(com_gamedir));

	for(newpath = newgamedirs; newpath && *newpath; )
	{
		char *e = strchr(newpath, ';');
		if (e)
			*e++ = 0;

		if (!q_strcasecmp(GAMENAME, newpath))
			path = NULL;
		else for (path = newgamedirs; path < newpath; path += strlen(path)+1)
		{
			if (!q_strcasecmp(path, newpath))
				break;
		}

		if (path == newpath)	//not already loaded
			COM_AddGameDirectory(newpath);
		newpath = e;
	}
}

//==============================================================================
//johnfitz -- dynamic gamedir stuff -- modified by QuakeSpasm team.
//==============================================================================

/*
=================
COM_GameDirExists

Checks if a gamedir exists in any basedir
=================
*/
static qboolean COM_GameDirExists (const char *game)
{
	int i;

	for (i = 0; i < com_numbasedirs; i++)
		if (Sys_FileType (va ("%s/%s", com_basedirs[i], game)) == FS_ENT_DIRECTORY)
			return true;

	return false;
}

/*
=================
COM_ValidateGameDirs
=================
*/
static qboolean COM_ValidateGameDirs (const char *newgamedirs)
{
	char	dirscopy[1024];
	char	*newpath;

	q_strlcpy (dirscopy, newgamedirs, sizeof (dirscopy));

	// parse the individual semicolon-separated gamedirs (e.g. id1;ad;ad_heresp1)
	for (newpath = dirscopy; newpath && *newpath; )
	{
		char *p = strchr (newpath, ';');
		if (p)
			*p++ = 0;

		if (!COM_GameDirExists (newpath))
		{
			Con_Printf ("No such game directory \"%s\"\n", newpath);
			return false;
		}

		newpath = p;
	}

	return true;
}

/*
=================
COM_SwitchGame
=================
*/
void COM_SwitchGame (const char *paths)
{
	extern cvar_t max_edicts;
	if (!q_strcasecmp(paths, COM_GetGameNames(true)))
	{
		Con_Printf("\"game\" is already \"%s\"\n", COM_GetGameNames(true));
		return;
	}

	if (!COM_ValidateGameDirs (paths))
		return;

	Host_WaitForSaveThread ();

	com_modified = true;

	//Kill the server
	CL_Disconnect ();
	Host_ShutdownServer(true);

	//Write config file
	Host_WriteConfiguration ();

	// stop parsing map files before changing file system search paths
	ExtraMaps_Clear ();

	COM_ResetGameDirectories(paths);

	//clear out and reload appropriate data
	Cache_Flush ();
	Mod_ResetAll();
	Sky_ClearAll();
	if (!isDedicated)
	{
		TexMgr_NewGame ();
		Draw_NewGame ();
		R_NewGame ();
		BGM_Stop ();
	}
	ExtraMaps_Init ();
	Host_Resetdemos ();
	DemoList_Rebuild ();
	SaveList_Rebuild ();
	SkyList_Rebuild ();
	M_CheckMods ();
	Cvar_SetQuick (&max_edicts, max_edicts.default_string);

	Con_Printf("\n%s\n\"game\" changed to \"%s\"\n", Con_Quakebar (40), COM_GetGameNames(true));

	VID_Lock ();
	Cbuf_AddText ("unaliasall\n");
	Cbuf_AddText ("exec quake.rc\n");
	Cbuf_AddText ("vid_unlock\n");
}


/*
=================
COM_Game_f
=================
*/
static void COM_Game_f (void)
{
	if (Cmd_Argc() > 1)
	{
		int i, pri;
		char paths[1024];

		if (!registered.value) //disable shareware quake
		{
			Con_Printf("You must have the registered version to use modified games\n");
			return;
		}

		*paths = 0;
		q_strlcat(paths, GAMENAME, sizeof(paths));
		for (pri = 0; pri <= 1; pri++)
		{
			for (i = 1; i < Cmd_Argc(); i++)
			{
				const char *p = Cmd_Argv(i);
				if (!*p)
					p = GAMENAME;
				if (pri == 0)
				{
					if (*p != '-')
						continue;
					p++;
				}
				else if (*p == '-')
					continue;
				
				if (!*p || !strcmp(p, ".") || strstr(p, "..") || strstr(p, "/") || strstr(p, "\\") || strstr(p, ":"))
				{
					Con_Printf ("gamedir should be a single directory name, not a path\n");
					return;
				}

				if (!q_strcasecmp(p, GAMENAME))
					continue; //don't add id1, its not interesting enough.

				if (*paths)
					q_strlcat(paths, ";", sizeof(paths));
				q_strlcat(paths, p, sizeof(paths));
			}
		}

		COM_SwitchGame (paths);
	}
	else //Diplay the current gamedir
		Con_Printf("\"game\" is \"%s\"\n", COM_GetGameNames(true));
}

/*
=================
COM_SetBaseDir
=================
*/
static qboolean COM_SetBaseDir (const char *path)
{
	const char pak0[] = "/" GAMENAME "/pak0.pak";
	char pakpath[countof (com_basedirs[0])];
	size_t i;

	i = strlen (path);
	if (i && (path[i - 1] == '/' || path[i - 1] == '\\'))
		--i;
	if (i + countof (pak0) > countof (pakpath))
		return false;

	memcpy (pakpath, path, i);
	memcpy (pakpath + i, pak0, sizeof (pak0));
	if (!Sys_FileExists (pakpath))
		return false;

	memcpy (com_basedirs[0], path, i);
	com_basedirs[0][i] = 0;
	com_numbasedirs = 1;

	return true;
}

/*
=================
COM_AddBaseDir
=================
*/
static void COM_AddBaseDir (const char *path)
{
	if (com_numbasedirs >= countof (com_basedirs))
		Sys_Error ("Too many basedirs (%d)", com_numbasedirs);
	if ((size_t) q_strlcpy (com_basedirs[com_numbasedirs++], path, sizeof (com_basedirs[0])) >= sizeof (com_basedirs[0]))
		Sys_Error ("Basedir too long (%d characters, max %d):\n%s\n", (int)strlen (path), (int)sizeof (com_basedirs[0]), path);
}

/*
=================
COM_MigrateNightdiveUserFiles

Checks the Nightdive dir for subdirs containing
an ironwail.cfg file and moves known files over
to the new dir
=================
*/
static void COM_MigrateNightdiveUserFiles (void)
{
	const char	*episodes[] = {"id1", "hipnotic", "rogue", "dopa", "mg1"};
	const char	*filetypes[] = {"cfg", "txt", "sav", "dem", "png", "jpg"};
	const char	*game, *ext;
	char		src[MAX_OSPATH];
	char		dst[MAX_OSPATH];
	char		*subdirs = NULL;
	findfile_t	*moditer, *fileiter;
	size_t		i;

	// move episode dirs if they contain a config file
	for (i = 0; i < countof (episodes); i++)
	{
		const char *game = episodes[i];
		if ((size_t) q_snprintf (src, sizeof (src), "%s/%s/%s", com_nightdivedir, game, CONFIG_NAME) >= sizeof (src))
			continue;
		if (!Sys_FileExists (src))
			continue;
		q_snprintf (src, sizeof (src), "%s/%s", com_nightdivedir, game);
		q_snprintf (dst, sizeof (dst), "%s/%s", com_userprefdir, game);
		Sys_rename (src, dst);
	}

	// iterate through all remaining subdirs
	for (moditer = Sys_FindFirst (com_nightdivedir, NULL); moditer; moditer = Sys_FindNext (moditer))
	{
		char srcmod[MAX_OSPATH];
		char dstmod[MAX_OSPATH];
		char *cfg;

		if (!(moditer->attribs & FA_DIRECTORY))
			continue;
		if (!strcmp (moditer->name, ".") || !strcmp (moditer->name, ".."))
			continue;

		for (i = 0; i < countof (episodes); i++)
			if (!strcmp (moditer->name, episodes[i]))
				break;
		if (i != countof (episodes))
			continue;

		// look for engine config
		if ((size_t) q_snprintf (src, sizeof (src), "%s/%s/%s", com_nightdivedir, moditer->name, CONFIG_NAME) >= sizeof (src) ||
			(size_t) q_snprintf (dst, sizeof (dst), "%s/%s/%s", com_userprefdir, moditer->name, CONFIG_NAME) >= sizeof (dst))
			continue;
		cfg = (char *) COM_LoadMallocFile_TextMode_OSPath (src, NULL);
		if (!cfg)
			continue;

		// write config (and create directory structure as needed)
		COM_WriteFile_OSPath (dst, cfg, strlen (cfg));
		free (cfg);
		Sys_remove (src);

		// move all recognized files
		q_snprintf (srcmod, sizeof (srcmod), "%s/%s", com_nightdivedir, moditer->name);
		q_snprintf (dstmod, sizeof (dstmod), "%s/%s", com_userprefdir, moditer->name);
		for (fileiter = Sys_FindFirst (srcmod, NULL); fileiter; fileiter = Sys_FindNext (fileiter))
		{
			if (fileiter->attribs & FA_DIRECTORY)
				continue;

			ext = COM_FileGetExtension (fileiter->name);
			for (i = 0; i < countof (filetypes); i++)
				if (!q_strcasecmp (ext, filetypes[i]))
					break;
			if (i == countof (filetypes))
				continue;

			if ((size_t) q_snprintf (src, sizeof (src), "%s/%s", srcmod, fileiter->name) < sizeof (src) &&
				(size_t) q_snprintf (dst, sizeof (dst), "%s/%s", dstmod, fileiter->name) < sizeof (dst))
			{
				Sys_rename (src, dst);
			}
		}

		Vec_Append ((void **)&subdirs, 1, moditer->name, strlen (moditer->name) + 1);
	}

	VEC_PUSH (subdirs, '\0');

	// remove empty dirs
	for (game = subdirs; *game; game += strlen (game) + 1)
	{
		q_snprintf (src, sizeof (src), "%s/%s", com_nightdivedir, game);
		Sys_remove (game);
	}

	VEC_FREE (subdirs);
}

/*
=================
COM_SetBaseDirRec

Looks for a valid basedir in all the ancestors of the supplied path
Returns the path relative to that basedir if successful, NULL otherwise
=================
*/
static const char *COM_SetBaseDirRec (const char *start)
{
	char	buf[MAX_OSPATH];
	size_t	i, len;

	q_strlcpy (buf, start, sizeof (buf));
	len = strlen (start);

	for (i = len - 1; i > 1; i--)
	{
		if (Sys_IsPathSep (buf[i]))
		{
			buf[i] = '\0';
			if (COM_SetBaseDir (buf))
				return start + i + 1;
		}
	}

	return NULL;
}

/*
=================
COM_MakeRelative
=================
*/
static const char *COM_MakeRelative (const char *basepath, const char *fullpath)
{
	for (; *basepath && *fullpath; ++basepath, ++fullpath)
	{
		if (Sys_IsPathSep (*basepath) != Sys_IsPathSep (*fullpath))
			return NULL;
		if (*basepath != *fullpath)
			return NULL;
	}

	while (Sys_IsPathSep (*fullpath))
		++fullpath;

	return fullpath;
}

/*
=================
COM_PatchCmdLine

Tries to initialize basedir from a single command-line argument
(either a mod dir or a map/save/demo file)

Returns true if successful, false otherwise
=================
*/
static qboolean COM_PatchCmdLine (const char *fullpath)
{
	static char	game[MAX_QPATH];
	char		qpath[MAX_QPATH];
	char		printpath[MAX_OSPATH];
	const char	*relpath;
	const char	*sep;
	int			type;
	int			i;

	// The path (file or directory) must exist
	type = Sys_FileType (fullpath);
	if (type == FS_ENT_NONE)
	{
		UTF8_ToQuake (printpath, sizeof (printpath), fullpath);
		Con_SafePrintf ("\"%s\" does not exist\n", printpath);
		return false;
	}

	// Look for the corresponding basedir
	relpath = NULL;
	for (i = 0; i < com_numbasedirs; i++)
	{
		relpath = COM_MakeRelative (com_basedirs[i], fullpath);
		if (relpath)
			break;
	}
	if (!relpath)
	{
		UTF8_ToQuake (printpath, sizeof (printpath), fullpath);
		Con_SafePrintf ("\"%s\" does not belong to an existing Quake installation\n", printpath);
		return false;
	}

	// Game dir is the first component of the relative path
	sep = COM_FirstPathSep (relpath);
	if ((uintptr_t)(sep - relpath) >= sizeof (game))
	{
		UTF8_ToQuake (printpath, sizeof (printpath), relpath);
		Con_SafePrintf ("\"%s\" is too long\n", printpath);
		return false;
	}

	UTF8_ToQuake (printpath, sizeof (printpath), relpath);

	// Apply game dir
	if (*sep)
	{
		Q_strncpy (game, relpath, (int)(sep - relpath));
		COM_AddArg ("-game");
		COM_AddArg (game);
		relpath = sep + 1;
	}
	else if (type == FS_ENT_DIRECTORY)
	{
		COM_AddArg ("-game");
		COM_AddArg (relpath);
		return true;
	}
	else
	{
		game[0] = '\0';
	}

	q_strlcpy (qpath, relpath, sizeof (qpath));
	COM_NormalizePath (qpath);

	// Check argument type
	switch (type)
	{
	case FS_ENT_DIRECTORY:
		if (qpath[0])
		{
			if (q_strcasecmp (qpath, "maps") == 0)
			{
				Cbuf_AddText ("menu_maps\n");
				return true;
			}
			UTF8_ToQuake (printpath, sizeof (printpath), qpath);
			Con_SafePrintf ("\x02subdir \"%s\" ignored\n", printpath);
		}
		return true;

	case FS_ENT_FILE:
		{
			const char *ext = COM_FileGetExtension (qpath);

			// Map file
			if (q_strcasecmp (ext, "bsp") == 0)
			{
				if (!game[0])
				{
					Con_SafePrintf ("Map \"%s\" not in a mod dir, ignoring.\n", printpath);
					return false;
				}
				if (q_strncasecmp (qpath, "maps/", 5) != 0)
				{
					Con_SafePrintf ("Map \"%s\" not in the \"maps\" dir, ignoring.\n", printpath);
					return false;
				}
				memmove (qpath, qpath + 5, strlen (qpath + 5) + 1);
				Cbuf_AddText (va ("menu_maps \"%s\"\n", qpath));
				return true;
			}

			// Save file
			if (q_strcasecmp (ext, "sav") == 0)
			{
				const char *kex = game[0] ? "" : "kex";
				Cbuf_AddText (va ("load \"%s\" %s\n", qpath, kex));
				return true;
			}

			// Demo file
			if (q_strcasecmp (ext, "dem") == 0)
			{
				if (!game[0])
				{
					Con_SafePrintf ("Demo \"%s\" not in a mod dir, ignoring.\n", printpath);
					return false;
				}
				Cbuf_AddText (va ("playdemo \"%s\"\n", qpath));
				return true;
			}

			break;
		}

	default:
		break;
	}

	if (!game[0])
		Con_SafePrintf ("File \"%s\" not in a mod dir, ignoring.\n", printpath);
	else
		Con_SafePrintf ("Unsupported file type \"%s\", ignoring.\n", printpath);

	return false;
}

/*
=================
COM_InitBaseDir
=================
*/
static void COM_InitBaseDir (void)
{
	steamgame_t steamquake;
	char path[MAX_OSPATH];
	char original[MAX_OSPATH] = {0};
	char remastered[MAX_OSPATH] = {0};
	int i, steam, gog, egs;

	// command-line basedir takes priority over everything else
	i = COM_CheckParm ("-basedir");
	if (i)
	{
		const char *dir;
		if (i >= com_argc - 1)
			Sys_Error (
				"Please specify a valid Quake directory after -basedir\n"
				"(one that has an " GAMENAME " subdirectory containing pak0.pak)\n"
			);

		dir = com_argv[++i];
		if (!COM_SetBaseDir (dir))
			Sys_Error (
				"The specified -basedir is not a valid Quake directory:\n"
				"%s\n"
				"doesn't have an " GAMENAME " subdirectory containing pak0.pak.\n",
				dir
			);

		for (;;) // add all other -basedirs
		{
			i = COM_CheckParmNext (i, "-basedir");
			if (!i)
				break;
			if (i >= com_argc - 1)
				Sys_Error ("Please specify a directory after -basedir\n");
			COM_AddBaseDir (com_argv[++i]);
		}

		return;
	}

	// skip default basedir if a store version is requested explicitly
	steam = COM_CheckParm ("-steam");
	if (steam)
		goto try_steam;
	gog = COM_CheckParm ("-gog");
	if (gog)
		goto try_gog;
	egs = COM_CheckParm ("-egs");
	if (!egs)
		egs = COM_CheckParm ("-epic");
	if (egs)
		goto try_egs;

	// try current working directory, then its ancestors (in case the executable is in its own subdirectory)
	if (COM_SetBaseDir (host_parms->basedir))
		return;
	if (COM_SetBaseDirRec (host_parms->basedir))
		return;

	// on Linux, game data might actually be in the user dir
	if (host_parms->userdir && host_parms->userdir != host_parms->basedir && COM_SetBaseDir (host_parms->userdir))
	{
		host_parms->basedir = host_parms->userdir;
		return;
	}

	if (!COM_CheckParm ("-nosteam"))
	{
	try_steam:
		if (Steam_FindGame (&steamquake, QUAKE_STEAM_APPID) &&
			Steam_ResolvePath (original, sizeof (original), &steamquake))
		{
			if ((size_t) q_snprintf (remastered, sizeof (remastered), "%s/rerelease", original) >= sizeof (remastered))
				remastered[0] = '\0';
			else if (!Sys_GetSteamQuakeUserDir (com_nightdivedir, sizeof (com_nightdivedir), steamquake.library))
				com_nightdivedir[0] = '\0';
		}
		else
		{
			memset (&steamquake, 0, sizeof (steamquake));
		}
		if (steam)
			goto storesetup;
	}

	if (!COM_CheckParm ("-nogog"))
	{
	try_gog:
		if (!original[0] && !Sys_GetGOGQuakeDir (original, sizeof (original)))
			original[0] = '\0';
		if (!remastered[0])
		{
			if (Sys_GetGOGQuakeEnhancedDir (remastered, sizeof (remastered)))
			{
				if (!com_nightdivedir[0] && !Sys_GetGOGQuakeEnhancedUserDir (com_nightdivedir, sizeof (com_nightdivedir)))
					com_nightdivedir[0] = '\0';
			}
			else
				remastered[0] = '\0';
		}
		if (gog)
			goto storesetup;
	}

	if (!COM_CheckParm ("-noegs") && !COM_CheckParm ("-noepic"))
	{
	try_egs:
		if (!remastered[0])
		{
			if (EGS_FindGame (remastered, sizeof (remastered), QUAKE_EGS_NAMESPACE, QUAKE_EGS_ITEM_ID, QUAKE_EGS_APP_NAME))
			{
				// same directory as GOG
				if (!Sys_GetGOGQuakeEnhancedUserDir (com_nightdivedir, sizeof (com_nightdivedir)))
					com_nightdivedir[0] = '\0';
			}
			else
				remastered[0] = '\0';
		}
		if (egs)
			goto storesetup;
	}

storesetup:
	if (original[0] || remastered[0])
	{
		quakeflavor_t flavor;
		if (original[0] && remastered[0])
		{
			if (COM_CheckParm ("-prefremaster") || COM_CheckParm ("-remaster") || COM_CheckParm ("-remastered"))
				flavor = QUAKE_FLAVOR_REMASTERED;
			else if (COM_CheckParm ("-preforiginal") || COM_CheckParm ("-original"))
				flavor = QUAKE_FLAVOR_ORIGINAL;
			else
				flavor = ChooseQuakeFlavor ();
		}
		else
			flavor = remastered[0] ? QUAKE_FLAVOR_REMASTERED : QUAKE_FLAVOR_ORIGINAL;
		q_strlcpy (path, flavor == QUAKE_FLAVOR_REMASTERED ? remastered : original, sizeof (path));

		if (steamquake.appid)
			Steam_Init (&steamquake);

		if (COM_SetBaseDir (path))
		{
			if (!Sys_GetAltUserPrefDir (flavor == QUAKE_FLAVOR_REMASTERED, com_userprefdir, sizeof (com_userprefdir)))
				Sys_Error ("Couldn't set up settings dir");

			if (flavor == QUAKE_FLAVOR_REMASTERED)
			{
				if (com_nightdivedir[0])
				{
					COM_MigrateNightdiveUserFiles ();
					COM_AddBaseDir (com_nightdivedir);
				}
				else
					Con_Warning ("Nightdive dir not found\n");
			}
			else
			{
				com_nightdivedir[0] = '\0';
			}
			
			host_parms->userdir = com_userprefdir;

			return;
		}
	}

	if (steam)
		Sys_Error ("Couldn't find Steam Quake");
	if (gog)
		Sys_Error ("Couldn't find GOG Quake");
	if (egs)
		Sys_Error ("Couldn't find Epic Games Store Quake");

	Sys_Error (
		"Couldn't determine where Quake is installed.\n"
		"Please use the -basedir option to specify a path\n"
		"(with an " GAMENAME " subdirectory containing pak0.pak)"
	);
}

/*
=================
COM_ChooseStartArgFlavor

Checks if the supplied path belongs to the user dir
of a specific Quake flavor (original/remastered)
and adds the corresponding command-line argument if needed
=================
*/
static void COM_ChooseStartArgFlavor (const char *startarg)
{
	steamgame_t steamquake;
	char steampath[MAX_OSPATH];
	char userdir[MAX_OSPATH];

	if (Sys_GetAltUserPrefDir (true, userdir, sizeof (userdir)) && COM_MakeRelative (userdir, startarg))
	{
		COM_AddArg ("-prefremaster");
		return;
	}

	if (Sys_GetAltUserPrefDir (false, userdir, sizeof (userdir)) && COM_MakeRelative (userdir, startarg))
	{
		COM_AddArg ("-preforiginal");
		return;
	}

	if (Steam_FindGame (&steamquake, QUAKE_STEAM_APPID) &&
		Steam_ResolvePath (steampath, sizeof (steampath), &steamquake) &&
		Sys_GetSteamQuakeUserDir (userdir, sizeof (userdir), steamquake.library) &&
		COM_MakeRelative (userdir, startarg))
	{
		COM_AddArg ("-prefremaster");
		return;
	}

	if (Sys_GetGOGQuakeEnhancedUserDir (userdir, sizeof (userdir)) && COM_MakeRelative (userdir, startarg))
	{
		COM_AddArg ("-prefremaster");
		return;
	}
}

/*
=================
COM_InitFilesystem
=================
*/
void COM_InitFilesystem (void) //johnfitz -- modified based on topaz's tutorial
{
	int i;
	const char *p, *startarg;

	Cvar_RegisterVariable (&registered);
	Cvar_RegisterVariable (&cmdline);
	Cmd_AddCommand ("path", COM_Path_f);
	Cmd_AddCommand ("game", COM_Game_f); //johnfitz

	startarg = (com_argc == 2 && Sys_FileType (com_argv[1]) != FS_ENT_NONE) ? com_argv[1] : NULL;
	if (startarg)
		COM_ChooseStartArgFlavor (startarg);

	if (!startarg || !COM_SetBaseDirRec (startarg))
		COM_InitBaseDir ();

	if (host_parms->userdir != host_parms->basedir)
		COM_AddBaseDir (host_parms->userdir);

	if (startarg)
		COM_PatchCmdLine (startarg);

	i = COM_CheckParm ("-basegame");
	if (i)
	{	//-basegame:
		// a) replaces all hardcoded dirs (read: alternative to id1)
		// b) isn't flushed on normal gamedir switches (like id1).
		com_modified = true; //shouldn't be relevant when not using id content... but we don't really know.
		for(;; i = COM_CheckParmNext (i, "-basegame"))
		{
			if (!i || i >= com_argc-1)
				break;

			p = com_argv[i + 1];
			if (!*p || !strcmp(p, ".") || strstr(p, "..") || strstr(p, "/") || strstr(p, "\\") || strstr(p, ":"))
				Sys_Error ("gamedir should be a single directory name, not a path\n");
			if (p != NULL)
				COM_AddGameDirectory (p);
		}
	}
	else
	{
		// start up with GAMENAME by default (id1)
		COM_AddGameDirectory (GAMENAME);
	}

	/* this is the end of our base searchpath:
	 * any set gamedirs, such as those from -game command line
	 * arguments or by the 'game' console command will be freed
	 * up to here upon a new game command. */
	com_base_searchpaths = com_searchpaths;
	COM_ResetGameDirectories("");

	Modlist_Init ();

	// add mission pack requests (only one should be specified)
	if (COM_CheckParm ("-rogue"))
		COM_AddGameDirectory ("rogue");
	if (COM_CheckParm ("-hipnotic"))
		COM_AddGameDirectory ("hipnotic");
	if (COM_CheckParm ("-quoth"))
		COM_AddGameDirectory ("quoth");

	for(i = 0;;)
	{
		i = COM_CheckParmNext (i, "-game");
		if (!i || i >= com_argc-1)
			break;

		p = com_argv[i + 1];
		if (!*p || !strcmp(p, ".") || strstr(p, "..") || strstr(p, "/") || strstr(p, "\\") || strstr(p, ":"))
			Sys_Error ("gamedir should be a single directory name, not a path\n");
		if (!COM_GameDirExists (p))
			Sys_Error ("No such game directory \"%s\"", p);
		com_modified = true;
		if (p != NULL)
			COM_AddGameDirectory (p);
	}

	COM_CheckRegistered ();
}


/* The following FS_*() stdio replacements are necessary if one is
 * to perform non-sequential reads on files reopened on pak files
 * because we need the bookkeeping about file start/end positions.
 * Allocating and filling in the fshandle_t structure is the users'
 * responsibility when the file is initially opened. */

size_t FS_fread(void *ptr, size_t size, size_t nmemb, fshandle_t *fh)
{
	long byte_size;
	long bytes_read;
	size_t nmemb_read;

	if (!fh) {
		errno = EBADF;
		return 0;
	}
	if (!ptr) {
		errno = EFAULT;
		return 0;
	}
	if (!size || !nmemb) {	/* no error, just zero bytes wanted */
		errno = 0;
		return 0;
	}

	byte_size = nmemb * size;
	if (byte_size > fh->length - fh->pos)	/* just read to end */
		byte_size = fh->length - fh->pos;
	bytes_read = fread(ptr, 1, byte_size, fh->file);
	fh->pos += bytes_read;

	/* fread() must return the number of elements read,
	 * not the total number of bytes. */
	nmemb_read = bytes_read / size;
	/* even if the last member is only read partially
	 * it is counted as a whole in the return value. */
	if (bytes_read % size)
		nmemb_read++;

	return nmemb_read;
}

int FS_fseek(fshandle_t *fh, long offset, int whence)
{
/* I don't care about 64 bit off_t or fseeko() here.
 * the quake/hexen2 file system is 32 bits, anyway. */
	int ret;

	if (!fh) {
		errno = EBADF;
		return -1;
	}

	/* the relative file position shouldn't be smaller
	 * than zero or bigger than the filesize. */
	switch (whence)
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += fh->pos;
		break;
	case SEEK_END:
		offset = fh->length + offset;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	if (offset > fh->length)	/* just seek to end */
		offset = fh->length;

	ret = fseek(fh->file, fh->start + offset, SEEK_SET);
	if (ret < 0)
		return ret;

	fh->pos = offset;
	return 0;
}

int FS_fclose(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fclose(fh->file);
}

long FS_ftell(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->pos;
}

void FS_rewind(fshandle_t *fh)
{
	if (!fh) return;
	clearerr(fh->file);
	fseek(fh->file, fh->start, SEEK_SET);
	fh->pos = 0;
}

int FS_feof(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	if (fh->pos >= fh->length)
		return -1;
	return 0;
}

int FS_ferror(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return ferror(fh->file);
}

int FS_fgetc(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return EOF;
	}
	if (fh->pos >= fh->length)
		return EOF;
	fh->pos += 1;
	return fgetc(fh->file);
}

char *FS_fgets(char *s, int size, fshandle_t *fh)
{
	char *ret;

	if (FS_feof(fh))
		return NULL;

	if (size > (fh->length - fh->pos) + 1)
		size = (fh->length - fh->pos) + 1;

	ret = fgets(s, size, fh->file);
	fh->pos = ftell(fh->file) - fh->start;

	return ret;
}

long FS_filelength (fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->length;
}

/*
============================================================================
								LOCALIZATION
============================================================================
*/
typedef struct
{
	char *key;
	char *value;
} locentry_t;

typedef struct
{
	int			numentries;
	int			maxnumentries;
	int			numindices;
	unsigned	*indices;
	locentry_t	*entries;
	char		*text;
} localization_t;

static localization_t localization;

/*
================
COM_HashString
Computes the FNV-1a hash of string str
================
*/
unsigned COM_HashString (const char *str)
{
	unsigned hash = 0x811c9dc5u;
	while (*str)
	{
		hash ^= *str++;
		hash *= 0x01000193u;
	}
	return hash;
}

/*
================
COM_HashBlock
Computes the FNV-1a hash of a memory block
================
*/
unsigned COM_HashBlock (const void *data, size_t size)
{
	const byte *ptr = (const byte *)data;
	unsigned hash = 0x811c9dc5u;
	while (size--)
	{
		hash ^= *ptr++;
		hash *= 0x01000193u;
	}
	return hash;
}

static size_t mz_zip_file_read_func(void *opaque, mz_uint64 ofs, void *buf, size_t n)
{
	if (SDL_RWseek((SDL_RWops*)opaque, (Sint64)ofs, RW_SEEK_SET) < 0)
		return 0;
	return SDL_RWread((SDL_RWops*)opaque, buf, 1, n);
}

/*
================
LOC_LoadFile
================
*/
qboolean LOC_LoadFile (const char *file)
{
	char path[1024];
	int i,lineno,warnings;
	char *cursor;

	SDL_RWops *rw = NULL;
	Sint64 sz;
	mz_zip_archive archive;
	size_t size = 0;

	// clear existing data
	if (localization.text)
	{
		free(localization.text);
		localization.text = NULL;
	}
	localization.numentries = 0;
	localization.numindices = 0;

	if (!file || !*file)
		return false;

	memset(&archive, 0, sizeof(archive));
	for (i = com_numbasedirs - 1; i >= 0; i--)
	{
		q_snprintf(path, sizeof(path), "%s/%s", com_basedirs[i], file);
		rw = SDL_RWFromFile(path, "rb");
		if (rw)
			break;
	}
	if (!rw)
	{
		for (i = com_numbasedirs - 1; i >= 0; i--)
		{
			q_snprintf(path, sizeof(path), "%s/QuakeEX.kpf", com_basedirs[i]);
			rw = SDL_RWFromFile(path, "rb");
			if (rw)
				break;
		}
		if (!rw)
		{
			steamgame_t steamquake;
			char steampath[MAX_OSPATH];
			if (Steam_FindGame (&steamquake, QUAKE_STEAM_APPID) &&
				Steam_ResolvePath (steampath, sizeof (steampath), &steamquake))
			{
				q_snprintf(path, sizeof(path), "%s/rerelease/QuakeEX.kpf", steampath);
				rw = SDL_RWFromFile(path, "rb");
			}
		}
		if (!rw)
		{
			char gogpath[MAX_OSPATH];
			if (Sys_GetGOGQuakeEnhancedDir (gogpath, sizeof (gogpath)))
			{
				q_snprintf(path, sizeof(path), "%s/QuakeEX.kpf", gogpath);
				rw = SDL_RWFromFile(path, "rb");
			}
		}
		if (!rw)
		{
			char egspath[MAX_OSPATH];
			if (EGS_FindGame (egspath, sizeof (egspath), QUAKE_EGS_NAMESPACE, QUAKE_EGS_ITEM_ID, QUAKE_EGS_APP_NAME))
			{
				q_snprintf(path, sizeof(path), "%s/QuakeEX.kpf", egspath);
				rw = SDL_RWFromFile(path, "rb");
			}
		}
		if (!rw) goto fail;
		sz = SDL_RWsize(rw);
		if (sz <= 0) goto fail;
		archive.m_pRead = mz_zip_file_read_func;
		archive.m_pIO_opaque = rw;
		if (!mz_zip_reader_init(&archive, sz, 0)) goto fail;
		localization.text = (char *) mz_zip_reader_extract_file_to_heap(&archive, file, &size, 0);
		if (!localization.text) goto fail;
		mz_zip_reader_end(&archive);
		SDL_RWclose(rw);
		localization.text = (char *) realloc(localization.text, size+1);
		localization.text[size] = 0;
	}
	else
	{
		sz = SDL_RWsize(rw);
		if (sz <= 0) goto fail;
		localization.text = (char *) calloc(1, sz+1);
		if (!localization.text)
		{
fail:			mz_zip_reader_end(&archive);
			if (rw) SDL_RWclose(rw);
			Con_Printf("Couldn't load '%s'\n", file);
			return false;
		}
		SDL_RWread(rw, localization.text, 1, sz);
		SDL_RWclose(rw);
	}

	cursor = localization.text;

	// skip BOM
	if ((unsigned char)(cursor[0]) == 0xEF && (unsigned char)(cursor[1]) == 0xBB && (unsigned char)(cursor[2]) == 0xBF)
		cursor += 3;

	warnings = 0;
	lineno = 0;
	while (*cursor)
	{
		char *line, *equals;

		lineno++;

		// skip leading whitespace
		while (q_isblank(*cursor))
			++cursor;

		line = cursor;
		equals = NULL;
		// find line end and first equals sign, if any
		while (*cursor && *cursor != '\n')
		{
			if (*cursor == '=' && !equals)
				equals = cursor;
			cursor++;
		}

		if (line[0] == '/')
		{
			if (line[1] != '/')
				Con_DPrintf("LOC_LoadFile: malformed comment on line %d\n", lineno);
		}
		else if (equals)
		{
			char *key_end = equals;
			qboolean leading_quote;
			qboolean trailing_quote;
			locentry_t *entry;
			char *value_src;
			char *value_dst;
			char *value;

			// trim whitespace before equals sign
			while (key_end != line && q_isspace(key_end[-1]))
				key_end--;
			*key_end = 0;

			value = equals + 1;
			// skip whitespace after equals sign
			while (value != cursor && q_isspace(*value))
				value++;

			leading_quote = (*value == '\"');
			trailing_quote = false;
			value += leading_quote;

			// transform escape sequences in-place
			value_src = value;
			value_dst = value;
			while (value_src != cursor)
			{
				if (*value_src == '\\' && value_src + 1 != cursor)
				{
					char c = value_src[1];
					value_src += 2;
					switch (c)
					{
						case 'n': *value_dst++ = '\n'; break;
						case 't': *value_dst++ = '\t'; break;
						case 'v': *value_dst++ = '\v'; break;
						case 'b': *value_dst++ = '\b'; break;
						case 'f': *value_dst++ = '\f'; break;

						case '"':
						case '\'':
							*value_dst++ = c;
							break;

						default:
							Con_Printf("LOC_LoadFile: unrecognized escape sequence \\%c on line %d\n", c, lineno);
							*value_dst++ = c;
							break;
					}
					continue;
				}

				if (*value_src == '\"')
				{
					trailing_quote = true;
					*value_dst = 0;
					break;
				}

				*value_dst++ = *value_src++;
			}

			// if not a quoted string, trim trailing whitespace
			if (!trailing_quote)
			{
				while (value_dst != value && q_isblank(value_dst[-1]))
				{
					*value_dst = 0;
					value_dst--;
				}
			}

			if (localization.numentries == localization.maxnumentries)
			{
				// grow by 50%
				localization.maxnumentries += localization.maxnumentries >> 1;
				localization.maxnumentries = q_max(localization.maxnumentries, 32);
				localization.entries = (locentry_t*) realloc(localization.entries, sizeof(*localization.entries) * localization.maxnumentries);
			}

			UTF8_ToQuake (value, strlen (value) + 1, value);

			// Log entries with unprintable characters if developer is set to 2 or higher
			if (developer.value >= 2.f && strchr (value, QCHAR_BOX))
			{
				int trim = (int) strlen (value);
				// trim trailing newlines
				while (trim > 0 && value[trim - 1] == '\n')
					--trim;
				// print header before first entry
				if (!warnings++)
					Sys_Printf ("Entries with unprintable characters:\n");
				Sys_Printf ("   %d. %s = \"%.*s\"\n", warnings, line, trim, value);
			}

			entry = &localization.entries[localization.numentries++];
			entry->key = line;
			entry->value = value;
		}

		if (*cursor)
			*cursor++ = 0; // terminate line and advance to next
	}

	if (developer.value >= 2.f && warnings > 0)
		Sys_Printf ("%d strings with unprintable characters\n", warnings);

	// hash all entries

	localization.numindices = localization.numentries * 2; // 50% load factor
	if (localization.numindices == 0)
	{
		Con_Printf("No localized strings in file '%s'\n", file);
		return false;
	}

	localization.indices = (unsigned*) realloc(localization.indices, localization.numindices * sizeof(*localization.indices));
	memset(localization.indices, 0, localization.numindices * sizeof(*localization.indices));

	for (i = 0; i < localization.numentries; i++)
	{
		locentry_t *entry = &localization.entries[i];
		unsigned pos = COM_HashString(entry->key) % localization.numindices, end = pos;

		for (;;)
		{
			if (!localization.indices[pos])
			{
				localization.indices[pos] = i + 1;
				break;
			}

			++pos;
			if (pos == localization.numindices)
				pos = 0;

			if (pos == end)
				Sys_Error("LOC_LoadFile failed");
		}
	}

	Con_SafePrintf ("[skipnotify]Loaded %d strings from '%s'\n", localization.numentries, file);

	return true;
}

static const char *const knownlangs[][2] =
{
	{"",   "auto"},
	{"en", "english"},
	{"fr", "french"},
	{"de", "german"},
	{"it", "italian"},
	{"es", "spanish"},
};

// Different from language cvar if language is "auto"
static const char *userlang = "english";

/*
================
LOC_Load
================
*/
void LOC_Load(void)
{
	if (!LOC_LoadFile(va("localization/loc_%s.txt", userlang)))
		LOC_LoadFile("localization/loc_english.txt");
}

/*
================
LOC_GetSystemLanguage
================
*/
const char *LOC_GetSystemLanguage (void)
{
#if SDL_VERSION_ATLEAST (2, 0, 14)
	size_t i, j;
	SDL_Locale *prefs = SDL_GetPreferredLocales ();

	if (!prefs)
		return "english";

	for (i = 0; prefs[i].language; i++)
	{
		for (j = 1; j < countof (knownlangs); j++) // start at 1, skipping "auto"
		{
			if (!q_strcasecmp (prefs[i].language, knownlangs[j][0]))
			{
				SDL_free (prefs);
				return knownlangs[j][1];
			}
		}
	}

	SDL_free (prefs);
#else
	#pragma message("Warning: automatic language detection disabled (needs SDL >= 2.0.14)")
#endif
	return "english";
}

/*
================
LOC_Language_f

Called when language changes
================
*/
void LOC_Language_f (cvar_t *cvar)
{
	if (!q_strcasecmp (cvar->string, "auto"))
		userlang = LOC_GetSystemLanguage ();
	else
		userlang = cvar->string;
	LOC_Load ();
}

/*
================
LOC_LanguageCompletion_f
================
*/
void LOC_LanguageCompletion_f (cvar_t *cvar, const char *partial)
{
	size_t i;

	for (i = 0; i < countof (knownlangs); i++)
		Con_AddToTabList (knownlangs[i][1], partial, NULL);
}

/*
================
LOC_Init
================
*/
void LOC_Init(void)
{
	Con_Printf("\nLanguage initialization\n");

	Cvar_RegisterVariable (&language);
	Cvar_SetCallback (&language, LOC_Language_f);
	Cvar_SetCompletion (&language, LOC_LanguageCompletion_f);
	language.callback (&language);
}

/*
================
LOC_Shutdown
================
*/
void LOC_Shutdown(void)
{
	free(localization.indices);
	free(localization.entries);
	free(localization.text);
}

/*
================
LOC_GetRawString

Returns localized string if available, or NULL otherwise
================
*/
const char* LOC_GetRawString (const char *key)
{
	unsigned pos, end;

	if (!localization.numindices || !key || !*key || *key != '$')
		return NULL;
	key++;

	pos = COM_HashString(key) % localization.numindices;
	end = pos;

	do
	{
		unsigned idx = localization.indices[pos];
		locentry_t *entry;
		if (!idx)
			return NULL;

		entry = &localization.entries[idx - 1];
		if (!Q_strcmp(entry->key, key))
			return entry->value;

		++pos;
		if (pos == localization.numindices)
			pos = 0;
	} while (pos != end);

	return NULL;
}

/*
================
LOC_GetString

Returns localized string if available, or input string otherwise
================
*/
const char* LOC_GetString (const char *key)
{
	const char* value = LOC_GetRawString(key);
	return value ? value : key;
}

/*
================
LOC_ParseArg

Returns argument index (>= 0) and advances the string if it starts with a placeholder ({} or {N}),
otherwise returns a negative value and leaves the pointer unchanged
================
*/
static int LOC_ParseArg (const char **pstr)
{
	int arg;
	const char *str = *pstr;

	// opening brace
	if (*str != '{')
		return -1;
	++str;

	// optional index, defaulting to 0
	arg = 0;
	while (q_isdigit(*str))
		arg = arg * 10 + *str++ - '0';

	// closing brace
	if (*str != '}')
		return -1;
	*pstr = ++str;

	return arg;
}

/*
================
LOC_HasPlaceholders
================
*/
qboolean LOC_HasPlaceholders (const char *str)
{
	if (!localization.numindices)
		return false;
	while (*str)
	{
		if (LOC_ParseArg(&str) >= 0)
			return true;
		str++;
	}
	return false;
}

/*
================
LOC_Format

Replaces placeholders (of the form {} or {N}) with the corresponding arguments

Returns number of written chars, excluding the NUL terminator
If len > 0, output is always NUL-terminated
================
*/
size_t LOC_Format (const char *format, const char* (*getarg_fn) (int idx, void* userdata), void* userdata, char* out, size_t len)
{
	size_t written = 0;
	int numargs = 0;

	if (!len)
	{
		Con_DPrintf("LOC_Format: no output space\n");
		return 0;
	}
	--len; // reserve space for the terminator

	while (*format && written < len)
	{
		const char* insert;
		size_t space_left;
		size_t insert_len;
		int argindex = LOC_ParseArg(&format);

		if (argindex < 0)
		{
			out[written++] = *format++;
			continue;
		}

		insert = getarg_fn(argindex, userdata);
		space_left = len - written;
		insert_len = Q_strlen(insert);

		if (insert_len > space_left)
		{
			Con_DPrintf("LOC_Format: overflow at argument #%d\n", numargs);
			insert_len = space_left;
		}

		Q_memcpy(out + written, insert, insert_len);
		written += insert_len;
	}

	if (*format)
		Con_DPrintf("LOC_Format: overflow\n");

	out[written] = 0;

	return written;
}

/*
============================================================================
								UNICODE
============================================================================
*/

static const uint32_t 	utf8_maxcode[4] = {0x7F, 0x7FF, 0xFFFF, 0x10FFFF}; // 1/2/3/4 bytes

static char				unicode_translit[65536][2];
static qboolean			unicode_translit_init = false;

static const uint32_t qchar_to_unicode[256] =
{/*     0       1       2       3       4       5       6       7       8       9       10      11      12      13      14      15
      ----------------------------------------------------------------------------------------------------------------------------------
  0 */  0x00B7, 0,      0,      0,      0,      0x00B7, 0,      0,      0,      0,      '\n',   0x25A0, ' ',    0x25B6, 0x00B7, 0x00B7, /*
  1 */  0x301A, 0x301B, '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    0x00B7, '-',    '-',    '-',    /*
  2 */  ' ',    '!',    '"',    '#',    '$',    '%',    '&',    '\'',   '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',    /*
  3 */  '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',    /*
  4 */  '@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',    /*
  5 */  'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',    'X',    'Y',    'Z',    '[',    '\\',   ']',    '^',    '_',    /*
  6 */  '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',    /*
  7 */  'p',    'q',    'r',    's',    't',    'u',    'v',    'w',    'x',    'y',    'z',    '{',    '|',    '}',    '~',    0x2190, /*

  8 */  '-',    '-',    '-',    '-',    0,      0x2022, 0,      0,      0,      0,      '\n',   0x25A0, ' ',    0x25B6, 0x2022, 0x2022, /*
  9 */  0x301A, 0x301B, '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    0x2022, '-',    '-',    '-',    /*
 10 */  ' ',    '!',    '"',    '#',    '$',    '%',    '&',    '\'',   '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',    /*
 11 */  '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',    /*
 12 */  '@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',    /*
 13 */  'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',    'X',    'Y',    'Z',    '[',    '\\',   ']',    '^',    '_',    /*
 14 */  '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',    /*
 15 */  'p',    'q',    'r',    's',    't',    'u',    'v',    'w',    'x',    'y',    'z',    '{',    '|',    '}',    '~',    0x2190, /*
      ----------------------------------------------------------------------------------------------------------------------------------
*/};

/*
==================
UTF8_CodePointLength

Returns the number of bytes needed to encode the codepoint
using UTF-8 (max 4), or 0 for an invalid code point
==================
*/
size_t UTF8_CodePointLength (uint32_t codepoint)
{
	if (codepoint < 0x80)
		return 1;

	if (codepoint < 0x800)
		return 2;

	if (codepoint < 0x10000)
		return 3;

	if (codepoint < 0x110000)
		return 4;

	return 0;
}

/*
==================
UTF8_WriteCodePoint

Writes a single Unicode code point using UTF-8

Returns the number of bytes written (up to 4),
or 0 on error (overflow or invalid code point)
==================
*/
size_t UTF8_WriteCodePoint (char *dst, size_t maxbytes, uint32_t codepoint)
{
	if (!maxbytes)
		return 0;

	if (codepoint < 0x80)
	{
		dst[0] = (char)codepoint;
		return 1;
	}

	if (codepoint < 0x800)
	{
		if (maxbytes < 2)
			return 0;
		dst[0] = 0xC0 | (codepoint >> 6);
		dst[1] = 0x80 | (codepoint & 63);
		return 2;
	}

	if (codepoint < 0x10000)
	{
		if (maxbytes < 3)
			return 0;
		dst[0] = 0xE0 | (codepoint >> 12);
		dst[1] = 0x80 | ((codepoint >> 6) & 63);
		dst[2] = 0x80 | (codepoint & 63);
		return 3;
	}

	if (codepoint < 0x110000)
	{
		if (maxbytes < 4)
			return 0;
		dst[0] = 0xF0 | (codepoint >> 18);
		dst[1] = 0x80 | ((codepoint >> 12) & 63);
		dst[2] = 0x80 | ((codepoint >> 6) & 63);
		dst[3] = 0x80 | (codepoint & 63);
		return 4;
	}

	return 0;
}

/*
==================
UTF8_ReadCodePoint

Reads at most 6 bytes from *src and advances the pointer

Returns 32-bit codepoint, or UNICODE_UNKNOWN on error
==================
*/
uint32_t UTF8_ReadCodePoint (const char **src)
{
	const char	*text = *src;
	uint32_t	code, mask, i;
	uint8_t		first, cont;
	
	first = text[0];
	if (!first)
		return 0;

	if (first < 128)
	{
		*src = text + 1;
		return first;
	}

	if ((first & 0xC0) != 0xC0)
	{
		*src = text + 1;
		return UNICODE_UNKNOWN;
	}

	mask = first << 1;
	code = 0;
	for (i = 1; i < 6 && (mask & 0x80) != 0; i++, mask <<= 1)
	{
		cont = text[i];
		if (!cont)
		{
			*src = text + i;
			return UNICODE_UNKNOWN;
		}
		if ((cont & 0x80) != 0x80)
		{
			*src = text + i + 1;
			return UNICODE_UNKNOWN;
		}
		code = (code << 6) | (cont & 63);
	}

	mask = ((1 << (7 - i)) - 1);
	code |= (first & mask) << (6 * (i - 1));
	*src = text + i;

	if (code > UNICODE_MAX			||	// out of range
		i > 4						||	// out of range/overlong
		code > utf8_maxcode[i - 1]	||	// overlong
		code - 0xD800 < 1024)			// surrogate
	{
		code = UNICODE_UNKNOWN;
	}

	return code;
}

/*
==================
UTF8_FromQuake

Converts a string from Quake encoding to UTF-8

Returns the number of written characters (including the NUL terminator)
if a valid output buffer is provided (dst is non-NULL, maxbytes > 0),
or the total amount of space necessary to encode the entire src string
if dst is NULL and maxbytes is 0.
==================
*/
size_t UTF8_FromQuake (char *dst, size_t maxbytes, const char *src)
{
	size_t i, j, written;

	if (!maxbytes)
	{
		if (dst)
			return 0; // error
		for (i = 0, j = 0; src[i]; i++)
		{
			uint32_t codepoint = qchar_to_unicode[(unsigned char) src[i]];
			if (codepoint)
				j += UTF8_CodePointLength (codepoint);
		}
		return j + 1; // include terminator
	}

	--maxbytes;

	for (i = 0, j = 0; j < maxbytes && src[i]; i++)
	{
		uint32_t codepoint = qchar_to_unicode[(unsigned char) src[i]];
		if (!codepoint)
			continue;
		written = UTF8_WriteCodePoint (dst + j, maxbytes - j, codepoint);
		if (!written)
			break;
		j += written;
	}

	dst[j++] = '\0';

	return j;
}

/*
==================
UTF8_ToQuake

Transliterates a string from UTF-8 to Quake encoding

Returns the number of written characters (including the NUL terminator)
if a valid output buffer is provided (dst is non-NULL, maxbytes > 0),
or the total amount of space necessary to encode the entire src string
if dst is NULL and maxbytes is 0.

Note: only single-character and two-character transliterations are used
so that the string is never expanded during the process.
==================
*/
size_t UTF8_ToQuake (char *dst, size_t maxbytes, const char *src)
{
	size_t i, j;
	uint32_t cp;

	if (!unicode_translit_init)
	{
		// precomputed single-character/two-character transliterations
		for (i = 0; i < countof (unicode_translit_src); i++)
		{
			unicode_translit[unicode_translit_src[i].code][0] = unicode_translit_src[i].remap[0];
			unicode_translit[unicode_translit_src[i].code][1] = unicode_translit_src[i].remap[1];
		}

		// Quake-specific characters: we process the list in reverse order
		// so that codepoints used for both colored and non-colored qchars
		// end up being remapped to the non-colored versions
		// Note: 0 is not included
		for (i = countof (qchar_to_unicode) - 1; i > 0; i--)
		{
			if (qchar_to_unicode[i] >= 128 && qchar_to_unicode[i] < countof (unicode_translit))
			{
				unicode_translit[qchar_to_unicode[i]][0] = (char) i;
				unicode_translit[qchar_to_unicode[i]][1] = '\0';
			}
		}

		// map ASCII characters to themselves
		for (i = 0; i < 128; i++)
		{
			unicode_translit[i][0] = (char) i;
			unicode_translit[i][1] = '\0';
		}

		// Map all other characters to QCHAR_BOX (unknown character)
		for (i = 0; i < Q_COUNTOF (unicode_translit); i++)
		{
			if (!unicode_translit[i][0])
			{
				unicode_translit[i][0] = QCHAR_BOX;
				unicode_translit[i][1] = '\0';
			}
		}

		unicode_translit_init = true;
	}

	if (!maxbytes)
	{
		if (dst)
			return 0; // error

		// Determine necessary output buffer size
		for (i = 0, j = 0; *src; i++)
		{
			// ASCII fast path
			while (*src && (byte)*src < 0x80)
			{
				src++;
				j++;
			}

			if (!*src)
				break;

			// A codepoint maps to either one or two Quake characters
			cp = UTF8_ReadCodePoint (&src);
			if (cp < Q_COUNTOF (unicode_translit))
				j += unicode_translit[cp][1] != '\0' ? 2 : 1;
			else
				j++;
		}

		return j + 1; // include terminator
	}

	--maxbytes;

	for (i = 0; i < maxbytes && *src; i++)
	{
		// ASCII fast path
		while (*src && i < maxbytes && (byte)*src < 0x80)
			dst[i++] = *src++;

		if (!*src || i >= maxbytes)
			break;

		cp = UTF8_ReadCodePoint (&src);
		if (cp < countof (unicode_translit))
		{
			char c0 = unicode_translit[cp][0];
			char c1 = unicode_translit[cp][1];
			dst[i] = c0;
			if (c1 && i + 1 < maxbytes)
				dst[++i] = c1;
		}
		else
			dst[i] = QCHAR_BOX;
	}

	dst[i++] = '\0';

	return i;
}
