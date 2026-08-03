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
// r_brush.c: brush model rendering. renamed from r_surf.c

#include "quakedef.h"

extern cvar_t gl_fullbrights, gl_overbright; //johnfitz

int		gl_lightmap_format;
int		lightmap_bytes;

typedef struct {
	qboolean		reverse;
	int				x, width, height;
	int				*allocated;
} chart_t;

#define MAX_SANITY_LIGHTMAPS (1u<<20)
lightmap_t		*lightmaps;
int				lightmap_count;
int				last_lightmap_allocated;
chart_t			lightmap_chart;
msurface_t		**lit_surfs;
int				*lit_surf_order[2];
int				num_lightmap_samples;
unsigned		*lightmap_data;
gltexture_t		*lightmap_texture;
int				lightmap_width;
int				lightmap_height;


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base, int frame)
{
	int		relative;
	int		count;

	if (frame)
		if (base->alternate_anims)
			base = base->alternate_anims;

	if (!base->anim_total)
		return base;

	relative = (int)(cl.time*10) % base->anim_total;

	count = 0;
	while (base->anim_min > relative || base->anim_max <= relative)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}

/*
=============================================================

	LIGHTMAPS

=============================================================
*/

/*
==================
Chart_Init
==================
*/
static void Chart_Init (chart_t *chart, int width, int height)
{
	if (chart->width != width)
	{
		chart->allocated = (int *)realloc (chart->allocated, sizeof (chart->allocated[0]) * width);
		if (!chart->allocated)
			Sys_Error ("Chart_Init: could not allocate %" SDL_PRIu64 " bytes", (uint64_t) (sizeof (chart->allocated[0]) * width));
	}
	memset (chart->allocated, 0, sizeof (chart->allocated[0]) * width);
	chart->width = width;
	chart->height = height;
	chart->x = 0;
	chart->reverse = false;
}

/*
==================
Chart_Add
==================
*/
static qboolean Chart_Add (chart_t *chart, int w, int h, short *outx, short *outy)
{
	int i, x, y;
	if (chart->width < w || chart->height < h)
		Sys_Error ("Chart_Add: block too large %dx%d, max is %dx%d", w, h, chart->width, chart->height);

	// advance horizontally, reversing direction at the edges
	if (chart->reverse)
	{
		if (chart->x < w)
		{
			chart->x = 0;
			chart->reverse = false;
			goto forward;
		}
	reverse:
		x = chart->x - w;
		chart->x = x;
	}
	else
	{
		if (chart->x + w > chart->width)
		{
			chart->x = chart->width;
			chart->reverse = true;
			goto reverse;
		}
	forward:
		x = chart->x;
		chart->x += w;
	}

	// find lowest unoccupied vertical position
	y = 0;
	for (i = 0; i < w; i++)
		y = q_max (y, chart->allocated[x + i]);
	if (y + h > chart->height)
		return false;

	// update vertical position for each column
	for (i = 0; i < w; i++)
		chart->allocated[x + i] = y + h;

	*outx = x;
	*outy = y;

	return true;
}

/*
========================
AllocBlock -- returns a texture number and the position inside it
========================
*/
static int AllocBlock (int w, int h, short *x, short *y)
{
	int		texnum;

	// ericw -- rather than searching starting at lightmap 0 every time,
	// start at the last lightmap we allocated a surface in.
	// This makes AllocBlock much faster on large levels (can shave off 3+ seconds
	// of load time on a level with 180 lightmaps), at a cost of not quite packing
	// lightmaps as tightly vs. not doing this (uses ~5% more lightmaps)
	for (texnum=last_lightmap_allocated ; texnum<MAX_SANITY_LIGHTMAPS ; texnum++)
	{
		if (texnum == lightmap_count)
		{
			lightmap_count++;
			lightmaps = (lightmap_t *) realloc(lightmaps, sizeof(*lightmaps)*lightmap_count);
			memset(&lightmaps[texnum], 0, sizeof(lightmaps[texnum]));
			// as we're only tracking one texture, we don't need multiple copies any more.
			Chart_Init (&lightmap_chart, LMBLOCK_WIDTH, LMBLOCK_HEIGHT);
			// reserve 1 texel for unlit water surfaces in maps with lit water
			if (lightmap_count == 1)
			{
				lightmap_chart.x = 1;
				lightmap_chart.allocated[0] = 1;
			}
		}

		if (!Chart_Add (&lightmap_chart, w, h, x, y))
			continue;

		last_lightmap_allocated = texnum;
		return texnum;
	}

	Sys_Error ("AllocBlock: full");
	return 0; //johnfitz -- shut up compiler
}

