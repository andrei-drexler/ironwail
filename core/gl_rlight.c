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
// r_light.c

#include "quakedef.h"

extern cvar_t r_flatlightstyles; //johnfitz
extern cvar_t r_lerplightstyles;
extern cvar_t r_dynamic;

gpulightbuffer_t r_lightbuffer;

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i,j,k,n;
	double		f,base;

//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	f = cl.time * 10.0;
	base = floor(f);
	i = (int)base;
	f -= base;
	if (!r_lerplightstyles.value)
		f = 0.0;

	for (j=0 ; j<MAX_LIGHTSTYLES ; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			r_lightbuffer.lightstyles[j] = 1.f;
			continue;
		}
		//johnfitz -- r_flatlightstyles
		if (r_flatlightstyles.value == 2)
			k = n = cl_lightstyle[j].peak - 'a';
		else if (r_flatlightstyles.value == 1 || !r_dynamic.value)
			k = n = cl_lightstyle[j].average - 'a';
		else
		{
			k = i % cl_lightstyle[j].length;
			n = k + 1;
			if (n == cl_lightstyle[j].length)
				n = 0;
			k = cl_lightstyle[j].map[k] - 'a';
			n = cl_lightstyle[j].map[n] - 'a';
		}
		// only interpolate abrupt changes (e.g. flickering light in e1m1) if r_lerplightstyles >= 2
		if (r_lerplightstyles.value < 2.f && abs(n - k) >= ('m' - 'a') / 2)
			n = k;
		d_lightstylevalue[j] = (int)(k*22 + (n-k)*22*f);
		r_lightbuffer.lightstyles[j] = (k + (n-k)*f) * (22.f/256.f);
		//johnfitz
	}

	if (r_fullbright_cheatsafe)
		r_lightbuffer.lightstyles[0] = 1.f;
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

static GLuint gl_lightclustertexture;

typedef struct gpu_cluster_inputs_s {
	float		transposed_proj[16];
	float		view_matrix[16];
} gpu_cluster_inputs_t;

