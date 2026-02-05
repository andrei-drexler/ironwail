/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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

// draw.c -- 2d drawing

#include "quakedef.h"

const vec3_t	rgb_black = {0.f, 0.f, 0.f};
const vec3_t	rgb_white = {1.f, 1.f, 1.f};

cvar_t		scr_conalpha = {"scr_conalpha", "0.5", CVAR_ARCHIVE}; //johnfitz
cvar_t		scr_conbrightness = {"scr_conbrightness", "1.0", CVAR_ARCHIVE};

qpic_t		*draw_disc;
qpic_t		*draw_backtile;
qboolean	custom_conchars;

gltexture_t *char_texture; //johnfitz
byte		char_texture_data[256 * 10 * 10];
qpic_t		*pic_ovr, *pic_ins; //johnfitz -- new cursor handling
qpic_t		*pic_nul; //johnfitz -- for missing gfx, don't crash

//johnfitz -- new pics
byte pic_ovr_data[8][8] =
{
	{255,255,255,255,255,255,255,255},
	{255, 15, 15, 15, 15, 15, 15,255},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255,255,  2,  2,  2,  2,  2,  2},
};

byte pic_ins_data[9][8] =
{
	{ 15, 15,255,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{255,  2,  2,255,255,255,255,255},
};

byte pic_nul_data[8][8] =
{
	{252,252,252,252,  0,  0,  0,  0},
	{252,252,252,252,  0,  0,  0,  0},
	{252,252,252,252,  0,  0,  0,  0},
	{252,252,252,252,  0,  0,  0,  0},
	{  0,  0,  0,  0,252,252,252,252},
	{  0,  0,  0,  0,252,252,252,252},
	{  0,  0,  0,  0,252,252,252,252},
	{  0,  0,  0,  0,252,252,252,252},
};

byte pic_stipple_data[8][8] =
{
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
};

byte pic_crosshair_data[8][8] =
{
	{255,255,255,255,255,255,255,255},
	{255,255,255,  8,  9,255,255,255},
	{255,255,255,  6,  8,  2,255,255},
	{255,  6,  8,  8,  6,  8,  8,255},
	{255,255,  2,  8,  8,  2,  2,  2},
	{255,255,255,  7,  8,  2,255,255},
	{255,255,255,255,  2,  2,255,255},
	{255,255,255,255,255,255,255,255},
};
//johnfitz

typedef struct
{
	gltexture_t *gltexture;
	float		sl, tl, sh, th;
} glpic_t;

typedef struct guivertex_t {
	float		pos[2];
	float		uv[2];
	byte		color[4];
} guivertex_t;

#define MAX_BATCH_QUADS 2048

static int numbatchquads = 0;
static guivertex_t batchverts[4 * MAX_BATCH_QUADS];
static GLushort batchindices[6 * MAX_BATCH_QUADS];

glcanvas_t glcanvas;

//==============================================================================
//
//  PIC CACHING
//
//==============================================================================

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	qpic_t		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		512	//Spike -- increased to avoid csqc issues.
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

//  scrap allocation
//  Allocate all the little status bar obejcts into a single texture
//  to crutch up stupid hardware / drivers

#define	SCRAP_BLOCKS_X		4
#define	SCRAP_BLOCKS_Y		1
#define	MAX_SCRAPS			(SCRAP_BLOCKS_X * SCRAP_BLOCKS_Y)
#define	BLOCK_WIDTH			256
#define	BLOCK_HEIGHT		1024
#define	SCRAP_ATLAS_WIDTH	(SCRAP_BLOCKS_X * BLOCK_WIDTH)
#define	SCRAP_ATLAS_HEIGHT	(SCRAP_BLOCKS_Y * BLOCK_HEIGHT)
#define	MAX_SCRAP_WIDTH		128
#define	MAX_SCRAP_HEIGHT	128

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[SCRAP_ATLAS_WIDTH*SCRAP_ATLAS_HEIGHT];
qboolean	scrap_dirty;
gltexture_t	*scrap_texture; //johnfitz
gltexture_t	*winquakemenubg;


/*
================
Scrap_AllocBlock
================
*/
qboolean Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	// padding
	w += 2;
	h += 2;

	if (w > MAX_SCRAP_WIDTH || h > MAX_SCRAP_HEIGHT)
		return false;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		*x += 1 + (texnum % SCRAP_BLOCKS_X) * BLOCK_WIDTH;
		*y += 1 + (texnum / SCRAP_BLOCKS_X) * BLOCK_HEIGHT;

		return true;
	}

	return false;
}

/*
================
Scrap_Upload -- johnfitz -- now uses TexMgr
================
*/
void Scrap_Upload (void)
{
	scrap_texture = TexMgr_LoadImage (NULL, "scrap", SCRAP_ATLAS_WIDTH, SCRAP_ATLAS_HEIGHT, SRC_INDEXED, scrap_texels,
		"", (src_offset_t)scrap_texels, TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP | TEXPREF_UNCOMPRESSED);
	scrap_dirty = false;
}

/*
================
Draw_FillClampTexels

Fills the given rectangle *and* a 1-pixel border around it
with the closest pixels from the source image (emulating UV clamping)
================
*/
void Draw_FillClampTexels (const byte *src, int w, int h, int srcpitch, byte *dst, int dstpitch)
{
	int i;
	for (i = -1; i <= h; i++, dst += dstpitch)
	{
		const byte *rowsrc = src + CLAMP (0, i, h - 1) * srcpitch;
		dst[0] = rowsrc[0];
		memcpy (dst + 1, rowsrc, w);
		dst[w + 1] = rowsrc[w - 1];
	}
}