/*
========================
GL_NumLightmapTaps
========================
*/
static int GL_NumLightmapTaps (const msurface_t *surf)
{
	if (surf->styles[1] == 255)
		return 1;
	if (surf->styles[2] == 255)
		return 2;
	return 3;
}

/*
========================
GL_FillSurfaceLightmap
========================
*/
static void GL_FillSurfaceLightmap (msurface_t *surf)
{
	lightmap_t	*lm;
	int			smax, tmax;
	int			xofs, yofs;
	int			map;
	byte		*src;
	unsigned	*dst;
	int			s, t, facesize;

	if (!cl.worldmodel->lightdata || !surf->samples || surf->styles[0] == 255)
		return;

	lm = &lightmaps[surf->lightmaptexturenum];
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	xofs = lm->xofs + surf->light_s;
	yofs = lm->yofs + surf->light_t;
	facesize = smax * tmax * 3;

	src = surf->samples;
	dst = lightmap_data + yofs * lightmap_width + xofs;

	if (surf->styles[1] == 255) // single lightstyle
	{
		for (t = 0; t < tmax; t++, dst += lightmap_width)
			for (s = 0; s < smax; s++, src += 3)
				dst[s] = src[0] | (src[1] << 8) | (src[2] << 16) | 0xff000000u;
	}
	else if (surf->styles[2] == 255) // 2 lightstyles
	{
		for (t = 0; t < tmax; t++, dst += lightmap_width)
		{
			for (s = 0; s < smax; s++, src += 3)
			{
				dst[s       ] = src[0           ] | (src[1           ] << 8) | (src[2           ] << 16) | 0xff000000u;
				dst[s + smax] = src[0 + facesize] | (src[1 + facesize] << 8) | (src[2 + facesize] << 16) | 0xff000000u;
			}
		}
	}
	else // 3 or 4 lightstyles
	{
		for (t = 0; t < tmax; t++, dst += lightmap_width)
		{
			for (s = 0; s < smax; s++, src += 3)
			{
				const byte *mapsrc = src;
				unsigned r = 0, g = 0, b = 0;
				for (map = 0; map < 4 && surf->styles[map] != 255; map++, mapsrc += facesize)
				{
					r |= mapsrc[0] << (map << 3);
					g |= mapsrc[1] << (map << 3);
					b |= mapsrc[2] << (map << 3);
				}
				dst[s           ] = r;
				dst[s + smax    ] = g;
				dst[s + smax * 2] = b;
			}
		}
	}
}

/*
==================
GL_FreeLightmapData
==================
*/
static void GL_FreeLightmapData (void)
{
	if (lightmap_data)
	{
		free (lightmap_data);
		lightmap_data = NULL;
	}
	if (lightmaps)
	{
		free (lightmaps);
		lightmaps = NULL;
	}

	VEC_CLEAR (lit_surfs);

	lightmap_texture = NULL; // freed by the texture manager
	last_lightmap_allocated = 0;
	lightmap_count = 0;
	lightmap_width = 0;
	lightmap_height = 0;
	num_lightmap_samples = 0;
}

