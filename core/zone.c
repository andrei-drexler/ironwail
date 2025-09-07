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
// zone.c

#include "quakedef.h"

#define	DYNAMIC_SIZE	(4 * 1024 * 1024) // ericw -- was 512KB (64-bit) / 384KB (32-bit)

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64

typedef struct memblock_s
{
	int	size;		// including the header and possibly tiny fragments
	int	tag;		// a tag of 0 is a free block
	int	id;		// should be ZONEID
	int	pad;		// pad to 64 bit boundary
	struct	memblock_s	*next, *prev;
} memblock_t;

typedef struct
{
	int		size;		// total bytes malloced, including header
	memblock_t	blocklist;	// start / end cap for linked list
	memblock_t	*rover;
} memzone_t;

void Cache_FreeLow (int new_low_hunk);


/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

static memzone_t	*mainzone;


/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr)
{
	memblock_t	*block, *other;

	if (!ptr)
		Sys_Error ("Z_Free: NULL pointer");

	block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
	if (block->id != ZONEID)
		Sys_Error ("Z_Free: freed a pointer without ZONEID");
	if (block->tag == 0)
		Sys_Error ("Z_Free: freed a freed pointer");

	block->tag = 0;		// mark as free

	other = block->prev;
	if (!other->tag)
	{	// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == mainzone->rover)
			mainzone->rover = other;
		block = other;
	}

	other = block->next;
	if (!other->tag)
	{	// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == mainzone->rover)
			mainzone->rover = block;
	}
}


static void *Z_TagMalloc (int size, int tag)
{
	int		extra;
	memblock_t	*start, *rover, *newblock, *base;

	if (!tag)
		Sys_Error ("Z_TagMalloc: tried to use a 0 tag");

//
// scan through the block list looking for the first free block
// of sufficient size
//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 7) & ~7;		// align to 8-byte boundary

	base = rover = mainzone->rover;
	start = base->prev;

	do
	{
		if (rover == start)	// scaned all the way around the list
			return NULL;
		if (rover->tag)
			base = rover = rover->next;
		else
			rover = rover->next;
	} while (base->tag || base->size < size);

//
// found a block big enough
//
	extra = base->size - size;
	if (extra >  MINFRAGMENT)
	{	// there will be a free fragment after the allocated block
		newblock = (memblock_t *) ((byte *)base + size );
		newblock->size = extra;
		newblock->tag = 0;			// free block
		newblock->prev = base;
		newblock->id = ZONEID;
		newblock->next = base->next;
		newblock->next->prev = newblock;
		base->next = newblock;
		base->size = size;
	}

	base->tag = tag;				// no longer a free block

	mainzone->rover = base->next;	// next allocation will start looking here

	base->id = ZONEID;

// marker for memory trash testing
	*(int *)((byte *)base + base->size - 4) = ZONEID;

	return (void *) ((byte *)base + sizeof(memblock_t));
}

/*
========================
Z_CheckHeap
========================
*/
static void Z_CheckHeap (void)
{
	memblock_t	*block;

	for (block = mainzone->blocklist.next ; ; block = block->next)
	{
		if (block->next == &mainzone->blocklist)
			break;			// all blocks have been hit
		if ( (byte *)block + block->size != (byte *)block->next)
			Sys_Error ("Z_CheckHeap: block size does not touch the next block");
		if ( block->next->prev != block)
			Sys_Error ("Z_CheckHeap: next block doesn't have proper back link");
		if (!block->tag && !block->next->tag)
			Sys_Error ("Z_CheckHeap: two consecutive free blocks");
	}
}


/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size)
{
	void	*buf;

	Z_CheckHeap ();	// DEBUG
	buf = Z_TagMalloc (size, 1);
	if (!buf)
		Sys_Error ("Z_Malloc: failed on allocation of %i bytes",size);
	Q_memset (buf, 0, size);

	return buf;
}

