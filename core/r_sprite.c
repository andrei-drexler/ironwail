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
//r_sprite.c -- sprite model rendering

#include "quakedef.h"

typedef struct spritevert_t {
	vec3_t		pos;
	float		uv[2];
} spritevert_t;

#define MAX_BATCH_SPRITES	1024

static int numbatchquads = 0;
static spritevert_t batchverts[4 * MAX_BATCH_SPRITES];
static gltexture_t *batchtexture;
static qmodel_t *batchmodel;
static qboolean batchshowtris;

static GLushort batchindices[6 * MAX_BATCH_SPRITES];
static qboolean batchindices_init = false;

/*
================
R_InitSpriteIndices
================
*/
static void R_InitSpriteIndices (void)
{
	int i;
	if (batchindices_init)
		return;

	for (i = 0; i < MAX_BATCH_SPRITES; i++)
	{
		batchindices[i*6 + 0] = i*4 + 0;
		batchindices[i*6 + 1] = i*4 + 1;
		batchindices[i*6 + 2] = i*4 + 2;
		batchindices[i*6 + 3] = i*4 + 0;
		batchindices[i*6 + 4] = i*4 + 2;
		batchindices[i*6 + 5] = i*4 + 3;
	}

	batchindices_init = true;
}

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currentent)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = (msprite_t *) currentent->model->cache.data;
	frame = currentent->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_DrawSprite: no such frame %d for '%s'\n", frame, currentent->model->name);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else if (psprite->frames[frame].type == SPR_ANGLED)
	{
		// erysdren - angled sprites code backported from FTEQW
		vec3_t axis[3];
		AngleVectors(currentent->angles, axis[0], axis[1], axis[2]);
		{
			float f = DotProduct(vpn, axis[0]);
			float r = DotProduct(vright, axis[0]);
			int dir = (atan2(r, f)+1.125*M_PI)*(4/M_PI);
			pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
			pspriteframe = pspritegroup->frames[dir&7];
		}
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currentent->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i = 0; i < (numframes-1); i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}

/*
=================
R_FlushSpriteInstances
=================
*/
static void R_FlushSpriteInstances (void)
{
	qboolean		dither;
	qboolean		showtris = batchshowtris;
	msprite_t		*psprite;
	GLuint			buf;
	GLbyte*			ofs;

	if (!numbatchquads)
		return;

	R_InitSpriteIndices ();
	psprite = (msprite_t *) batchmodel->cache.data;

	GL_BeginGroup (batchtexture->name);

	//johnfitz: offset decals
	if (psprite->type == SPR_ORIENTED)
		GL_PolygonOffset (OFFSET_DECAL);

	dither = (softemu == SOFTEMU_COARSE && !showtris);
	GL_UseProgram (glprogs.sprites[dither]);

	if (showtris)
		GL_SetState (GLS_BLEND_OPAQUE | GLS_NO_ZWRITE | GLS_CULL_BACK | GLS_ATTRIBS(2));
	else
		GL_SetState (GLS_BLEND_OPAQUE | GLS_CULL_BACK | GLS_ATTRIBS(2));

	GL_Bind (GL_TEXTURE0, showtris ? whitetexture : batchtexture);

	GL_Upload (GL_ARRAY_BUFFER, batchverts, sizeof(batchverts[0]) * 4 * numbatchquads, &buf, &ofs);
	GL_BindBuffer (GL_ARRAY_BUFFER, buf);
	GL_VertexAttribPointerFunc (0, 3, GL_FLOAT, GL_FALSE, sizeof(batchverts[0]), ofs + offsetof(spritevert_t, pos));
	GL_VertexAttribPointerFunc (1, 2, GL_FLOAT, GL_FALSE, sizeof(batchverts[0]), ofs + offsetof(spritevert_t, uv));

	GL_Upload (GL_ELEMENT_ARRAY_BUFFER, batchindices, sizeof(batchindices[0]) * 6 * numbatchquads, &buf, &ofs);
	GL_BindBuffer (GL_ELEMENT_ARRAY_BUFFER, buf);
	glDrawElements (GL_TRIANGLES, 6 * numbatchquads, GL_UNSIGNED_SHORT, ofs);

	//johnfitz: offset decals
	if (psprite->type == SPR_ORIENTED)
		GL_PolygonOffset (OFFSET_NONE);

	GL_EndGroup ();

	numbatchquads = 0;
}