/*
================
Scrap_FillTexels
================
*/
void Scrap_FillTexels (int x, int y, int w, int h, const byte *data)
{
	byte *dst = &scrap_texels[(y - 1) * SCRAP_ATLAS_WIDTH + x - 1];
	Draw_FillClampTexels (data, w, h, w, dst, SCRAP_ATLAS_WIDTH);
	scrap_dirty = true;
}

/*
================
Scrap_Compatible
================
*/
qboolean Scrap_Compatible (unsigned int texflags)
{
	unsigned int required = TEXPREF_PAD;
	unsigned int unsupported = TEXPREF_MIPMAP | TEXPREF_NEAREST | TEXPREF_LINEAR;
	return (texflags & required) == required && (texflags & unsupported) == 0;
}

/*
================
Draw_PicFromWad
================
*/
qpic_t *Draw_PicFromWad2 (const char *name, unsigned int texflags)
{
	int i, x, y;
	cachepic_t *pic;
	qpic_t	*p;
	glpic_t	gl;
	src_offset_t offset; //johnfitz
	lumpinfo_t *info;

	//Spike -- added cachepic stuff here, to avoid glitches if the function is called multiple times with the same image.
	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
	{
		if (!strcmp (name, pic->name))
			return &pic->pic;
	}
	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");

	p = (qpic_t *) W_GetLumpName (name, &info);
	if (!p)
	{
		Con_SafePrintf ("W_GetLumpName: %s not found\n", name);
		return pic_nul; //johnfitz
	}
	if (info->type != TYP_QPIC) {Con_SafePrintf ("Draw_PicFromWad: lump \"%s\" is not a qpic\n", name); return pic_nul;}
	if ((size_t)info->size < sizeof(int)*2) {Con_SafePrintf ("Draw_PicFromWad: pic \"%s\" is too small for its qpic header (%u bytes)\n", name, info->size); return pic_nul;}
	if ((size_t)info->size < sizeof(int)*2+p->width*p->height) {Con_SafePrintf ("Draw_PicFromWad: pic \"%s\" truncated (%u*%u requires %u at least bytes)\n", name, p->width,p->height, 8+p->width*p->height); return pic_nul;}
	if ((size_t)info->size > sizeof(int)*2+p->width*p->height) Con_DPrintf ("Draw_PicFromWad: pic \"%s\" over-sized (%u*%u requires only %u bytes)\n", name, p->width,p->height, 8+p->width*p->height);

	// load little ones into the scrap
	if (Scrap_Compatible (texflags) && Scrap_AllocBlock (p->width, p->height, &x, &y))
	{
		Scrap_FillTexels (x, y, p->width, p->height, p->data);

		gl.gltexture = scrap_texture; //johnfitz -- changed to an array
		//johnfitz -- no longer go from 0.01 to 0.99
		gl.sl = x/(float)SCRAP_ATLAS_WIDTH;
		gl.sh = (x+p->width)/(float)SCRAP_ATLAS_WIDTH;
		gl.tl = y/(float)SCRAP_ATLAS_HEIGHT;
		gl.th = (y+p->height)/(float)SCRAP_ATLAS_HEIGHT;
	}
	else
	{
		char texturename[64]; //johnfitz
		q_snprintf (texturename, sizeof(texturename), "%s:%s", WADFILENAME, name); //johnfitz

		offset = (src_offset_t)p - (src_offset_t)wad_base + sizeof(int)*2; //johnfitz

		gl.gltexture = TexMgr_LoadImage (NULL, texturename, p->width, p->height, SRC_INDEXED, p->data, WADFILENAME,
										  offset, texflags); //johnfitz -- TexMgr
		gl.sl = 0;
		gl.sh = (texflags&TEXPREF_PAD)?(float)p->width/(float)TexMgr_PadConditional(p->width):1; //johnfitz
		gl.tl = 0;
		gl.th = (texflags&TEXPREF_PAD)?(float)p->height/(float)TexMgr_PadConditional(p->height):1; //johnfitz
	}

	menu_numcachepics++;
	strcpy (pic->name, name);
	pic->pic = *p;
	memcpy (pic->pic.data, &gl, sizeof(glpic_t));

	return &pic->pic;
}

qpic_t *Draw_PicFromWad (const char *name)
{
	return Draw_PicFromWad2 (name, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP | TEXPREF_UNCOMPRESSED);
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_TryCachePic (const char *path, unsigned int texflags)
{
	cachepic_t	*pic;
	int			i, x, y;
	qpic_t		*dat;
	glpic_t		gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
	{
		if (!strcmp (path, pic->name))
			return &pic->pic;
	}
	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)QFS_LoadMallocFile (path, NULL, NULL);
	if (!dat)
		return NULL;
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep this as a separate texture
	// so that the menu configuration dialog can translate its colors
	// without affecting the whole scrap atlas
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		texflags &= ~TEXPREF_PAD; // no scrap usage

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	if (Scrap_Compatible (texflags) && Scrap_AllocBlock (dat->width, dat->height, &x, &y))
	{
		Scrap_FillTexels (x, y, dat->width, dat->height, dat->data);
		gl.gltexture = scrap_texture;
		gl.sl = x/(float)SCRAP_ATLAS_WIDTH;
		gl.tl = y/(float)SCRAP_ATLAS_HEIGHT;
		gl.sh = (x+dat->width)/(float)SCRAP_ATLAS_WIDTH;
		gl.th = (y+dat->height)/(float)SCRAP_ATLAS_HEIGHT;
	}
	else
	{
		gl.gltexture = TexMgr_LoadImage (NULL, path, dat->width, dat->height, SRC_INDEXED, dat->data, path,
										  sizeof(int)*2, texflags); //johnfitz -- TexMgr
		gl.sl = 0;
		gl.sh = (float)dat->width/(float)TexMgr_PadConditional(dat->width); //johnfitz
		gl.tl = 0;
		gl.th = (float)dat->height/(float)TexMgr_PadConditional(dat->height); //johnfitz
	}

	free (dat);
	memcpy (pic->pic.data, &gl, sizeof(glpic_t));

	return &pic->pic;
}

