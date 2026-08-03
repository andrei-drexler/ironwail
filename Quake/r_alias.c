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

//r_alias.c -- alias model rendering

#include "quakedef.h"

extern cvar_t gl_overbright_models, gl_fullbrights, r_lerpmodels, r_lerpmove; //johnfitz
extern cvar_t scr_fov, cl_gun_fovscale, cl_gun_x, cl_gun_y, cl_gun_z;
extern cvar_t r_oit;

//up to 16 color translated skins
gltexture_t *playertextures[MAX_SCOREBOARD]; //johnfitz -- changed to an array of pointers

const float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

extern vec3_t	lightcolor; //johnfitz -- replaces "float shadelight" for lit support

static float	entalpha; //johnfitz

//johnfitz -- struct for passing lerp information to drawing functions
typedef struct {
	short pose1;
	short pose2;
	float blend;
	vec3_t origin;
	vec3_t angles;
} lerpdata_t;
//johnfitz

#define MAX_ALIAS_INSTANCES 256

typedef struct aliasinstance_s {
	float		worldmatrix[12];
	vec3_t		lightcolor;
	float		alpha;
	int32_t		pose1;
	int32_t		pose2;
	float		blend;
	int32_t		padding;
} aliasinstance_t;

struct ibuf_s {
	int			count;
	entity_t	*ent;

	struct {
		float	matviewproj[16];
		vec3_t	eyepos;
		float	_pad;
		vec4_t	fog;
		float	dither;
		float	_padding[3];
	} global;
	aliasinstance_t inst[MAX_ALIAS_INSTANCES];
} ibuf;

/*
=================
R_SetupAliasFrame -- johnfitz -- rewritten to support lerping
=================
*/
void R_SetupAliasFrame (entity_t *e, aliashdr_t *paliashdr, lerpdata_t *lerpdata)
{
	int posenum, numposes;
	int frame = e->frame;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d for '%s'\n", frame, e->model->name);
		frame = 0;
	}

	posenum = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		e->lerptime = paliashdr->frames[frame].interval;
		posenum += (int)(cl.time / e->lerptime) % numposes;
	}
	else
		e->lerptime = 0.1;

	if (e->lerpflags & LERP_RESETANIM) //kill any lerp in progress
	{
		e->lerpstart = 0;
		e->previouspose = posenum;
		e->currentpose = posenum;
		e->lerpflags -= LERP_RESETANIM;
	}
	else if (e->currentpose != posenum) // pose changed, start new lerp
	{
		if (e->lerpflags & LERP_RESETANIM2) //defer lerping one more time
		{
			e->lerpstart = 0;
			e->previouspose = posenum;
			e->currentpose = posenum;
			e->lerpflags -= LERP_RESETANIM2;
		}
		else
		{
			e->lerpstart = cl.time;
			e->previouspose = e->currentpose;
			e->currentpose = posenum;
		}
	}

	//set up values
	if (r_lerpmodels.value && !(e->model->flags & MOD_NOLERP && r_lerpmodels.value != 2))
	{
		float s = (cls.demoplayback && cls.demospeed < 0.f) ? -1.f : 1.f;
		if (e->lerpflags & LERP_FINISH && numposes == 1)
			lerpdata->blend = CLAMP (0.0f, (float)(cl.time - e->lerpstart) / (e->lerpfinish - e->lerpstart), 1.0f);
		else
			lerpdata->blend = CLAMP (0.0f, (float)(cl.time - e->lerpstart) / e->lerptime * s, 1.0f);
		if (lerpdata->blend == 1.0f)
			e->previouspose = e->currentpose;
		lerpdata->pose1 = e->previouspose;
		lerpdata->pose2 = e->currentpose;
	}
	else //don't lerp
	{
		lerpdata->blend = 1;
		lerpdata->pose1 = posenum;
		lerpdata->pose2 = posenum;
	}
}