/*
==================
GL_PackLitSurfaces
==================
*/
static void GL_PackLitSurfaces (void)
{
	int			i, j, k, pass, bins[256];
	int			maxblack[2] = {0, 0};
	short		blackofs[2];
	int			blacklm;
	msurface_t *surf;

	// generate surface list
	for (j=1 ; j<MAX_MODELS ; j++)
	{
		qmodel_t *m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		for (i=0, surf=m->surfaces ; i<m->numsurfaces ; i++, surf++)
		{
			int w, h;
			if (surf->flags & SURF_DRAWTILED)
				continue;

			w = (surf->extents[0]>>4)+1;
			h = (surf->extents[1]>>4)+1;
			if (!surf->samples)
			{
				maxblack[0] = q_max (maxblack[0], w);
				maxblack[1] = q_max (maxblack[1], h);
			}
			w *= GL_NumLightmapTaps (surf);
			w = q_min (w, 255);

			// use light_s temporarily as a sort key
			surf->light_s = Interleave (w, h) ^ 0xffffu;
			VEC_PUSH (lit_surfs, surf);
		}
	}

	blacklm = AllocBlock (maxblack[0]+1, maxblack[1]+1, &blackofs[0], &blackofs[1]);

	if (VEC_SIZE (lit_surfs) == 0)
		return;

	lit_surf_order[0] = (int *) realloc (lit_surf_order[0], sizeof (lit_surf_order[0][0]) * VEC_SIZE (lit_surfs));
	lit_surf_order[1] = (int *) realloc (lit_surf_order[1], sizeof (lit_surf_order[1][0]) * VEC_SIZE (lit_surfs));

	if (!lit_surf_order[0] || !lit_surf_order[1])
		Sys_Error ("GL_PackLitSurfaces: out of memory (%" SDL_PRIu64 " surfs)", (uint64_t) VEC_SIZE (lit_surfs));

	for (i = 0, j = VEC_SIZE (lit_surfs); i < j; i++)
		lit_surf_order[0][i] = i;

	// generate surface order (radix sort: 2 passes x 8-bits)
	for (pass = 0; pass < 2; pass++)
	{
		memset (bins, 0, sizeof (bins));

		// count keys
		for (i = 0, j = VEC_SIZE (lit_surfs); i < j; i++)
		{
			int idx = lit_surf_order[pass][i];
			surf = lit_surfs[idx];
			k = surf->light_s & 255;
			++bins[k];
		}

		// generate offsets (prefix sum)
		for (i = 0, j = 0; i < countof(bins); i++)
		{
			int tmp = bins[i];
			bins[i] = j;
			j += tmp;
		}

		// reorder
		for (i = 0, j = VEC_SIZE (lit_surfs); i < j; i++)
		{
			int idx = lit_surf_order[pass][i];
			surf = lit_surfs[idx];
			k = surf->light_s & 255;
			surf->light_s >>= 8;
			lit_surf_order[pass ^ 1][bins[k]++] = idx;
		}
	}

	// pack surfaces in sort order
	for (i = 0, j = VEC_SIZE (lit_surfs); i < j; i++)
	{
		int smax, tmax;

		surf = lit_surfs[lit_surf_order[0][i]];
		smax = (surf->extents[0]>>4)+1;
		tmax = (surf->extents[1]>>4)+1;
		smax *= GL_NumLightmapTaps (surf);
		num_lightmap_samples += smax * tmax;

		if (surf->samples)
		{
			surf->lightmaptexturenum = AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
		}
		else
		{
			surf->lightmaptexturenum = blacklm;
			surf->light_s = blackofs[0];
			surf->light_t = blackofs[1];
		}
	}
}