qpic_t	*Draw_CachePic (const char *path)
{
	qpic_t *pic = Draw_TryCachePic(path, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP | TEXPREF_CLAMP | TEXPREF_UNCOMPRESSED);
	if (!pic)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	return pic;
}

/*
================
Draw_MakePic -- johnfitz -- generate pics from internal data
================
*/
qpic_t *Draw_MakePic (const char *name, int width, int height, byte *data)
{
	int flags = TEXPREF_NEAREST | TEXPREF_ALPHA | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_PAD | TEXPREF_UNCOMPRESSED;
	qpic_t		*pic;
	glpic_t		gl;

	pic = (qpic_t *) Hunk_Alloc (sizeof(qpic_t) - 4 + sizeof (glpic_t));
	pic->width = width;
	pic->height = height;

	gl.gltexture = TexMgr_LoadImage (NULL, name, width, height, SRC_INDEXED, data, "", (src_offset_t)data, flags);
	gl.sl = 0;
	gl.sh = (float)width/(float)TexMgr_PadConditional(width);
	gl.tl = 0;
	gl.th = (float)height/(float)TexMgr_PadConditional(height);
	memcpy (pic->data, &gl, sizeof(glpic_t));

	return pic;
}

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
===============
Draw_LoadPics -- johnfitz
===============
*/
void Draw_LoadPics (void)
{
	lumpinfo_t	*info;
	byte		*data;
	int			i, row, col;

	data = (byte *) W_GetLumpName ("conchars", &info);
	if (!data)
		Sys_Error ("Draw_LoadPics: couldn't load conchars");
	if (info->disksize < 128*128)
		Sys_Error ("Draw_LoadPics: truncated conchars");

	custom_conchars = (COM_HashBlock (data, 128*218) != 0xc7e2a10a);

	for (i = 0; i < 256; i++)
	{
		row = i / 16;
		col = i % 16;
		Draw_FillClampTexels (data + row*(16*8*8) + col*8, 8, 8, 16*8,
			char_texture_data + row*(16*10*10) + col*10, 16*10);
	}
	char_texture = TexMgr_LoadImage (NULL, WADFILENAME":conchars", 16*10, 16*10, SRC_INDEXED, char_texture_data,
		"", (src_offset_t) char_texture_data, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS | TEXPREF_UNCOMPRESSED);

	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad2 ("backtile", TEXPREF_ALPHA | TEXPREF_NOPICMIP | TEXPREF_UNCOMPRESSED); // no pad flag to force separate allocation
}

/*
===============
Draw_NewGame -- johnfitz
===============
*/
void Draw_NewGame (void)
{
	cachepic_t	*pic;
	int			i;

	// empty scrap and reallocate gltextures
	memset(scrap_allocated, 0, sizeof(scrap_allocated));
	memset(scrap_texels, 255, sizeof(scrap_texels));

	Scrap_Upload (); //creates 2 empty gltextures

	// empty lmp cache
	for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
		pic->name[0] = 0;
	menu_numcachepics = 0;

	// reload wad pics
	W_LoadWadFile (); //johnfitz -- filename is now hard-coded for honesty
	Draw_LoadPics ();
	SCR_LoadPics ();
	Sbar_LoadPics ();
	PR_ReloadPics (false);
}

/*
===============
Draw_CreateWinQuakeMenuBgTex
===============
*/
static void Draw_CreateWinQuakeMenuBgTex (void)
{
	static unsigned winquakemenubg_pixels[4*2] =
	{
		0x00ffffffu, 0xff000000u, 0xff000000u, 0xff000000u,
		0xff000000u, 0xff000000u, 0x00ffffffu, 0xff000000u,
	};

	winquakemenubg = TexMgr_LoadImage (NULL, "winquakemenubg", 4, 2, SRC_RGBA,
		(byte*)winquakemenubg_pixels, "", (src_offset_t)winquakemenubg_pixels,
		TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_UNCOMPRESSED
	);
}

/*
===============
Draw_Init -- johnfitz -- rewritten
===============
*/
void Draw_Init (void)
{
	int i;

	Cvar_RegisterVariable (&scr_conalpha);
	Cvar_RegisterVariable (&scr_conbrightness);

	// init quad indices
	for (i = 0; i < MAX_BATCH_QUADS; i++)
	{
		batchindices[i*6 + 0] = i*4 + 0;
		batchindices[i*6 + 1] = i*4 + 1;
		batchindices[i*6 + 2] = i*4 + 2;
		batchindices[i*6 + 3] = i*4 + 0;
		batchindices[i*6 + 4] = i*4 + 2;
		batchindices[i*6 + 5] = i*4 + 3;
	}

	// clear scrap and allocate gltextures
	memset(scrap_allocated, 0, sizeof(scrap_allocated));
	memset(scrap_texels, 255, sizeof(scrap_texels));

	Scrap_Upload (); //creates 2 empty textures

	// create internal pics
	pic_ins = Draw_MakePic ("ins", 8, 9, &pic_ins_data[0][0]);
	pic_ovr = Draw_MakePic ("ovr", 8, 8, &pic_ovr_data[0][0]);
	pic_nul = Draw_MakePic ("nul", 8, 8, &pic_nul_data[0][0]);

	Draw_CreateWinQuakeMenuBgTex ();

	// load game pics
	Draw_LoadPics ();
}