/*
=================
R_SetupEntityTransform -- johnfitz -- set up transform part of lerpdata
=================
*/
void R_SetupEntityTransform (entity_t *e, lerpdata_t *lerpdata)
{
	float blend;
	vec3_t d;
	int i;

	// if LERP_RESETMOVE, kill any lerps in progress
	if (e->lerpflags & LERP_RESETMOVE)
	{
		e->movelerpstart = 0;
		VectorCopy (e->origin, e->previousorigin);
		VectorCopy (e->origin, e->currentorigin);
		VectorCopy (e->angles, e->previousangles);
		VectorCopy (e->angles, e->currentangles);
		e->lerpflags -= LERP_RESETMOVE;
	}
	else if (!VectorCompare (e->origin, e->currentorigin) || !VectorCompare (e->angles, e->currentangles)) // origin/angles changed, start new lerp
	{
		e->movelerpstart = cl.time;
		VectorCopy (e->currentorigin, e->previousorigin);
		VectorCopy (e->origin,  e->currentorigin);
		VectorCopy (e->currentangles, e->previousangles);
		VectorCopy (e->angles,  e->currentangles);
	}

	//set up values
	if (r_lerpmove.value && e != &cl.viewent && e->lerpflags & LERP_MOVESTEP)
	{
		float s = (cls.demoplayback && cls.demospeed < 0.f) ? -1.f : 1.f;
		if (e->lerpflags & LERP_FINISH)
			blend = CLAMP (0.0f, (float)(cl.time - e->movelerpstart) / (e->lerpfinish - e->movelerpstart), 1.0f);
		else
			blend = CLAMP (0.0f, (float)(cl.time - e->movelerpstart) / 0.1f * s, 1.0f);

		//translation
		VectorSubtract (e->currentorigin, e->previousorigin, d);
		lerpdata->origin[0] = e->previousorigin[0] + d[0] * blend;
		lerpdata->origin[1] = e->previousorigin[1] + d[1] * blend;
		lerpdata->origin[2] = e->previousorigin[2] + d[2] * blend;

		//rotation
		VectorSubtract (e->currentangles, e->previousangles, d);
		for (i = 0; i < 3; i++)
		{
			if (d[i] > 180)  d[i] -= 360;
			if (d[i] < -180) d[i] += 360;
		}
		lerpdata->angles[0] = e->previousangles[0] + d[0] * blend;
		lerpdata->angles[1] = e->previousangles[1] + d[1] * blend;
		lerpdata->angles[2] = e->previousangles[2] + d[2] * blend;
	}
	else //don't lerp
	{
		VectorCopy (e->origin, lerpdata->origin);
		VectorCopy (e->angles, lerpdata->angles);
	}

	// chasecam
	if (chase_active.value && e == &cl_entities[cl.viewentity])
		lerpdata->angles[PITCH] *= 0.3f;
}

/*
=================
R_SetupAliasLighting -- johnfitz -- broken out from R_DrawAliasModel and rewritten
=================
*/
void R_SetupAliasLighting (entity_t	*e)
{
	vec3_t		dist;
	float		add;
	int			i;

	// if the initial trace is completely black, try again from above
	// this helps with models whose origin is slightly below ground level
	// (e.g. some of the candles in the DOTM start map)
	if (!R_LightPoint (e->origin, 0.f, &e->lightcache))
		R_LightPoint (e->origin, e->model->maxs[2] * 0.5f, &e->lightcache);

	//add dlights
	for (i=0; i<r_framedata.numlights; i++)
	{
		gpulight_t *l = &r_lightbuffer.lights[i];
		VectorSubtract (e->origin, l->pos, dist);
		add = DotProduct (dist, dist);
		if (l->radius * l->radius > add)
			VectorMA (lightcolor, l->radius - sqrtf (add), l->color, lightcolor);
	}

	// minimum light value on gun (24)
	if (e == &cl.viewent)
	{
		add = 72.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			add *= 1.0f / 3.0f;
			lightcolor[0] += add;
			lightcolor[1] += add;
			lightcolor[2] += add;
		}
	}

	// minimum light value on players (8)
	if (e > cl_entities && e <= cl_entities + cl.maxclients)
	{
		add = 24.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			add *= 1.0f / 3.0f;
			lightcolor[0] += add;
			lightcolor[1] += add;
			lightcolor[2] += add;
		}
	}

	// clamp lighting so it doesn't overbright as much (96)
	if (gl_overbright_models.value)
	{
		add = lightcolor[0] + lightcolor[1] + lightcolor[2];
		if (add > 288.0f)
			VectorScale(lightcolor, 288.0f / add, lightcolor);
	}
	//hack up the brightness when fullbrights but no overbrights (256)
	else if (e->model->flags & MOD_FBRIGHTHACK && gl_fullbrights.value)
	{
		lightcolor[0] = 256.0f;
		lightcolor[1] = 256.0f;
		lightcolor[2] = 256.0f;
	}

	VectorScale (lightcolor, 1.0f / 200.0f, lightcolor);
}