/*
==================
GL_BuildLightmaps -- called at level load time

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps (void)
{
	int			i, j, xblocks, yblocks, lmsize;
	lightmap_t	*lm;

	r_framecount = 1; // no dlightcache

	//Spike -- wipe out all the lightmap data (johnfitz -- the gltexture objects were already freed by Mod_ClearAll)
	GL_FreeLightmapData ();

	gl_lightmap_format = GL_RGBA;//FIXME: hardcoded for now!

	switch (gl_lightmap_format)
	{
	case GL_RGBA:
		lightmap_bytes = 4;
		break;
	case GL_BGRA:
		lightmap_bytes = 4;
		break;
	default:
		Sys_Error ("GL_BuildLightmaps: bad lightmap format");
	}

	// allocate lightmap blocks
	GL_PackLitSurfaces ();

	// determine combined texture size and allocate memory for it
	xblocks = (int) ceil (sqrt (lightmap_count));
	yblocks = (lightmap_count + xblocks - 1) / xblocks;
	lightmap_width = xblocks * LMBLOCK_WIDTH;
	lightmap_height = yblocks * LMBLOCK_HEIGHT;
	lmsize = lightmap_width * lightmap_height;
	if (q_max(lightmap_width, lightmap_height) > gl_max_texture_size)
	{
		// dimensions get zero-ed out in GL_FreeLightmapData, save them for the error message
		int w = lightmap_width;
		int h = lightmap_height;
		GL_FreeLightmapData ();
		Host_Error ("Lightmap texture overflow: needed %dx%d, max is %dx%d\n", w, h, gl_max_texture_size, gl_max_texture_size);
	}

	Con_DPrintf (
		"Lightmap size:   %d x %d (%d/%d blocks)\n"
		"Lightmap memory: %.1lf MB (%.1lf%% efficiency)\n",
		lightmap_width, lightmap_height, lightmap_count, xblocks * yblocks,
		(lightmap_bytes * lmsize) / (float)0x100000, 100.0 * num_lightmap_samples / lmsize
	);

	lightmap_data = (unsigned *) calloc (lmsize, sizeof (*lightmap_data));
	if (!lightmap_data)
		Sys_Error ("GL_BuildLightmaps: out of memory on %" SDL_PRIu64 " bytes", (uint64_t)(lmsize * sizeof (*lightmap_data)));

	// compute offsets for each lightmap block
	for (i=0; i<lightmap_count; i++)
	{
		lm = &lightmaps[i];
		lm->xofs = (i % xblocks) * LMBLOCK_WIDTH;
		lm->yofs = (i / xblocks) * LMBLOCK_HEIGHT;
	}

	// fill reserved texel
	lightmap_data[0] = 0xff808080u;

	// unlit map? fill with 50% grey
	if (!cl.worldmodel->lightdata)
		for (i = 1; i < lmsize; i++)
			lightmap_data[i] = 0xff808080u;

	// fill lightmap samples
	for (i = 0, j = VEC_SIZE (lit_surfs); i < j; i++)
		GL_FillSurfaceLightmap (lit_surfs[i]);

	lightmap_texture =
		TexMgr_LoadImage (cl.worldmodel, "lightmap", lightmap_width, lightmap_height,
			SRC_LIGHTMAP, (byte *)lightmap_data, "", (src_offset_t)lightmap_data,
			TEXPREF_ALPHA | TEXPREF_LINEAR | TEXPREF_NOPICMIP
		);

	//johnfitz -- warn about exceeding old limits
	//GLQuake limit was 64 textures of 128x128. Estimate how many 128x128 textures we would need
	//given that we are using lightmap_count of LMBLOCK_WIDTH x LMBLOCK_HEIGHT
	i = lightmap_count * ((LMBLOCK_WIDTH / 128) * (LMBLOCK_HEIGHT / 128));
	if (i > 64)
		Con_DWarning("%i lightmaps exceeds standard limit of 64.\n",i);
	//johnfitz
}

/*
=============================================================

	VBO support

=============================================================
*/

GLuint gl_bmodel_vbo = 0;
size_t gl_bmodel_vbo_size = 0;

GLuint gl_bmodel_ibo = 0;
size_t gl_bmodel_ibo_size = 0;
GLuint gl_bmodel_indirect_buffer = 0;
size_t gl_bmodel_indirect_buffer_size = 0;
GLuint gl_bmodel_surf_buffer = 0;
GLuint gl_bmodel_marksurf_buffer = 0;
GLuint gl_bmodel_marksurf_buffer_size = 0;

/*
==================
GL_DeleteBModelBuffers
==================
*/
void GL_DeleteBModelBuffers (void)
{
	GL_DeleteBuffer (gl_bmodel_vbo);
	GL_DeleteBuffer (gl_bmodel_ibo);
	GL_DeleteBuffer (gl_bmodel_indirect_buffer);
	GL_DeleteBuffer (gl_bmodel_surf_buffer);
	GL_DeleteBuffer (gl_bmodel_marksurf_buffer);
	gl_bmodel_vbo = 0;
	gl_bmodel_vbo_size = 0;
	gl_bmodel_ibo = 0;
	gl_bmodel_ibo_size = 0;
	gl_bmodel_indirect_buffer = 0;
	gl_bmodel_indirect_buffer_size = 0;
	gl_bmodel_surf_buffer = 0;
	gl_bmodel_marksurf_buffer = 0;
	gl_bmodel_marksurf_buffer_size = 0;
}