//==============================================================================
//
//  2D DRAWING
//
//==============================================================================

/*
================
Draw_Flush
================
*/
void Draw_Flush (void)
{
	GLuint buf;
	GLbyte *ofs;

	if (!numbatchquads)
		return;

	if (scrap_dirty && glcanvas.texture == scrap_texture)
		Scrap_Upload ();

	GL_UseProgram (glprogs.gui);
	GL_SetState (glcanvas.blendmode | GLS_NO_ZTEST | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS(3));
	GL_Bind (GL_TEXTURE0, glcanvas.texture);

	GL_Upload (GL_ARRAY_BUFFER, batchverts, sizeof(batchverts[0]) * 4 * numbatchquads, &buf, &ofs);
	GL_BindBuffer (GL_ARRAY_BUFFER, buf);
	GL_VertexAttribPointerFunc (0, 2, GL_FLOAT, GL_FALSE, sizeof(batchverts[0]), ofs + offsetof(guivertex_t, pos));
	GL_VertexAttribPointerFunc (1, 2, GL_FLOAT, GL_FALSE, sizeof(batchverts[0]), ofs + offsetof(guivertex_t, uv));
	GL_VertexAttribPointerFunc (2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(batchverts[0]), ofs + offsetof(guivertex_t, color));

	GL_Upload (GL_ELEMENT_ARRAY_BUFFER, batchindices, sizeof(batchindices[0]) * 6 * numbatchquads, &buf, &ofs);
	GL_BindBuffer (GL_ELEMENT_ARRAY_BUFFER, buf);
	glDrawElements (GL_TRIANGLES, numbatchquads * 6, GL_UNSIGNED_SHORT, ofs);

	numbatchquads = 0;
}

/*
================
Draw_SetTexture
================
*/
static void Draw_SetTexture (gltexture_t *tex)
{
	if (tex == glcanvas.texture)
		return;
	Draw_Flush ();
	glcanvas.texture = tex;
}

/*
================
Draw_SetBlending
================
*/
static void Draw_SetBlending (unsigned blend)
{
	if (blend == glcanvas.blendmode)
		return;
	Draw_Flush ();
	glcanvas.blendmode = blend;
}

/*
================
GL_SetCanvasColor
================
*/
void GL_SetCanvasColor (float r, float g, float b, float a)
{
	glcanvas.colorstack[glcanvas.colorstacktop] =
		((uint32_t) CLAMP(0.f, r * 255.f + 0.5f, 255.f) <<  0) |
		((uint32_t) CLAMP(0.f, g * 255.f + 0.5f, 255.f) <<  8) |
		((uint32_t) CLAMP(0.f, b * 255.f + 0.5f, 255.f) << 16) |
		((uint32_t) CLAMP(0.f, a * 255.f + 0.5f, 255.f) << 24)
	;
}

/*
================
GL_PushCanvasColor
================
*/
void GL_PushCanvasColor (float r, float g, float b, float a)
{
	if (++glcanvas.colorstacktop >= (int) Q_COUNTOF (glcanvas.colorstack))
		Sys_Error ("GL_PushCanvasColor: overflow");
	GL_SetCanvasColor (r, g, b, a);
}

/*
================
GL_PopCanvasColor
================
*/
void GL_PopCanvasColor (void)
{
	if (--glcanvas.colorstacktop < 0)
		Sys_Error ("GL_PopCanvasColor: underflow");
}

/*
================
Draw_FadedOut
================
*/
static qboolean Draw_FadedOut (void)
{
	return (glcanvas.colorstack[glcanvas.colorstacktop] & 0xff000000u) == 0;
}

/*
================
Draw_AllocQuad
================
*/
static guivertex_t* Draw_AllocQuad (void)
{
	if (numbatchquads == MAX_BATCH_QUADS)
		Draw_Flush ();
	return batchverts + 4 * numbatchquads++;
}

/*
================
Draw_SetVertex
================
*/
static void Draw_SetVertex (guivertex_t *v, float x, float y, float s, float t)
{
	uint32_t color = glcanvas.colorstack[glcanvas.colorstacktop];
	v->pos[0] = x * glcanvas.transform.scale[0] + glcanvas.transform.offset[0];
	v->pos[1] = y * glcanvas.transform.scale[1] + glcanvas.transform.offset[1];
	v->uv[0] = s;
	v->uv[1] = t;
	v->color[0] = (color >>  0) & 0xff;
	v->color[1] = (color >>  8) & 0xff;
	v->color[2] = (color >> 16) & 0xff;
	v->color[3] = (color >> 24) & 0xff;
}