/*
=================
R_DrawSpriteModel_Real
=================
*/
static void R_DrawSpriteModel_Real (entity_t *e, qboolean showtris)
{
	msprite_t		*psprite;
	mspriteframe_t	*frame;
	vec3_t			v_forward, v_right, v_up;
	float			*s_up, *s_right;
	float			angle, sr, cr;
	spritevert_t	*verts;
	float			scale = ENTSCALE_DECODE(e->scale);

	frame = R_GetSpriteFrame (e);
	psprite = (msprite_t *) e->model->cache.data;

	switch(psprite->type)
	{
	case SPR_VP_PARALLEL_UPRIGHT: //faces view plane, up is towards the heavens
		v_up[0] = 0;
		v_up[1] = 0;
		v_up[2] = 1;
		CrossProduct(vpn, v_up, v_right);
		VectorNormalizeFast(v_right);
		s_up = v_up;
		s_right = v_right;
		break;
	case SPR_FACING_UPRIGHT: //faces camera origin, up is towards the heavens
		VectorSubtract(e->origin, r_origin, v_forward);
		v_forward[2] = 0;
		VectorNormalizeFast(v_forward);
		v_right[0] = v_forward[1];
		v_right[1] = -v_forward[0];
		v_right[2] = 0;
		v_up[0] = 0;
		v_up[1] = 0;
		v_up[2] = 1;
		s_up = v_up;
		s_right = v_right;
		break;
	case SPR_VP_PARALLEL: //faces view plane, up is towards the top of the screen
		s_up = vup;
		s_right = vright;
		break;
	case SPR_ORIENTED: //pitch yaw roll are independent of camera
		AngleVectors (e->angles, v_forward, v_right, v_up);
		s_up = v_up;
		s_right = v_right;
		break;
	case SPR_VP_PARALLEL_ORIENTED: //faces view plane, but obeys roll value
		angle = e->angles[ROLL] * M_PI_DIV_180;
		sr = sin(angle);
		cr = cos(angle);
		v_right[0] = vright[0] * cr + vup[0] * sr;
		v_right[1] = vright[1] * cr + vup[1] * sr;
		v_right[2] = vright[2] * cr + vup[2] * sr;
		v_up[0] = vright[0] * -sr + vup[0] * cr;
		v_up[1] = vright[1] * -sr + vup[1] * cr;
		v_up[2] = vright[2] * -sr + vup[2] * cr;
		s_up = v_up;
		s_right = v_right;
		break;
	default:
		return;
	}

	if (numbatchquads)
		if (numbatchquads == countof(batchverts) / 4 || batchmodel != e->model || batchtexture != frame->gltexture)
			R_FlushSpriteInstances ();

	if (!numbatchquads)
	{
		batchmodel = e->model;
		batchtexture = frame->gltexture;
		batchshowtris = showtris;
	}
	verts = batchverts + numbatchquads * 4;
	++numbatchquads;

	VectorMA (e->origin, frame->down * scale, s_up, verts[0].pos);
	VectorMA (verts[0].pos, frame->left * scale, s_right, verts[0].pos);
	verts[0].uv[0] = 0.f;
	verts[0].uv[1] = frame->tmax;

	VectorMA (verts[0].pos, (frame->up - frame->down) * scale, s_up, verts[1].pos);
	verts[1].uv[0] = 0.f;
	verts[1].uv[1] = 0.f;

	VectorMA (verts[1].pos, (frame->right - frame->left) * scale, s_right, verts[2].pos);
	verts[2].uv[0] = frame->smax;
	verts[2].uv[1] = 0.f;

	VectorMA (verts[2].pos, (frame->down - frame->up) * scale, s_up, verts[3].pos);
	verts[3].uv[0] = frame->smax;
	verts[3].uv[1] = frame->tmax;
}

/*
=================
R_DrawSpriteModels
=================
*/
void R_DrawSpriteModels (entity_t **ents, int count)
{
	int i;
	for (i = 0; i < count; i++)
		R_DrawSpriteModel_Real (ents[i], false);
	R_FlushSpriteInstances ();
}

/*
=================
R_DrawSpriteModels_ShowTris
=================
*/
void R_DrawSpriteModels_ShowTris (entity_t **ents, int count)
{
	int i;
	for (i = 0; i < count; i++)
		R_DrawSpriteModel_Real (ents[i], true);
	R_FlushSpriteInstances ();
}