/*
==================
GL_BuildBModelVertexBuffer

Rebuilds gl_bmodel_vbo with all surfaces from world + all brush models
==================
*/
void GL_BuildBModelVertexBuffer (void)
{
	unsigned int	numverts, varray_bytes, varray_index;
	int			i, j, k;
	qmodel_t	*m;
	glvert_t	*varray;
	float		lmscalex = 1.f / 16.f / lightmap_width;
	float		lmscaley = 1.f / 16.f / lightmap_height;

// count all verts in all models
	numverts = 0;
	for (j=1 ; j<MAX_MODELS ; j++)
	{
		m = cl.model_precache[j];
		if (!m || m->name[0] == '*' || m->type != mod_brush)
			continue;

		for (i=0 ; i<m->numsurfaces ; i++)
		{
			numverts += m->surfaces[i].numedges;
		}
	}
	
// build vertex array
	varray_bytes = sizeof (glvert_t) * numverts;
	varray = (glvert_t *) malloc (varray_bytes);
	if (!varray)
		Sys_Error ("GL_BuildBModelVertexBuffer: out of memory on %u bytes", varray_bytes);
	varray_index = 0;
	
	for (j=1 ; j<MAX_MODELS ; j++)
	{
		m = cl.model_precache[j];
		if (!m || m->name[0] == '*' || m->type != mod_brush)
			continue;

		for (i = 0; i < m->numsurfaces; i++)
		{
			msurface_t	*fa = &m->surfaces[i];
			texture_t	*texture = m->textures[fa->texinfo->texnum];
			glvert_t	*vert = &varray[varray_index];
			float		texscalex, texscaley, useofs, lmofs;
			medge_t		*r_pedge;
			lightmap_t	*lm;

			if (fa->flags & SURF_DRAWTILED)
			{
				// match old Mod_PolyForUnlitSurface
				if (fa->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
					texscalex = 1.f / 128.f; //warp animation repeats every 128
				else
					texscalex = 1.f / 32.f; //to match r_notexture_mip
				texscaley = texscalex;
				useofs = 0.f; //unlit surfaces don't use the texture offset
				lmofs = 0.f;
				lm = NULL;
			}
			else
			{
				if (fa->flags & SURF_DRAWTURB)
				{
					texscaley = texscalex = 1.f / 128.f; //warp animation repeats every 128
					useofs = 0.f;
				}
				else
				{
					texscalex = 1.f / texture->width;
					texscaley = 1.f / texture->height;
					useofs = 1.f;
				}
				lm = &lightmaps[fa->lightmaptexturenum];
				lmofs = ((fa->extents[0]>>4)+1) / (float)lightmap_width;
			}

			fa->vbo_firstvert = varray_index;
			varray_index += fa->numedges;

			for (k = 0; k < fa->numedges; k++, vert++)
			{
				float	*vec;
				float	s, t;
				int		lindex;

				lindex = m->surfedges[fa->firstedge + k];
				if (lindex > 0)
				{
					r_pedge = &m->edges[lindex];
					vec = m->vertexes[r_pedge->v[0]].position;
				}
				else
				{
					r_pedge = &m->edges[-lindex];
					vec = m->vertexes[r_pedge->v[1]].position;
				}

				s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3] * useofs;
				s *= texscalex;

				t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3] * useofs;
				t *= texscaley;

				VectorCopy (vec, vert->pos);
				vert->st[0] = s;
				vert->st[1] = t;

				if (!(fa->flags & SURF_DRAWTILED))
				{
					// match old BuildSurfaceDisplayList

					// Q64 RERELEASE texture shift
					if (texture->shift > 0)
					{
						vert->st[0] /= (2 * texture->shift);
						vert->st[1] /= (2 * texture->shift);
					}

					//
					// lightmap texture coordinates
					//
					s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
					s -= fa->texturemins[0];
					s += (fa->light_s + lm->xofs) * 16;
					s += 8;
					s *= lmscalex;

					t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
					t -= fa->texturemins[1];
					t += (fa->light_t + lm->yofs) * 16;
					t += 8;
					t *= lmscaley;

					vert->st[2] = s;
					vert->st[3] = t;
					vert->lmofs = lmofs;
					vert->styles = fa->styles[0] | (fa->styles[1] << 8) | (fa->styles[2] << 16) | (fa->styles[3] << 24);
				}
				else
				{
					// first lightmap texel is fullbright
					vert->st[2] = 0.5f / lightmap_width;
					vert->st[3] = 0.5f / lightmap_height;
					vert->lmofs = 0.f;
					vert->styles = ~0u;
				}
			}
		}
	}