/*
================
Draw_CharacterQuadEx -- johnfitz -- seperate function to spit out verts
================
*/
void Draw_CharacterQuadEx (float x, float y, float dimx, float dimy, char num)
{
	int				row, col;
	float			frow, fcol, fsize;
	guivertex_t		*verts;

	row = num>>4;
	col = num&15;

	frow = row * 0.0625f + 1.f / (16.f * 10.f);
	fcol = col * 0.0625f + 1.f / (16.f * 10.f);
	fsize = 8.f / (16.f * 10.f);

	verts = Draw_AllocQuad ();
	Draw_SetVertex (verts++, x,      y,      fcol,         frow);
	Draw_SetVertex (verts++, x+dimx, y,      fcol + fsize, frow);
	Draw_SetVertex (verts++, x+dimx, y+dimy, fcol + fsize, frow + fsize);
	Draw_SetVertex (verts++, x,      y+dimy, fcol,         frow + fsize);
}

/*
================
Draw_CharacterQuad
================
*/
void Draw_CharacterQuad (int x, int y, char num)
{
	Draw_CharacterQuadEx (x, y, 8, 8, num);
}

/*
================
Draw_CharacterEx
================
*/
void Draw_CharacterEx (float x, float y, float dimx, float dimy, int num)
{
	if (y <= glcanvas.top - dimy)
		return;			// totally off screen

	num &= 255;

	if ((num & 0x7f) == 32)
		return; //don't waste verts on spaces

	Draw_SetTexture (char_texture);
	Draw_CharacterQuadEx (x, y, dimx, dimy, (char) num);
}

/*
================
Draw_Character
================
*/
void Draw_Character (int x, int y, int num)
{
	Draw_CharacterEx (x, y, 8, 8, (char) num);
}

/*
================
Draw_StringEx
================
*/
void Draw_StringEx (float x, float y, float dim, const char *str)
{
	if (y <= glcanvas.top - dim)
		return;			// totally off screen

	if (Draw_FadedOut ())
		return;

	Draw_SetTexture (char_texture);

	while (*str)
	{
		if ((*str & 0x7f) != 32) //don't waste verts on spaces
			Draw_CharacterQuadEx (x, y, dim, dim, *str);
		str++;
		x += dim;
	}
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, const char *str)
{
	Draw_StringEx (x, y, 8, str);
}

/*
=============
Draw_Pic -- johnfitz -- modified
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t		*gl;
	guivertex_t	*verts;

	if (!pic || Draw_FadedOut ())
		return;

	gl = (glpic_t *)pic->data;
	Draw_SetTexture (gl->gltexture);

	verts = Draw_AllocQuad ();
	Draw_SetVertex (verts++, x,            y,             gl->sl, gl->tl);
	Draw_SetVertex (verts++, x+pic->width, y,             gl->sh, gl->tl);
	Draw_SetVertex (verts++, x+pic->width, y+pic->height, gl->sh, gl->th);
	Draw_SetVertex (verts++, x,            y+pic->height, gl->sl, gl->th);
}

/*
=============
Draw_SubPic
=============
*/
void Draw_SubPic (float x, float y, float w, float h, qpic_t *pic, float s1, float t1, float s2, float t2, const float *rgb, float alpha)
{
	glpic_t		*gl;
	guivertex_t	*verts;

	if (!pic || alpha <= 0.0f)
		return;

	s2 += s1;
	t2 += t1;

	gl = (glpic_t *)pic->data;
	Draw_SetTexture (gl->gltexture);

	if (rgb)
		GL_PushCanvasColor (rgb[0], rgb[1], rgb[2], alpha);
	else
		GL_PushCanvasColor (1.f, 1.f, 1.f, alpha);

	verts = Draw_AllocQuad ();
	Draw_SetVertex (verts++, x,   y,   LERP (gl->sl, gl->sh, s1), LERP (gl->tl, gl->th, t1));
	Draw_SetVertex (verts++, x+w, y,   LERP (gl->sl, gl->sh, s2), LERP (gl->tl, gl->th, t1));
	Draw_SetVertex (verts++, x+w, y+h, LERP (gl->sl, gl->sh, s2), LERP (gl->tl, gl->th, t2));
	Draw_SetVertex (verts++, x,   y+h, LERP (gl->sl, gl->sh, s1), LERP (gl->tl, gl->th, t2));

	GL_PopCanvasColor ();
}