/*
========================
Z_Realloc
========================
*/
void *Z_Realloc(void *ptr, int size)
{
	int old_size;
	void *old_ptr;
	memblock_t *block;

	if (!ptr)
		return Z_Malloc (size);

	block = (memblock_t *) ((byte *) ptr - sizeof (memblock_t));
	if (block->id != ZONEID)
		Sys_Error ("Z_Realloc: realloced a pointer without ZONEID");
	if (block->tag == 0)
		Sys_Error ("Z_Realloc: realloced a freed pointer");

	old_size = block->size;
	old_size -= (4 + (int)sizeof(memblock_t));	/* see Z_TagMalloc() */
	old_ptr = ptr;

	Z_Free (ptr);
	ptr = Z_TagMalloc (size, 1);
	if (!ptr)
		Sys_Error ("Z_Realloc: failed on allocation of %i bytes", size);

	if (ptr != old_ptr)
		memmove (ptr, old_ptr, q_min(old_size, size));
	if (old_size < size)
		memset ((byte *)ptr + old_size, 0, size - old_size);

	return ptr;
}

char *Z_Strdup (const char *s)
{
	size_t sz = strlen(s) + 1;
	char *ptr = (char *) Z_Malloc (sz);
	memcpy (ptr, s, sz);
	return ptr;
}