// upload to GPU
	gl_bmodel_vbo_size = varray_bytes;
	gl_bmodel_vbo = GL_CreateBuffer (GL_ARRAY_BUFFER, GL_STATIC_DRAW, "brushverts", varray_bytes, varray);
	free (varray);
}

/*
===============
CompareMarkSurface
===============
*/
static int CompareMarkSurface (const void *pa, const void *pb)
{
	msurface_t *sa = &cl.worldmodel->surfaces[((bmodel_gpu_marksurf_t *)pa)->surfindex];
	msurface_t *sb = &cl.worldmodel->surfaces[((bmodel_gpu_marksurf_t *)pb)->surfindex];
	return sb->numedges - sa->numedges;
}

/*
===============
GL_BuildBModelMarkBuffers
===============
*/
void GL_BuildBModelMarkBuffers (void)
{
	int			i, j, k, sum;
	int			numtex = 0, numtris = 0, maxnumtex = 0, nummark = 0;
	int			*texidx = NULL;
	GLuint		*idx;
	bmodel_gpu_marksurf_t *mark;
	bmodel_draw_indirect_t *cmds;
	bmodel_gpu_surf_t *surfs;

	if (!cl.worldmodel)
		return;

	// count bmodel textures and triangles
	for (j = 1 ; j < MAX_MODELS; j++)
	{
		qmodel_t *m = cl.model_precache[j];
		if (!m || m->type != mod_brush)
			continue;
		m->firstcmd = numtex;
		numtex += m->texofs[TEXTYPE_COUNT];
		maxnumtex = q_max (maxnumtex, m->numtextures);
		for (i = 0; i < m->nummodelsurfaces; i++)
			numtris += q_max (m->surfaces[i + m->firstmodelsurface].numedges, 2) - 2;
	}

	// count marksurfaces used by worldmodel leafs
	for (i = 0; i < cl.worldmodel->numleafs; i++)
		nummark += cl.worldmodel->leafs[i + 1].nummarksurfaces;

	// allocate cpu-side buffers
	gl_bmodel_ibo_size = numtris * 3 * sizeof(idx[0]);
	gl_bmodel_indirect_buffer_size = numtex * sizeof(cmds[0]);
	gl_bmodel_marksurf_buffer_size = nummark * sizeof(mark[0]);
	cmds = (bmodel_draw_indirect_t *) calloc (numtex, sizeof(cmds[0]));
	if (!cmds)
		Sys_Error ("GL_BuildBModelMarkBuffers: out of memory (%d cmds)", numtex);
	idx = (GLuint *) calloc (numtris * 3, sizeof(idx[0]));
	if (!idx)
		Sys_Error ("GL_BuildBModelMarkBuffers: out of memory (%d indices)", numtris * 3);
	mark = (bmodel_gpu_marksurf_t *) calloc (nummark, sizeof (mark[0]));
	if (!mark)
		Sys_Error ("GL_BuildBModelMarkBuffers: out of memory (%d marksurfs)", nummark);
	surfs = (bmodel_gpu_surf_t *) calloc (cl.worldmodel->numsurfaces, sizeof(surfs[0]));
	if (!surfs)
		Sys_Error ("GL_BuildBModelMarkBuffers: out of memory (%d surfs)", cl.worldmodel->numsurfaces);
	texidx = (int *) calloc (maxnumtex, sizeof(texidx[0]));
	if (!texidx)
		Sys_Error ("GL_BuildBModelMarkBuffers: out of memory (%d tex indices)", maxnumtex);

	// fill marksurface data
	for (i = sum = 0; i < cl.worldmodel->numleafs; i++)
	{
		mleaf_t *leaf = &cl.worldmodel->leafs[i + 1];
		GLuint packedleafsky = (i << 1) | (leaf->contents == CONTENTS_SKY);
		for (j = 0; j < leaf->nummarksurfaces; j++)
		{
			mark[sum + j].packedleafsky = packedleafsky;
			mark[sum + j].surfindex = leaf->firstmarksurface[j];
		}
		qsort (mark + sum, j, sizeof (*mark), CompareMarkSurface);
		sum += j;
	}

	for (i = 0; i < cl.worldmodel->texofs[TEXTYPE_COUNT]; i++)
		texidx[cl.worldmodel->usedtextures[i]] = i;

	// fill worldmodel surface data
	for (i = 0; i < cl.worldmodel->numsurfaces; i++)
	{
		msurface_t *src = &cl.worldmodel->surfaces[i];
		bmodel_gpu_surf_t *dst = &surfs[i];
		float flip = (src->flags & SURF_PLANEBACK) ? -1.f : 1.f;

		if (src->texinfo->texnum < 0 || src->texinfo->texnum >= cl.worldmodel->numtextures)
			Sys_Error ("GL_BuildBModelMarkBuffers: bad texnum %d (total=%d)", src->texinfo->texnum, cl.worldmodel->numtextures);

		memcpy (dst->mins, src->mins, 3 * sizeof (float));
		memcpy (dst->maxs, src->maxs, 3 * sizeof (float));
		dst->plane[0] = src->plane->normal[0] * flip;
		dst->plane[1] = src->plane->normal[1] * flip;
		dst->plane[2] = src->plane->normal[2] * flip;
		dst->plane[3] = src->plane->dist * flip;
		dst->texnum = texidx[src->texinfo->texnum];
		dst->numedges = src->numedges;
		dst->firstvert = src->vbo_firstvert;
	}

	// count triangles for each model texture
	for (j = 1 ; j < MAX_MODELS; j++)
	{
		qmodel_t *m = cl.model_precache[j];
		msurface_t *s;
		if (!m || m->type != mod_brush)
			continue;

		memset (texidx, 0, sizeof(texidx[0]) * m->numtextures);
		for (i = 0; i < m->texofs[TEXTYPE_COUNT]; i++)
			texidx[m->usedtextures[i]] = i;

		for (i = 0, s = m->surfaces + m->firstmodelsurface; i < m->nummodelsurfaces; i++, s++)
			cmds[m->firstcmd + texidx[s->texinfo->texnum]].count += (q_max (s->numedges, 2) - 2) * 3;
	}

	// compute per-drawcall index buffer offsets
	sum = 0;
	for (i = 0; i < numtex; i++)
	{
		cmds[i].firstIndex = sum;
		sum += cmds[i].count;
		cmds[i].instanceCount = 1;
	}

	// build index buffer
	for (j = 1 ; j < MAX_MODELS; j++)
	{
		qmodel_t *m = cl.model_precache[j];
		msurface_t *s;
		if (!m || m->type != mod_brush)
			continue;

		memset (texidx, 0, sizeof(texidx[0]) * m->numtextures);
		for (i = 0; i < m->texofs[TEXTYPE_COUNT]; i++)
			texidx[m->usedtextures[i]] = i;

		for (i = 0, s = m->surfaces + m->firstmodelsurface; i < m->nummodelsurfaces; i++, s++)
		{
			bmodel_draw_indirect_t *draw = &cmds[m->firstcmd + texidx[s->texinfo->texnum]];
			for (k = 2; k < s->numedges; k++)
			{
				idx[draw->firstIndex++] = s->vbo_firstvert;
				idx[draw->firstIndex++] = s->vbo_firstvert + k - 1;
				idx[draw->firstIndex++] = s->vbo_firstvert + k;
			}
		}
	}

	// restore firstIndex values (they get shifted in the previous loop)
	sum = 0;
	for (i = 0; i < numtex; i++)
	{
		cmds[i].firstIndex = sum;
		sum += cmds[i].count;
	}

	// create gpu buffers
	gl_bmodel_indirect_buffer = GL_CreateBuffer (GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW, "bmodel indirect cmds",
		sizeof (cmds[0]) * numtex, cmds
	);
	gl_bmodel_ibo = GL_CreateBuffer (GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, "bmodel indices",
		sizeof (idx[0]) * numtris * 3, idx
	);
	gl_bmodel_surf_buffer = GL_CreateBuffer (GL_SHADER_STORAGE_BUFFER, GL_STATIC_DRAW, "bmodel surfs",
		sizeof (surfs[0]) * cl.worldmodel->numsurfaces, surfs
	);
	gl_bmodel_marksurf_buffer = GL_CreateBuffer (GL_SHADER_STORAGE_BUFFER, GL_STATIC_DRAW, "bmodel marksurfs",
		sizeof(mark[0]) * nummark, mark
	);

	// free cpu-side arrays
	free (texidx);
	free (surfs);
	free (mark);
	free (idx);
	free (cmds);
}