/*
=============
Draw_TransPicTranslate -- johnfitz -- rewritten to use texmgr to do translation

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom)
{
	static int oldtop = -2;
	static int oldbottom = -2;

	if (top != oldtop || bottom != oldbottom)
	{
		glpic_t *p = (glpic_t *)pic->data;
		gltexture_t *glt = p->gltexture;
		oldtop = top;
		oldbottom = bottom;
		TexMgr_ReloadImage (glt, top, bottom);
	}
	Draw_Pic (x, y, pic);
}

/*
================
Draw_ConsoleBackground -- johnfitz -- rewritten
================
*/
void Draw_ConsoleBackground (void)
{
	qpic_t *pic;
	float alpha, luma;

	pic = Draw_CachePic ("gfx/conback.lmp");
	pic->width = vid.conwidth;
	pic->height = vid.conheight;

	alpha = (con_forcedup) ? 1.0f : scr_conalpha.value;
	luma = scr_conbrightness.value;

	GL_SetCanvas (CANVAS_CONSOLE); //in case this is called from weird places

	if (alpha > 0.0f)
	{
		GL_PushCanvasColor (luma, luma, luma, alpha);
		Draw_Pic (0, 0, pic);
		GL_PopCanvasColor ();
	}
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	glpic_t	*gl;
	guivertex_t *verts;
	float scalex, scaley, uvscale, uvbias;

	gl = (glpic_t *)draw_backtile->data;

	GL_PushCanvasColor (1.f, 1.f, 1.f, 1.f);

	Draw_SetTexture (gl->gltexture);

	scalex = vid.guiwidth / (float) vid.width;
	scaley = vid.guiheight / (float) vid.height;
	uvscale = 1.f / 64.f / CLAMP (1.0f, scr_sbarscale.value, (float)vid.guiwidth / 320.0f); // use sbar scale
	uvbias = -vid.guiheight*uvscale; // bottom-aligned tiles

	verts = Draw_AllocQuad ();
	Draw_SetVertex (verts++, x*scalex,     y*scaley,     x*scalex*uvscale,     y*scaley*uvscale + uvbias);
	Draw_SetVertex (verts++, (x+w)*scalex, y*scaley,     (x+w)*scalex*uvscale, y*scaley*uvscale + uvbias);
	Draw_SetVertex (verts++, (x+w)*scalex, (y+h)*scaley, (x+w)*scalex*uvscale, (y+h)*scaley*uvscale + uvbias);
	Draw_SetVertex (verts++, x*scalex,     (y+h)*scaley, x*scalex*uvscale,     (y+h)*scaley*uvscale + uvbias);

	GL_PopCanvasColor ();
}

/*
=============
Draw_FillEx

Fills a box of pixels with a single color
=============
*/
void Draw_FillEx (float x, float y, float w, float h, const float *rgb, float alpha)
{
	guivertex_t *verts;

	if (alpha <= 0.f)
		return;

	GL_PushCanvasColor (rgb[0], rgb[1], rgb[2], alpha); //johnfitz -- added alpha
	Draw_SetTexture (whitetexture);

	verts = Draw_AllocQuad ();
	Draw_SetVertex (verts++, x,   y,   0.f, 0.f);
	Draw_SetVertex (verts++, x+w, y,   0.f, 0.f);
	Draw_SetVertex (verts++, x+w, y+h, 0.f, 0.f);
	Draw_SetVertex (verts++, x,   y+h, 0.f, 0.f);

	GL_PopCanvasColor ();
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c, float alpha) //johnfitz -- added alpha
{
	byte *pal = (byte *)d_8to24table; //johnfitz -- use d_8to24table instead of host_basepal
	float rgb[3];

	rgb[0] = pal[c*4+0] * (1.f/255.f);
	rgb[1] = pal[c*4+1] * (1.f/255.f);
	rgb[2] = pal[c*4+2] * (1.f/255.f);

	Draw_FillEx (x, y, w, h, rgb, alpha);
}

/*
================
Draw_PartialFadeScreen
================
*/
void Draw_PartialFadeScreen (float x0, float x1, float y0, float y1, float alpha)
{
	guivertex_t *verts;
	int type;
	float smax = 0.f, tmax = 0.f, s;
	float u0, u1, v0, v1;

	alpha *= scr_menubgalpha.value;
	if (alpha <= 0.f)
		return;

	u0 = GetFraction (x0, glcanvas.left, glcanvas.right);
	u1 = GetFraction (x1, glcanvas.left, glcanvas.right);
	v0 = GetFraction (y0, glcanvas.top, glcanvas.bottom);
	v1 = GetFraction (y1, glcanvas.top, glcanvas.bottom);

	if (scr_menubgstyle.value < 0.f)
	{
		if (softemu >= SOFTEMU_BANDED)
			type = MENUBG_DOSQUAKE;
		else if (softemu == SOFTEMU_COARSE)
			type = MENUBG_WINQUAKE_SCALED;
		else if (softemu == SOFTEMU_FINE)
			type = MENUBG_WINQUAKE;
		else
			type = MENUBG_GLQUAKE;
	}
	else
	{
		type = (int)scr_menubgstyle.value;
		type = CLAMP (0, type, MENUBG_NUMTYPES - 1);
	}

	if (type == MENUBG_DOSQUAKE)
	{
		float clr[3];
		Draw_SetTexture (whitetexture);
		/* first pass */
		Draw_SetBlending (GLS_BLEND_MULTIPLY);
		s = 1.f - CLAMP (0.f, alpha, 0.5f) * 2.f;
		clr[0] = LERP (0.56f, 1.f, s);
		clr[1] = LERP (0.43f, 1.f, s);
		clr[2] = LERP (0.13f, 1.f, s);
		GL_PushCanvasColor (clr[0], clr[1], clr[2], 1.f);
		verts = Draw_AllocQuad ();
		Draw_SetVertex (verts++, x0, y1, u0*smax, v1*tmax);
		Draw_SetVertex (verts++, x1, y1, u1*smax, v1*tmax);
		Draw_SetVertex (verts++, x1, y0, u1*smax, v0*tmax);
		Draw_SetVertex (verts++, x0, y0, u0*smax, v0*tmax);
		/* second pass */
		Draw_SetBlending (GLS_BLEND_ALPHA);
		s = CLAMP (0.f, alpha, 1.f);
		s = (sqrt (s) + s) * 0.5f;	// ~0.6 with scr_menubgalpha 0.5
		GL_SetCanvasColor (0.095f, 0.08f, 0.045f, s);
	}
	else if (type == MENUBG_WINQUAKE || type == MENUBG_WINQUAKE_SCALED)
	{
		if (type == MENUBG_WINQUAKE_SCALED)
		{
			s = q_min ((float)vid.guiwidth / 320.0f, (float)vid.guiheight / 200.0f);
			s = CLAMP (1.0f, scr_menuscale.value, s);
			s = floor (s);
		}
		else
			s = 1.f;
		smax = glwidth / (winquakemenubg->width * s);
		tmax = glheight / (winquakemenubg->height * s);
		Draw_SetTexture (winquakemenubg);
		if (alpha >= 0.5f)
		{
			Draw_SetBlending (GLS_BLEND_MULTIPLY);
			s = 2.f - q_min (1.f, alpha) * 2.f;
			GL_PushCanvasColor (s, s, s, 1.f);
		}
		else
		{
			Draw_SetBlending (GLS_BLEND_ALPHA);
			s = q_max (0.f, alpha) * 2.f;
			GL_PushCanvasColor (0.f, 0.f, 0.f, s);
		}
	}
	else // MENUBG_GLQUAKE
	{
		Draw_SetTexture (whitetexture);
		Draw_SetBlending (GLS_BLEND_ALPHA);
		GL_PushCanvasColor (0.f, 0.f, 0.f, alpha);
	}

	verts = Draw_AllocQuad ();
	Draw_SetVertex (verts++, x0, y1, u0*smax, v1*tmax);
	Draw_SetVertex (verts++, x1, y1, u1*smax, v1*tmax);
	Draw_SetVertex (verts++, x1, y0, u1*smax, v0*tmax);
	Draw_SetVertex (verts++, x0, y0, u0*smax, v0*tmax);

	Draw_SetBlending (GLS_BLEND_ALPHA);
	GL_PopCanvasColor ();

	Sbar_Changed();
}