/*
========================
Z_Print
========================
*/
void Z_Print (memzone_t *zone)
{
	memblock_t	*block;

	Con_Printf ("zone size: %i  location: %p\n",mainzone->size,mainzone);

	for (block = zone->blocklist.next ; ; block = block->next)
	{
		Con_Printf ("block:%p    size:%7i    tag:%3i\n",
			block, block->size, block->tag);

		if (block->next == &zone->blocklist)
			break;			// all blocks have been hit
		if ( (byte *)block + block->size != (byte *)block->next)
			Con_Printf ("ERROR: block size does not touch the next block\n");
		if ( block->next->prev != block)
			Con_Printf ("ERROR: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Con_Printf ("ERROR: two consecutive free blocks\n");
	}
}


//============================================================================

#define	HUNK_SENTINEL	0x1df001ed

#define HUNKNAME_LEN	24
typedef struct
{
	int		sentinel;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[HUNKNAME_LEN];
} hunk_t;

typedef struct hunkseg_s
{
	int					base;
	int					size;
	int					used;
	int					pad; // pad to power of 2
} hunkseg_t;

#define MAX_SEGMENTS	8
#define SEG_MEM(seg)	((byte *) ((seg) + 1))
#define LASTSEG			(hunk_segments[hunk_numsegments-1])

static int				hunk_low_used;
static int				hunk_numsegments;
static hunkseg_t		*hunk_segments[MAX_SEGMENTS];

typedef enum
{
	HF_UNINIT			= 0,
	HF_CLEAR			= 1 << 0,
} hunkflags_t;


/*
===================
Hunk_Size
===================
*/
static int Hunk_Size (void)
{
	return LASTSEG->base + LASTSEG->size;
}


/*
===================
Hunk_GetName
===================
*/
static const char *Hunk_GetName (const hunk_t *hunk)
{
	return hunk->name[0] ? hunk->name : "unknown";
}

/*
==============
Hunk_Check

Run consistency and sentinel trashing checks
==============
*/
void Hunk_Check (void)
{
	int i, ofs;

	for (i = 0; i < hunk_numsegments && hunk_segments[i]->base < hunk_low_used; i++)
	{
		const hunkseg_t *seg = hunk_segments[i];
		for (ofs = 0; ofs < seg->used; )
		{
			const hunk_t *h = (const hunk_t *) (SEG_MEM (seg) + ofs);
			if (h->sentinel != HUNK_SENTINEL)
				Sys_Error ("Hunk_Check: trashed sentinel");
			if (h->size < (int) sizeof(hunk_t) || h->size + ofs > seg->size)
				Sys_Error ("Hunk_Check: bad size");
			ofs += h->size;
		}
	}
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void Hunk_Print (qboolean all)
{
	int i, count, sum, totalblocks;

	count = 0;
	sum = 0;
	totalblocks = 0;

	Con_SafePrintf ("\n");

	// print segments if more than 1
	if (hunk_numsegments > 1)
	{
		Con_SafePrintf ("             Segments\n");
		Con_SafePrintf ("---------------------------------\n");
		Con_SafePrintf ("id :     offset :       size\n");
		Con_SafePrintf ("---------------------------------\n");
		for (i = 0; i < hunk_numsegments; i++)
			Con_SafePrintf ("%2i : %10i : %10i\n", i, hunk_segments[i]->base, hunk_segments[i]->size);
		Con_SafePrintf ("---------------------------------\n");
		Con_SafePrintf ("\n");
		Con_SafePrintf ("           Allocations\n");
		Con_SafePrintf ("---------------------------------\n");
	}

	if (all)
		Con_SafePrintf ("    offset :       size : name\n");
	else
		Con_SafePrintf ("allocs :       size : name\n");
	Con_SafePrintf ("---------------------------------\n");

	for (i = 0; i < hunk_numsegments; i++)
	{
		const hunkseg_t *seg = hunk_segments[i];
		if (seg->base < hunk_low_used)
		{
			int ofs;

			for (ofs = 0; ofs < seg->used; )
			{
				const hunk_t *h, *next;

				h = (const hunk_t *) (SEG_MEM (seg) + ofs);

				// if this is the last block in the segment, then the next block is either
				// the first block of the next segment, or NULL if this is the last segment
				if (ofs + h->size == seg->used)
					next = i != hunk_numsegments - 1 ? (const hunk_t *) SEG_MEM (hunk_segments[i + 1]) : NULL;
				else // at least 1 more block in the current segment
					next = (const hunk_t *) ((byte *)h + h->size);

				//
				// run consistency checks
				//
				if (h->sentinel != HUNK_SENTINEL)
					Sys_Error ("Hunk_Check: trashed sentinel");
				if (h->size < (int) sizeof(hunk_t) || h->size + ofs > seg->size)
					Sys_Error ("Hunk_Check: bad size");

				count++;
				totalblocks++;
				sum += h->size;

				//
				// print the single block
				//
				if (all)
					Con_SafePrintf ("%10i : %10i : %s\n", seg->base + ofs, h->size, Hunk_GetName (h));

				//
				// print the total
				//
				if (!next || strncmp (Hunk_GetName (h), Hunk_GetName (next), HUNKNAME_LEN - 1) != 0)
				{
					if (!all)
						Con_SafePrintf ("%6i : %10i : %s\n", count, sum, Hunk_GetName (h));
					count = 0;
					sum = 0;
				}

				ofs += h->size;
			}
		}
	}

	Con_SafePrintf ("---------------------------------\n");

	if (all)
	{
		Con_SafePrintf ("%10s   %10i   USED (%d alloc%s)\n", "", hunk_low_used, PLURAL (totalblocks));
		Con_SafePrintf ("%10s   %10i   REMAINING\n", "", Hunk_Size () - hunk_low_used);
		Con_SafePrintf ("%10s   %10i   TOTAL\n", "", Hunk_Size ());
	}
	else
	{
		Con_SafePrintf ("%6i : %10i : USED\n", totalblocks, hunk_low_used);
		Con_SafePrintf ("%6s : %10i : REMAINING\n", "", Hunk_Size () - hunk_low_used);
		Con_SafePrintf ("%6s : %10i : TOTAL\n", "", Hunk_Size ());
	}
}

/*
===================
Hunk_Print_f -- johnfitz -- console command to call hunk_print
===================
*/
void Hunk_Print_f (void)
{
	Hunk_Print (false);
}

/*
===================
Hunk_SegForOfs
===================
*/
static int Hunk_SegForOfs (int ofs)
{
	int i;

	for (i = hunk_numsegments - 1; i >= 0; i--)
	{
		const hunkseg_t *seg = hunk_segments[i];
		if (seg->base <= ofs && ofs < seg->base + seg->size)
			return i;
	}

	Sys_Error ("Hunk_SegForOfs: bad offset %d (max: %d)", ofs, Hunk_Size ());

	return -1;
}

/*
===================
Hunk_SegForPtr
===================
*/
static int Hunk_SegForPtr (const void *ptr)
{
	int i;

	for (i = hunk_numsegments - 1; i >= 0; i--)
	{
		const hunkseg_t *seg = hunk_segments[i];
		const byte *begin = SEG_MEM (seg);
		const byte *end = begin + seg->size;
		if (PTR_IN_RANGE (ptr, begin, end))
			return i;
	}

	Sys_Error ("Hunk_SegForPtr: bad pointer");

	return -1;
}


/*
===================
Hunk_AllocInternal
===================
*/
static void *Hunk_AllocInternal (int size, const char *name, hunkflags_t flags)
{
	hunkseg_t	*seg;
	hunk_t		*h;
	int			i;

#ifdef PARANOID
	Hunk_Check ();
#endif

	if (size == 0)
		return NULL;

	if (size < 0)
		Sys_Error ("Hunk_Alloc: bad size: %i", size);

	size = sizeof(hunk_t) + ((size+15)&~15);

	i = Hunk_SegForOfs (hunk_low_used);

	// skip segments that can't handle this request (adjusting hunk_low_used)
	while (i < hunk_numsegments && (hunk_low_used - hunk_segments[i]->base) + size > hunk_segments[i]->size)
	{
		hunk_low_used = hunk_segments[i]->base + hunk_segments[i]->size;
		i++;
	}

	// add new segment if we've reached the end
	if (i == hunk_numsegments)
	{
		int newbase, newsize;

		if (hunk_numsegments == MAX_SEGMENTS)
			Sys_Error ("Hunk_Alloc: segment overflow");

		Cache_Flush ();

		newbase = LASTSEG->base + LASTSEG->size;
		newsize = LASTSEG->size * 2;
		newsize = q_max (newsize, size);

		Sys_Printf ("Allocating new hunk segment: %.2lf MiB\n", newsize / 1048576.0);

		seg = (hunkseg_t *) malloc (sizeof (hunkseg_t) + newsize);
		if (!seg)
		{
			Sys_Error ("Hunk_Alloc: failed on %i bytes", size);
			return NULL;
		}

		seg->base = newbase;
		seg->size = newsize;
		seg->used = 0;

		hunk_segments[hunk_numsegments++] = seg;
		hunk_low_used = seg->base;
	}

	seg = hunk_segments[i];
	h = (hunk_t *) (SEG_MEM (seg) + hunk_low_used - seg->base);
	hunk_low_used += size;
	seg->used = hunk_low_used - seg->base;

	Cache_FreeLow (hunk_low_used);

	if (flags & HF_CLEAR)
		memset (h, 0, size);

	h->size = size;
	h->sentinel = HUNK_SENTINEL;
	if (name)
		q_strlcpy (h->name, name, HUNKNAME_LEN);
	else
		h->name[0] = '\0';

	return (void *)(h+1);
}

/*
===================
Hunk_AllocName
===================
*/
void *Hunk_AllocName (int size, const char *name)
{
	return Hunk_AllocInternal (size, name, HF_CLEAR);
}

/*
===================
Hunk_AllocNameNoFill
===================
*/
void *Hunk_AllocNameNoFill (int size, const char *name)
{
	return Hunk_AllocInternal (size, name, HF_UNINIT);
}

/*
===================
Hunk_Alloc
===================
*/
void *Hunk_Alloc (int size)
{
	return Hunk_AllocName (size, NULL);
}

/*
===================
Hunk_AllocNoFill
===================
*/
void *Hunk_AllocNoFill (int size)
{
	return Hunk_AllocNameNoFill (size, NULL);
}

int	Hunk_LowMark (void)
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark (int mark)
{
	int i;

	if (mark < 0 || mark > hunk_low_used)
		Sys_Error ("Hunk_FreeToLowMark: bad mark %i", mark);

	hunk_low_used = mark;
	for (i = Hunk_SegForOfs (hunk_low_used); i < hunk_numsegments; i++)
		hunk_segments[i]->used = q_max (0, hunk_low_used - hunk_segments[i]->base);
}

char *Hunk_Strdup (const char *s, const char *name)
{
	size_t sz = strlen(s) + 1;
	char *ptr = (char *) Hunk_AllocNameNoFill (sz, name);
	memcpy (ptr, s, sz);
	return ptr;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

#define CACHENAME_LEN	32
typedef struct cache_system_s
{
	int			size;		// including this header
	cache_user_t		*user;
	char			name[CACHENAME_LEN];
	struct cache_system_s	*prev, *next;
	struct cache_system_s	*lru_prev, *lru_next;	// for LRU flushing
} cache_system_t;

cache_system_t *Cache_TryAlloc (int size, qboolean nobottom);

cache_system_t	cache_head;

/*
===========
Cache_Move
===========
*/
void Cache_Move ( cache_system_t *c)
{
	cache_system_t		*new_cs;

// we are clearing up space at the bottom, so only allocate it late
	new_cs = Cache_TryAlloc (c->size, true);
	if (new_cs)
	{
//		Con_Printf ("cache_move ok\n");

		Q_memcpy ( new_cs+1, c+1, c->size - sizeof(cache_system_t) );
		new_cs->user = c->user;
		Q_memcpy (new_cs->name, c->name, sizeof(new_cs->name));
		Cache_Free (c->user, false); //johnfitz -- added second argument
		new_cs->user->data = (void *)(new_cs+1);
	}
	else
	{
//		Con_Printf ("cache_move failed\n");

		Cache_Free (c->user, true); // tough luck... //johnfitz -- added second argument
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeLow (int new_low_hunk)
{
	cache_system_t	*c;
	hunkseg_t		*seg;
	int				ofs;

	// can only allocate space in the last segment
	new_low_hunk = q_max (new_low_hunk, LASTSEG->base);

	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return;		// nothing in cache at all
		seg = hunk_segments[Hunk_SegForPtr (c)];
		ofs = (byte *) (c) - SEG_MEM (seg);
		if (ofs + seg->base >= new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move ( c );	// reclaim the space
	}
}

void Cache_UnlinkLRU (cache_system_t *cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		Sys_Error ("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

void Cache_MakeLRU (cache_system_t *cs)
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error ("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
cache_system_t *Cache_TryAlloc (int size, qboolean nobottom)
{
	cache_system_t	*cs, *new_cs;
	int ofs = q_max (hunk_low_used, LASTSEG->base);

// is the cache completely empty?
	if (!nobottom && cache_head.prev == &cache_head)
	{
		if ((ofs - LASTSEG->base) + size > LASTSEG->size)
			Sys_Error ("Cache_TryAlloc: %i is greater then free hunk", size);

		new_cs = (cache_system_t *) (SEG_MEM (LASTSEG) + ofs - LASTSEG->base);
		memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		cache_head.prev = cache_head.next = new_cs;
		new_cs->prev = new_cs->next = &cache_head;

		Cache_MakeLRU (new_cs);
		return new_cs;
	}

// search from the bottom up for space

	new_cs = (cache_system_t *) (SEG_MEM (LASTSEG) + ofs - LASTSEG->base);
	cs = cache_head.next;

	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ( (byte *)cs - (byte *)new_cs >= size)
			{	// found space
				memset (new_cs, 0, sizeof(*new_cs));
				new_cs->size = size;

				new_cs->next = cs;
				new_cs->prev = cs->prev;
				cs->prev->next = new_cs;
				cs->prev = new_cs;

				Cache_MakeLRU (new_cs);

				return new_cs;
			}
		}

	// continue looking
		new_cs = (cache_system_t *)((byte *)cs + cs->size);
		cs = cs->next;

	} while (cs != &cache_head);

// try to allocate one at the very end
	if ((byte *)new_cs - SEG_MEM (LASTSEG) + size <= LASTSEG->size)
	{
		memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		new_cs->next = &cache_head;
		new_cs->prev = cache_head.prev;
		cache_head.prev->next = new_cs;
		cache_head.prev = new_cs;

		Cache_MakeLRU (new_cs);

		return new_cs;
	}

	return NULL;		// couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
void Cache_Flush (void)
{
	while (cache_head.next != &cache_head)
		Cache_Free ( cache_head.next->user, true); // reclaim the space //johnfitz -- added second argument
}

/*
============
Cache_Print

============
*/
void Cache_Print (void)
{
	cache_system_t	*cd;

	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		Con_Printf ("%8i : %s\n", cd->size, cd->name);
	}
}

/*
============
Cache_Report

============
*/
void Cache_Report (void)
{
	Con_DPrintf ("%4.1f megabyte data cache\n", (Hunk_Size () - hunk_low_used) / (float)(1024*1024) );
}

/*
============
Cache_Init

============
*/
void Cache_Init (void)
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;

	Cmd_AddCommand ("flush", Cache_Flush);
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free (cache_user_t *c, qboolean freetextures) //johnfitz -- added second argument
{
	cache_system_t	*cs;

	if (!c->data)
		Sys_Error ("Cache_Free: not allocated");

	cs = ((cache_system_t *)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU (cs);

	//johnfitz -- if a model becomes uncached, free the gltextures.  This only works
	//becuase the cache_user_t is the last component of the qmodel_t struct.  Should
	//fail harmlessly if *c is actually part of an sfx_t struct.  I FEEL DIRTY
	if (freetextures)
		TexMgr_FreeTexturesForOwner ((qmodel_t *)(c + 1) - 1);
}



/*
==============
Cache_Check
==============
*/
void *Cache_Check (cache_user_t *c)
{
	cache_system_t	*cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *)c->data) - 1;

// move to head of LRU
	Cache_UnlinkLRU (cs);
	Cache_MakeLRU (cs);

	return c->data;
}


/*
==============
Cache_Alloc
==============
*/
void *Cache_Alloc (cache_user_t *c, int size, const char *name)
{
	cache_system_t	*cs;

	if (c->data)
		Sys_Error ("Cache_Alloc: already allocated");

	if (size <= 0)
		Sys_Error ("Cache_Alloc: size %i", size);

	size = (size + sizeof(cache_system_t) + 15) & ~15;

// find memory for it
	while (1)
	{
		cs = Cache_TryAlloc (size, false);
		if (cs)
		{
			q_strlcpy (cs->name, name, CACHENAME_LEN);
			c->data = (void *)(cs+1);
			cs->user = c;
			break;
		}

	// free the least recently used cahedat
		if (cache_head.lru_prev == &cache_head)
			Sys_Error ("Cache_Alloc: out of memory"); // not enough memory at all

		Cache_Free (cache_head.lru_prev->user, true); //johnfitz -- added second argument
	}

	return Cache_Check (c);
}

//============================================================================


static void Memory_InitZone (memzone_t *zone, int size)
{
	memblock_t	*block;

// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
		(memblock_t *)( (byte *)zone + sizeof(memzone_t) );
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}

/*
========================
Memory_Init
========================
*/
void Memory_Init (void *buf, int size)
{
	int p;
	int zonesize = DYNAMIC_SIZE;

	hunk_segments[0] = (hunkseg_t *) buf;
	hunk_segments[0]->base = 0;
	hunk_segments[0]->size = size - sizeof (hunkseg_t);
	hunk_numsegments = 1;
	hunk_low_used = 0;

	Cache_Init ();
	p = COM_CheckParm ("-zone");
	if (p)
	{
		if (p < com_argc-1)
			zonesize = Q_atoi (com_argv[p+1]) * 1024;
		else
			Sys_Error ("Memory_Init: you must specify a size in KB after -zone");
	}
	mainzone = (memzone_t *) Hunk_AllocName (zonesize, "zone" );
	Memory_InitZone (mainzone, zonesize);

	Cmd_AddCommand ("hunk_print", Hunk_Print_f); //johnfitz
}

