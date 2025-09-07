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

#ifndef __ZZONE_H
#define __ZZONE_H

/*
 memory allocation


H_??? The hunk manages the memory given to Quake.
Memory can be allocated from the low end in a stack fashion. 

The only way memory is released is by resetting the hunk cursor.

The hunk starts as a single continguous segment, but new ones can be added
if an allocation request cannot be serviced.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.
Hunk allocations are guaranteed to be 16 byte aligned.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.
Cache memory is always allocated from the top hunk segment.



------ Top of Memory -------

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/

void Memory_Init (void *buf, int size);

#ifdef __cplusplus
extern "C" {
#endif
void Z_Free (void *ptr);
void *Z_Malloc (int size);			// returns 0 filled memory
void *Z_Realloc (void *ptr, int size);
char *Z_Strdup (const char *s);
#ifdef __cplusplus
}
#endif

void *Hunk_Alloc (int size); // returns 0 filled memory
void *Hunk_AllocNoFill (int size); // returns uninitialized memory
void *Hunk_AllocName (int size, const char *name); // returns 0 filled memory
void *Hunk_AllocNameNoFill (int size, const char *name); // returns uninitialized memory
char *Hunk_Strdup (const char *s, const char *name);

int	Hunk_LowMark (void);
void Hunk_FreeToLowMark (int mark);

void Hunk_Check (void);

typedef struct cache_user_s
{
	void	*data;
} cache_user_t;

void Cache_Flush (void);

void *Cache_Check (cache_user_t *c);
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void Cache_Free (cache_user_t *c, qboolean freetextures); //johnfitz -- added second argument

void *Cache_Alloc (cache_user_t *c, int size, const char *name);
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.

void Cache_Report (void);

#endif	/* __ZZONE_H */