/*
================
Draw_FadeScreen -- johnfitz -- revised
================
*/
void Draw_FadeScreen (float alpha)
{
	Draw_PartialFadeScreen (glcanvas.left, glcanvas.right, glcanvas.top, glcanvas.bottom, alpha);
}

/*
================
Draw_SetClipRect
================
*/
void Draw_SetClipRect (float x, float y, float width, float height)
{
	float x2 = x + width;
	float y2 = y + height;

	// canvas to -1..1
	x  = x  * glcanvas.transform.scale[0] + glcanvas.transform.offset[0];
	x2 = x2 * glcanvas.transform.scale[0] + glcanvas.transform.offset[0];
	y  = y  * glcanvas.transform.scale[1] + glcanvas.transform.offset[1];
	y2 = y2 * glcanvas.transform.scale[1] + glcanvas.transform.offset[1];
	// -1..1 to 0..1
	x  = CLAMP (0.f, x  * 0.5f + 0.5f, 1.f);
	x2 = CLAMP (0.f, x2 * 0.5f + 0.5f, 1.f);
	y  = CLAMP (0.f, y  * 0.5f + 0.5f, 1.f);
	y2 = CLAMP (0.f, y2 * 0.5f + 0.5f, 1.f);
	// 0..1 to screen
	x  = floor (glx + x  * glwidth  + 0.5f);
	x2 = floor (glx + x2 * glwidth  + 0.5f);
	y  = floor (gly + y  * glheight + 0.5f);
	y2 = floor (gly + y2 * glheight + 0.5f);

	Draw_Flush ();
	glEnable (GL_SCISSOR_TEST);
	glScissor (x, y2, x2 - x, y - y2);
}

/*
================
Draw_ResetClipping
================
*/
void Draw_ResetClipping (void)
{
	Draw_Flush ();
	glDisable (GL_SCISSOR_TEST);
}

#define CANVAS_ALIGN_LEFT		0.f
#define CANVAS_ALIGN_CENTERX	0.5f
#define CANVAS_ALIGN_RIGHT		1.f
#define CANVAS_ALIGN_TOP		0.f
#define CANVAS_ALIGN_CENTERY	0.5f
#define CANVAS_ALIGN_BOTTOM		1.f

/*
================
Draw_Transform2
================
*/
static void Draw_Transform2 (float width, float height, float scalex, float scaley, float alignx, float aligny, drawtransform_t *out)
{
	float scrwidth = vid.guiwidth;
	float scrheight = vid.guiheight;
	out->scale[0] = scalex * 2.f / scrwidth;
	out->scale[1] = scaley * -2.f / scrheight;
	out->offset[0] = (scrwidth - width*scalex) * alignx / scrwidth * 2.f - 1.f;
	out->offset[1] = (scrheight - height*scaley) * aligny / scrheight * -2.f + 1.f;

	// shift projection by a fraction of a pixel to avoid interpolation artifacts at certain scales
	out->offset[0] += 0.61803399f / 2.f / glwidth;
	out->offset[1] += 0.61803399f / 2.f / glheight;
}

/*
================
Draw_Transform
================
*/
static void Draw_Transform (float width, float height, float scale, float alignx, float aligny, drawtransform_t *out)
{
	Draw_Transform2 (width, height, scale, scale, alignx, aligny, out);
}