/*
=================
R_FlushAliasInstances
=================
*/
void R_FlushAliasInstances (qboolean showtris)
{
	extern cvar_t r_softemu_mdl_warp;
	qmodel_t* model;
	aliashdr_t* mainhdr, *hdr;
	qboolean	alphatest, translucent, oit;
	int			totalverts;
	int			poseverttype;
	int			skinnum, anim, mode;
	unsigned	state, opaque_state, transparent_state;
	GLuint		buf;
	GLbyte* ofs;
	size_t		ibuf_size;
	GLuint		buffers[2];
	GLintptr	offsets[2];
	GLsizeiptr	sizes[2];
	gltexture_t* textures[2];

	if (!ibuf.count)
		return;

	model = ibuf.ent->model;
	mainhdr = (aliashdr_t*)Mod_Extradata (model);
	anim = (int)(cl.time * 10) & 3;

	GL_BeginGroup (model->name);
	poseverttype = mainhdr->poseverttype;

	alphatest = model->flags & MF_HOLEY ? 1 : 0;
	translucent = !ENTALPHA_OPAQUE (ibuf.ent->alpha);
	oit = translucent && R_GetEffectiveAlphaMode () == ALPHAMODE_OIT;
	switch (softemu)
	{
	case SOFTEMU_BANDED:
		mode = r_softemu_mdl_warp.value != 0.f ? ALIASSHADER_NOPERSP : ALIASSHADER_STANDARD;
		break;
	case SOFTEMU_COARSE:
		mode = r_softemu_mdl_warp.value > 0.f ? ALIASSHADER_NOPERSP : ALIASSHADER_DITHER;
		break;
	default:
		mode = r_softemu_mdl_warp.value > 0.f ? ALIASSHADER_NOPERSP : ALIASSHADER_STANDARD;
		break;
	}
	GL_UseProgram (glprogs.alias[oit][mode][alphatest][poseverttype]);

	if (poseverttype == PV_IQM)
		state = GLS_CULL_BACK | GLS_ATTRIBS (5);
	else
		state = GLS_CULL_BACK | GLS_ATTRIBS (1);

	opaque_state = (state | GLS_BLEND_OPAQUE) & ~(GLS_BLEND_ALPHA_OIT | GLS_NO_ZWRITE);
	transparent_state = (state | GLS_BLEND_ALPHA) & ~(GLS_BLEND_OPAQUE | GLS_CULL_BACK);

	if (translucent)
	{
		GL_SetState ((state | GLS_BLEND_ALPHA_OIT | GLS_NO_ZWRITE) & ~GLS_CULL_BACK);
	}

	memcpy (ibuf.global.matviewproj, r_matviewproj, sizeof (r_matviewproj));
	memcpy (ibuf.global.eyepos, r_refdef.vieworg, sizeof (r_refdef.vieworg));
	memcpy (ibuf.global.fog, r_framedata.fogdata, 3 * sizeof (float));
	ibuf.global.fog[3] =
		gl_overbright_models.value ?
		-fabs (r_framedata.fogdata[3]) :
		fabs (r_framedata.fogdata[3])
		;
	ibuf.global.dither = r_framedata.screendither;

	ibuf_size = sizeof (ibuf.global) + sizeof (ibuf.inst[0]) * ibuf.count;
	GL_Upload (GL_SHADER_STORAGE_BUFFER, &ibuf.global, ibuf_size, &buf, &ofs);

	for (hdr = mainhdr, totalverts = 0; hdr; hdr = Mod_NextSurface (hdr))
		totalverts += hdr->numverts_vbo;

	buffers[0] = buf;
	offsets[0] = (GLintptr)ofs;
	sizes[0] = ibuf_size;
	switch (poseverttype)
	{
	case PV_IQM:
		buffers[1] = model->meshvbo; offsets[1] = mainhdr->vboposeofs; sizes[1] = sizeof (bonepose_t) * mainhdr->numbones * mainhdr->numposes;
		break;
	case PV_MD3:
		buffers[1] = model->meshvbo; offsets[1] = mainhdr->vbovertofs; sizes[1] = sizeof (md3pose_t) * totalverts * mainhdr->numposes;
		break;
	case PV_QUAKE1:
		buffers[1] = model->meshvbo; offsets[1] = mainhdr->vbovertofs; sizes[1] = sizeof (meshxyz_t) * totalverts * mainhdr->numposes;
		break;
	default:
		return;
	}

	GL_BindBuffer (GL_ARRAY_BUFFER, model->meshvbo);
	GL_BindBuffer (GL_ELEMENT_ARRAY_BUFFER, model->meshindexesvbo);
	GL_BindBuffersRange (GL_SHADER_STORAGE_BUFFER, 1, 2, buffers, offsets, sizes);

	if (poseverttype == PV_IQM)
	{
		GL_VertexAttribPointerFunc (0, 3, GL_FLOAT, GL_FALSE, sizeof (iqmvert_t), (void*)(mainhdr->vbovertofs + offsetof (iqmvert_t, xyz)));
		GL_VertexAttribPointerFunc (1, 4, GL_BYTE, GL_TRUE, sizeof (iqmvert_t), (void*)(mainhdr->vbovertofs + offsetof (iqmvert_t, norm)));
		GL_VertexAttribPointerFunc (2, 2, GL_FLOAT, GL_FALSE, sizeof (iqmvert_t), (void*)(mainhdr->vbovertofs + offsetof (iqmvert_t, st)));
		GL_VertexAttribPointerFunc (3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof (iqmvert_t), (void*)(mainhdr->vbovertofs + offsetof (iqmvert_t, weight)));
		GL_VertexAttribIPointerFunc (4, 4, GL_UNSIGNED_BYTE, sizeof (iqmvert_t), (void*)(mainhdr->vbovertofs + offsetof (iqmvert_t, idx)));
	}
	else // PV_QUAKE1 || PV_MD3
	{
		GL_VertexAttribPointerFunc (0, 2, GL_FLOAT, GL_FALSE, sizeof (meshst_t), (void*)mainhdr->vbostofs);
	}

	if (!translucent)
		GL_SetState (opaque_state);

	for (hdr = mainhdr; hdr; hdr = Mod_NextSurface (hdr))
	{
		skinnum = ibuf.ent->skinnum;
		if ((skinnum >= hdr->numskins) || (skinnum < 0)) skinnum = 0;
		textures[0] = hdr->gltextures[skinnum][anim];
		if (!textures[0]) continue;

		if (!translucent && (textures[0]->flags & TEXPREF_ALPHAPIXELS))
		{
			continue;
		}
		textures[1] = hdr->fbtextures[skinnum][anim];
		if (hdr == mainhdr && ibuf.ent->colormap != vid.colormap && !gl_nocolors.value)
			if (CL_IsPlayerEnt (ibuf.ent)) textures[0] = playertextures[ibuf.ent - cl_entities - 1];
		if (!gl_fullbrights.value) textures[1] = blacktexture;
		if (r_lightmap_cheatsafe) { textures[0] = greytexture; textures[1] = blacktexture; }
		if (!textures[1]) textures[1] = blacktexture;
		if (showtris) { textures[0] = blacktexture; textures[1] = whitetexture; }

		GL_BindTextures (0, 2, textures);
		GL_DrawElementsInstancedFunc (GL_TRIANGLES, hdr->numindexes, GL_UNSIGNED_SHORT, (void*)hdr->eboofs, ibuf.count);
		rs_aliaspasses += hdr->numtris * ibuf.count;
	}

	if (!translucent)
	{
		GL_SetState (transparent_state);

		for (hdr = mainhdr; hdr; hdr = Mod_NextSurface (hdr))
		{
			skinnum = ibuf.ent->skinnum;
			if ((skinnum >= hdr->numskins) || (skinnum < 0)) skinnum = 0;
			textures[0] = hdr->gltextures[skinnum][anim];
			if (!textures[0]) continue;

			if (!(textures[0]->flags & TEXPREF_ALPHAPIXELS))
			{ 
				continue;
			}

			textures[1] = hdr->fbtextures[skinnum][anim];
			if (hdr == mainhdr && ibuf.ent->colormap != vid.colormap && !gl_nocolors.value)
				if (CL_IsPlayerEnt (ibuf.ent)) textures[0] = playertextures[ibuf.ent - cl_entities - 1];
			if (!gl_fullbrights.value) textures[1] = blacktexture;
			if (r_lightmap_cheatsafe) { textures[0] = greytexture; textures[1] = blacktexture; }
			if (!textures[1]) textures[1] = blacktexture;
			if (showtris) { textures[0] = blacktexture; textures[1] = whitetexture; }

			GL_BindTextures (0, 2, textures);
			GL_DrawElementsInstancedFunc (GL_TRIANGLES, hdr->numindexes, GL_UNSIGNED_SHORT, (void*)hdr->eboofs, ibuf.count);
			rs_aliaspasses += hdr->numtris * ibuf.count;
		}

	}

	ibuf.count = 0;
	GL_EndGroup ();
}