/*
=============
GLLight_CreateResources
=============
*/
void GLLight_CreateResources (void)
{
	glGenTextures (1, &gl_lightclustertexture);
	GL_BindNative (GL_TEXTURE0, GL_TEXTURE_3D, gl_lightclustertexture);
	GL_ObjectLabelFunc (GL_TEXTURE, gl_lightclustertexture, -1, "light clusters");
	GL_TexImage3DFunc (GL_TEXTURE_3D, 0, GL_RG32UI, LIGHT_TILES_X, LIGHT_TILES_Y, LIGHT_TILES_Z, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, NULL);
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/*
=============
GLLight_DeleteResources
=============
*/
void GLLight_DeleteResources (void)
{
	glDeleteTextures (1, &gl_lightclustertexture);
	gl_lightclustertexture = 0;
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int				i, j;
	GLuint			buf;
	GLbyte			*ofs;
	gpu_cluster_inputs_t cluster_inputs;

	r_framedata.numlights = 0;

	if (r_dynamic.value)
	{
		dlight_t *l;
		for (i = 0, l = cl_dlights; i < MAX_DLIGHTS; i++, l++)
		{
			gpulight_t *out;
			qboolean cull = false;

			if (l->spawn > cl.time)
			{
				l->die = 0.f;
				continue;
			}

			if (l->die < cl.time || !l->radius)
				continue;

			for (j = 0; j < 4; j++)
			{
				mplane_t *p = &frustum[j];
				if (DotProduct (p->normal, l->origin) - p->dist + l->radius < 0.f)
				{
					cull = true;
					break;
				}
			}
			if (cull)
				continue;

			out = &r_lightbuffer.lights[r_framedata.numlights++];
			out->pos[0]   = l->origin[0];
			out->pos[1]   = l->origin[1];
			out->pos[2]   = l->origin[2];
			out->radius   = l->radius;
			out->color[0] = l->color[0];
			out->color[1] = l->color[1];
			out->color[2] = l->color[2];
			out->minlight = l->minlight;
		}
	}

	GL_BeginGroup ("Light clustering");

	R_UploadFrameData ();

	for (i = 0; i < 16; i++)
		cluster_inputs.transposed_proj[i] = r_matproj[((i & 3) << 2) | (i >> 2)];
	memcpy (cluster_inputs.view_matrix, r_matview, 16 * sizeof (float));

	GL_UseProgram (glprogs.cluster_lights);
	GL_Upload (GL_UNIFORM_BUFFER, &cluster_inputs, sizeof (cluster_inputs), &buf, &ofs);
	GL_BindBufferRange (GL_UNIFORM_BUFFER, 1, buf, (GLintptr) ofs, sizeof (cluster_inputs));
	GL_BindImageTextureFunc (0, gl_lightclustertexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32UI);
	GL_DispatchComputeFunc ((LIGHT_TILES_X+7)/8, (LIGHT_TILES_Y+7)/8, LIGHT_TILES_Z);
	GL_MemoryBarrierFunc (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	GL_BindImageTextureFunc (0, gl_lightclustertexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RG32UI);

	GL_EndGroup ();
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t lightcolor; //johnfitz -- lit support via lordhavoc

static void InterpolateLightmap (vec3_t color, msurface_t *surf, int ds, int dt)
{
	byte *lightmap;
	int maps, line3, dsfrac = ds & 15, dtfrac = dt & 15, r00 = 0, g00 = 0, b00 = 0, r01 = 0, g01 = 0, b01 = 0, r10 = 0, g10 = 0, b10 = 0, r11 = 0, g11 = 0, b11 = 0;
	int scale;
	line3 = ((surf->extents[0]>>4)+1)*3;

	lightmap = surf->samples + ((dt>>4) * ((surf->extents[0]>>4)+1) + (ds>>4))*3; // LordHavoc: *3 for color

	for (maps = 0;maps < MAXLIGHTMAPS && surf->styles[maps] != 255;maps++)
	{
		scale = d_lightstylevalue[surf->styles[maps]];
		r00 += lightmap[      0] * scale; g00 += lightmap[      1] * scale; b00 += lightmap[      2] * scale;
		r01 += lightmap[      3] * scale; g01 += lightmap[      4] * scale; b01 += lightmap[      5] * scale;
		r10 += lightmap[line3+0] * scale; g10 += lightmap[line3+1] * scale; b10 += lightmap[line3+2] * scale;
		r11 += lightmap[line3+3] * scale; g11 += lightmap[line3+4] * scale; b11 += lightmap[line3+5] * scale;
		lightmap += ((surf->extents[0]>>4)+1) * ((surf->extents[1]>>4)+1)*3; // LordHavoc: *3 for colored lighting
	}

	color[0] = ((((((((r11-r10) * dsfrac) >> 4) + r10)-((((r01-r00) * dsfrac) >> 4) + r00)) * dtfrac) >> 4) + ((((r01-r00) * dsfrac) >> 4) + r00)) * (1.f/256.f);
	color[1] = ((((((((g11-g10) * dsfrac) >> 4) + g10)-((((g01-g00) * dsfrac) >> 4) + g00)) * dtfrac) >> 4) + ((((g01-g00) * dsfrac) >> 4) + g00)) * (1.f/256.f);
	color[2] = ((((((((b11-b10) * dsfrac) >> 4) + b10)-((((b01-b00) * dsfrac) >> 4) + b00)) * dtfrac) >> 4) + ((((b01-b00) * dsfrac) >> 4) + b00)) * (1.f/256.f);
}

/*
=============
RecursiveLightPoint -- johnfitz -- replaced entire function for lit support via lordhavoc
=============
*/
int RecursiveLightPoint (lightcache_t *cache, mnode_t *node, vec3_t rayorg, vec3_t start, vec3_t end, float *maxdist)
{
	float		front, back, frac;
	vec3_t		mid;

loc0:
	if (node->contents < 0)
		return false;		// didn't hit anything

// calculate mid point
	if (node->plane->type < 3)
	{
		front = start[node->plane->type] - node->plane->dist;
		back = end[node->plane->type] - node->plane->dist;
	}
	else
	{
		front = DotProduct(start, node->plane->normal) - node->plane->dist;
		back = DotProduct(end, node->plane->normal) - node->plane->dist;
	}

	// LordHavoc: optimized recursion
	if ((back < 0) == (front < 0))
//		return RecursiveLightPoint (cache, node->children[front < 0], rayorg, start, end, maxdist);
	{
		node = node->children[front < 0];
		goto loc0;
	}

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	if (RecursiveLightPoint (cache, node->children[front < 0], rayorg, start, mid, maxdist))
		return true;	// hit something
	else
	{
		unsigned int i;
		int ds, dt;
		msurface_t *surf;
	// check for impact on this node

		surf = cl.worldmodel->surfaces + node->firstsurface;
		for (i = 0;i < node->numsurfaces;i++, surf++)
		{
			float sfront, sback, dist;
			vec3_t raydelta;

			if (surf->flags & SURF_DRAWTILED)
				continue;	// no lightmaps

		// ericw -- added double casts to force 64-bit precision.
		// Without them the zombie at the start of jam3_ericw.bsp was
		// incorrectly being lit up in SSE builds.
			ds = (int) ((double) DoublePrecisionDotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
			dt = (int) ((double) DoublePrecisionDotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);

			if (ds < surf->texturemins[0] || dt < surf->texturemins[1])
				continue;

			ds -= surf->texturemins[0];
			dt -= surf->texturemins[1];

			if (ds > surf->extents[0] || dt > surf->extents[1])
				continue;

			if (surf->plane->type < 3)
			{
				sfront = rayorg[surf->plane->type] - surf->plane->dist;
				sback = end[surf->plane->type] - surf->plane->dist;
			}
			else
			{
				sfront = DotProduct(rayorg, surf->plane->normal) - surf->plane->dist;
				sback = DotProduct(end, surf->plane->normal) - surf->plane->dist;
			}
			VectorSubtract(end, rayorg, raydelta);
			dist = sfront / (sfront - sback) * VectorLength(raydelta);

			if (!surf->samples)
			{
				// We hit a surface that is flagged as lightmapped, but doesn't have actual lightmap info.
				// Instead of just returning black, we'll keep looking for nearby surfaces that do have valid samples.
				// This fixes occasional pitch-black models in otherwise well-lit areas in DOTM (e.g. mge1m1, mge4m1)
				// caused by overlapping surfaces with mixed lighting data.
				const float nearby = 8.f;
				dist += nearby;
				*maxdist = q_min(*maxdist, dist);
				continue;
			}

			if (dist < *maxdist)
			{
				cache->surfidx = surf - cl.worldmodel->surfaces + 1;
				cache->ds = ds;
				cache->dt = dt;
			}
			else
			{
				cache->surfidx = -1;
			}

			return true; // success
		}

	// go down back side
		return RecursiveLightPoint (cache, node->children[front >= 0], rayorg, mid, end, maxdist);
	}
}

/*
=============
R_LightPoint -- johnfitz -- replaced entire function for lit support via lordhavoc
=============
*/
int R_LightPoint (vec3_t p, float ofs, lightcache_t *cache)
{
	vec3_t		start, end;
	float		maxdist = 8192.f; //johnfitz -- was 2048

	if (!cl.worldmodel->lightdata)
	{
		lightcolor[0] = lightcolor[1] = lightcolor[2] = 255;
		return 255;
	}

	start[0] = p[0];
	start[1] = p[1];
	start[2] = p[2] + ofs;
	end[0] = start[0];
	end[1] = start[1];
	end[2] = start[2] - maxdist;

	lightcolor[0] = lightcolor[1] = lightcolor[2] = 0;

	if (cache->surfidx <= 0 // no cache or pitch black
		|| cache->surfidx > cl.worldmodel->numsurfaces
		|| fabsf (cache->pos[0] - p[0]) >= 1.f
		|| fabsf (cache->pos[1] - p[1]) >= 1.f
		|| fabsf (cache->pos[2] - p[2]) >= 1.f)
	{
		cache->surfidx = 0;
		VectorCopy (p, cache->pos);
		RecursiveLightPoint (cache, cl.worldmodel->nodes, start, start, end, &maxdist);
	}

	if (cache->surfidx > 0)
		InterpolateLightmap (lightcolor, cl.worldmodel->surfaces + cache->surfidx - 1, cache->ds, cache->dt);

	return ((lightcolor[0] + lightcolor[1] + lightcolor[2]) * (1.0f / 3.0f));
}