/*
================
Draw_GetCanvasTransform
================
*/
void Draw_GetCanvasTransform (canvastype type, drawtransform_t *transform)
{
	extern vrect_t scr_vrect;
	float s, s2;

	switch (type)
	{
	case CANVAS_DEFAULT:
		Draw_Transform (vid.guiwidth, vid.guiheight, 1.f, CANVAS_ALIGN_CENTERX, CANVAS_ALIGN_CENTERY, transform);
		break;
	case CANVAS_CONSOLE:
		s = (float)vid.guiwidth/vid.conwidth; //use console scale
		s2 = (float)vid.guiheight/vid.conheight;
		Draw_Transform2 (vid.conwidth, vid.conheight, s, s2, CANVAS_ALIGN_CENTERX, CANVAS_ALIGN_CENTERY, transform);
		transform->offset[1] += (1.f - scr_con_current/glheight) * 2.f;
		break;
	case CANVAS_MENU:
		s = q_min((float)vid.guiwidth / 320.0f, (float)vid.guiheight / 200.0f);
		s = CLAMP (1.0f, scr_menuscale.value, s);
		Draw_Transform (320, 200, s, CANVAS_ALIGN_CENTERX, CANVAS_ALIGN_CENTERY, transform);
		break;
	case CANVAS_CSQC:
		s = CLAMP (1.0f, scr_sbarscale.value, vid.guiwidth / 320.0f);
		Draw_Transform (vid.guiwidth/s, vid.guiheight/s, s, CANVAS_ALIGN_CENTERX, CANVAS_ALIGN_CENTERY, transform);
		break;
	case CANVAS_SBAR:
		if (hudstyle == HUD_QUAKEWORLD)	// qw hud could cut off if too short
			s = CLAMP (1.0f, scr_sbarscale.value, (float)vid.guiheight / 240.0f);
		else
			s = CLAMP(1.0f, scr_sbarscale.value, (float)vid.guiwidth / 320.0f);
		if (cl.gametype == GAME_DEATHMATCH && (hudstyle == HUD_CLASSIC || hudstyle == HUD_QUAKEWORLD))
			Draw_Transform (320, 48, s, CANVAS_ALIGN_LEFT, CANVAS_ALIGN_BOTTOM, transform);
		else
			Draw_Transform (320, 48, s, CANVAS_ALIGN_CENTERX, CANVAS_ALIGN_BOTTOM, transform);
		break;
    case CANVAS_SBAR_QW_INV:
		s = CLAMP(1.0f, scr_sbarscale.value, (float)vid.guiheight / 240.0f);
        Draw_Transform (48, 48, s, CANVAS_ALIGN_RIGHT, CANVAS_ALIGN_BOTTOM, transform);
		break;
	case CANVAS_SBAR2:
		s = q_min (vid.guiwidth / 400.0f, vid.guiheight / 225.0f);
		s = CLAMP (1.0f, scr_sbarscale.value, s);
		Draw_Transform (vid.guiwidth/s, vid.guiheight/s, s, CANVAS_ALIGN_CENTERX, CANVAS_ALIGN_CENTERY, transform);
		break;
	case CANVAS_CROSSHAIR: //0,0 is center of viewport
		s = CLAMP (1.0f, scr_crosshairscale.value, 10.0f);
		Draw_Transform (vid.guiwidth/s/2, vid.guiheight/s/2, s, CANVAS_ALIGN_LEFT, CANVAS_ALIGN_BOTTOM, transform);
		transform->offset[0] += 1.f;
		transform->offset[1] += 1.f - ((scr_vrect.y + scr_vrect.height / 2) * 2 / (float)glheight);
		break;
	case CANVAS_BOTTOMLEFT: //used by devstats
		s = (float)vid.guiwidth/vid.conwidth; //use console scale
		Draw_Transform (320, 200, s, CANVAS_ALIGN_LEFT, CANVAS_ALIGN_BOTTOM, transform);
		break;
	case CANVAS_BOTTOMRIGHT: //used by fps/clock
		s = (float)vid.guiwidth/vid.conwidth; //use console scale
		Draw_Transform (320, 200, s, CANVAS_ALIGN_RIGHT, CANVAS_ALIGN_BOTTOM, transform);
		break;
	case CANVAS_TOPRIGHT: //used by disc
		s = (float)vid.guiwidth/vid.conwidth; //use console scale
		Draw_Transform (320, 200, s, CANVAS_ALIGN_RIGHT, CANVAS_ALIGN_TOP, transform);
		break;
	default:
		Sys_Error ("Draw_GetCanvasTransform: bad canvas type");
	}
}

/*
================
Draw_GetTransformBounds
================
*/
void Draw_GetTransformBounds (const drawtransform_t *transform, float *left, float *top, float *right, float *bottom)
{
	*left	= (-1.f - transform->offset[0]) / transform->scale[0];
	*right	= ( 1.f - transform->offset[0]) / transform->scale[0];
	*bottom	= (-1.f - transform->offset[1]) / transform->scale[1];
	*top	= ( 1.f - transform->offset[1]) / transform->scale[1];
}

/*
================
GL_SetCanvas -- johnfitz -- support various canvas types
================
*/
void GL_SetCanvas (canvastype newcanvas)
{
	if (newcanvas == glcanvas.type)
		return;

	glcanvas.type = newcanvas;
	Draw_GetCanvasTransform (glcanvas.type, &glcanvas.transform);
	Draw_GetTransformBounds (&glcanvas.transform, &glcanvas.left, &glcanvas.top, &glcanvas.right, &glcanvas.bottom);
}

/*
================
GL_Set2D
================
*/
void GL_Set2D (void)
{
	glcanvas.type = CANVAS_INVALID;
	glcanvas.texture = NULL;
	glcanvas.blendmode = GLS_BLEND_ALPHA;
	glcanvas.colorstacktop = 0;
	glViewport (glx, gly, glwidth, glheight);
	GL_SetCanvas (CANVAS_DEFAULT);
	GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);
}