/*
=================
R_Alias_CanAddToBatch
=================
*/
static qboolean R_Alias_CanAddToBatch (const entity_t *e)
{
	// empty batch
	if (!ibuf.count)
		return true;

	// full batch
	if (ibuf.count == countof (ibuf.inst))
		return false;

	// different models/skins
	if (ibuf.ent->model != e->model || ibuf.ent->skinnum != e->skinnum)
		return false;

	// players have custom colors
	if (!gl_nocolors.value && CL_IsPlayerEnt (ibuf.ent))
		return false;

	return true;
}

/*
=================
R_DrawAliasModel_Real
=================
*/
static void R_DrawAliasModel_Real (entity_t *e, qboolean showtris)
{
	aliashdr_t	*paliashdr, *hdr;
	lerpdata_t	lerpdata;
	float		fovscale = 1.0f;
	float		model_matrix[16];
	aliasinstance_t	*instance;
	int			totalverts;

	//
	// setup pose/lerp data -- do it first so we don't miss updates due to culling
	//
	paliashdr = (aliashdr_t *)Mod_Extradata (e->model);

	R_SetupAliasFrame (e, paliashdr, &lerpdata);
	R_SetupEntityTransform (e, &lerpdata);

	if (lerpdata.pose1 == lerpdata.pose2)
		lerpdata.blend = 0.f;

	//
	// viewmodel adjustments (position, fov distortion correction)
	//
	if (e == &cl.viewent)
	{
		if (r_refdef.basefov > 90.f && cl_gun_fovscale.value)
		{
			fovscale = tan (r_refdef.basefov * (0.5f * M_PI / 180.f));
			fovscale = 1.f + (fovscale - 1.f) * cl_gun_fovscale.value;
		}

		VectorMA (lerpdata.origin, cl_gun_x.value * paliashdr->scale[0] * fovscale,	vright,	lerpdata.origin);
		VectorMA (lerpdata.origin, cl_gun_y.value * paliashdr->scale[1] * fovscale,	vup,	lerpdata.origin);
		VectorMA (lerpdata.origin, cl_gun_z.value * paliashdr->scale[2],			vpn,	lerpdata.origin);
	}

	//
	// cull it
	//
	if (R_CullModelForEntity(e))
		return;

	//
	// transform it
	//
	R_EntityMatrix (model_matrix, lerpdata.origin, lerpdata.angles, e->scale);
	ApplyTranslation (model_matrix, paliashdr->scale_origin[0], paliashdr->scale_origin[1] * fovscale, paliashdr->scale_origin[2] * fovscale);
	ApplyScale (model_matrix, paliashdr->scale[0], paliashdr->scale[1] * fovscale, paliashdr->scale[2] * fovscale);

	//
	// set up for alpha blending
	//
	if (r_lightmap_cheatsafe) //no alpha in drawflat or lightmap mode
		entalpha = 1;
	else
		entalpha = ENTALPHA_DECODE(e->alpha);

	if (entalpha == 0)
		return;

	//
	// set up lighting
	//
	rs_aliaspolys += paliashdr->numtris;
	R_SetupAliasLighting (e);

	//
	// draw it
	//

	if (r_fullbright_cheatsafe || showtris)
		lightcolor[0] = lightcolor[1] = lightcolor[2] = 0.5f;

	if (showtris)
		entalpha = 1.f;

	if (!R_Alias_CanAddToBatch (e))
		R_FlushAliasInstances (showtris);

	if (!ibuf.count)
		ibuf.ent = e;

	instance = &ibuf.inst[ibuf.count++];

	MatrixTranspose4x3 (model_matrix, instance->worldmatrix);

	instance->lightcolor[0] = lightcolor[0];
	instance->lightcolor[1] = lightcolor[1];
	instance->lightcolor[2] = lightcolor[2];
	instance->alpha = entalpha;
	instance->pose1 = lerpdata.pose1;
	instance->pose2 = lerpdata.pose2;
	instance->blend = lerpdata.blend;

	for (hdr = paliashdr, totalverts = 0; hdr; hdr = Mod_NextSurface (hdr))
		totalverts += hdr->numverts_vbo;

	if (paliashdr->poseverttype == PV_QUAKE1 || paliashdr->poseverttype == PV_MD3)
	{
		instance->pose1 *= totalverts;
		instance->pose2 *= totalverts;
	}
	else if (paliashdr->poseverttype == PV_IQM)
	{
		instance->pose1 *= paliashdr->numbones;
		instance->pose2 *= paliashdr->numbones;
	}
}

/*
=================
R_DrawAliasModels
=================
*/
void R_DrawAliasModels (entity_t **ents, int count)
{
	int i;
	for (i = 0; i < count; i++)
		R_DrawAliasModel_Real (ents[i], false);
	R_FlushAliasInstances (false);
}

/*
=================
R_DrawAliasModels_ShowTris
=================
*/
void R_DrawAliasModels_ShowTris (entity_t **ents, int count)
{
	int i;
	for (i = 0; i < count; i++)
		R_DrawAliasModel_Real (ents[i], true);
	R_FlushAliasInstances (true);
}
