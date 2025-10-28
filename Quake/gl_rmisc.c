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
// r_misc.c

#include "quakedef.h"

//johnfitz -- new cvars
extern cvar_t r_clearcolor;
extern cvar_t r_flatlightstyles;
extern cvar_t r_lerplightstyles;
extern cvar_t gl_fullbrights;
extern cvar_t gl_farclip;
extern cvar_t gl_overbright_models;
extern cvar_t r_waterwarp;
extern cvar_t r_oldskyleaf;
extern cvar_t r_drawworld;
extern cvar_t r_showtris;
extern cvar_t r_showbboxes;
extern cvar_t r_showbboxes_think;
extern cvar_t r_showbboxes_health;
extern cvar_t r_showbboxes_links;
extern cvar_t r_showbboxes_targets;
extern cvar_t r_showfields;
extern cvar_t r_showfields_align;
extern cvar_t r_lerpmodels;
extern cvar_t r_lerpmove;
extern cvar_t r_nolerp_list;
extern cvar_t r_noshadow_list;
//johnfitz
extern cvar_t gl_zfix; // QuakeSpasm z-fighting fix
extern cvar_t r_alphasort;
extern cvar_t r_oit;
extern cvar_t r_dither;

#if defined(USE_SIMD)
extern cvar_t r_simd;
#endif
qboolean use_simd;

extern gltexture_t *playertextures[MAX_SCOREBOARD]; //johnfitz

extern char r_showbboxes_filter_strings[MAXCMDLINE];
extern qboolean r_showbboxes_filter_byindex;

/*
====================
R_ShowbboxesFilter_f
====================
*/
static void R_ShowbboxesFilter_f (void)
{
	if (Cmd_Argc () >= 2)
	{
		int i, len, ofs;
		r_showbboxes_filter_byindex = false;
		for (i = 1, ofs = 0; i < Cmd_Argc (); i++)
		{
			const char *arg = Cmd_Argv (i);
			if (!*arg)
				continue;
			len = strlen (arg) + 1;
			if (ofs + len + 1 > (int) countof (r_showbboxes_filter_strings))
			{
				Con_Warning ("overflow at \"%s\"\n", arg);
				break;
			}
			r_showbboxes_filter_byindex |= (arg[0] == '#');
			memcpy (&r_showbboxes_filter_strings[ofs], arg, len);
			ofs += len;
		}
		r_showbboxes_filter_strings[ofs++] = '\0';
	}
	else
	{
		const char *p = r_showbboxes_filter_strings;
		Con_SafePrintf ("\"r_showbboxes_filter\" is");
		if (!*p)
			Con_SafePrintf (" \"\"");
		else do
		{
			Con_SafePrintf (" \"%s\"", p);
			p += strlen (p) + 1;
		} while (*p);
		Con_SafePrintf ("\n");
	}
}

/*
====================
R_ShowbboxesFilter_Completion_f -- tab completion for r_showbboxes_filter
====================
*/
static void R_ShowbboxesFilter_Completion_f (const char *partial)
{
	extern edict_t *sv_player;
	extern edict_t **bbox_linked;
	qcvm_t	*oldvm;
	edict_t	*ed;
	int		i;

	if (!sv.active)
		return;

	PR_PushQCVM (&sv.qcvm, &oldvm);

	if (*partial == '#')
	{
		for (i = 0; i < (int) VEC_SIZE (bbox_linked); i++)
		{
			ed = bbox_linked[i];
			Con_AddToTabList (va ("#%d", NUM_FOR_EDICT (ed)), partial, PR_GetString (ed->v.classname));
		}
	}
	else
	{
		for (i = 1, ed = NEXT_EDICT (qcvm->edicts); i < qcvm->num_edicts; i++, ed = NEXT_EDICT (ed))
		{
			const char *name;
			if (ed == sv_player || ed->free || !ed->v.classname)
				continue;
			name = PR_GetString (ed->v.classname);
			if (*name)
				Con_AddToTabList (name, partial, "#");
		}
	}

	PR_PopQCVM (oldvm);
}

/*
====================
R_ShowbboxesFilterClear_f
====================
*/
static void R_ShowbboxesFilterClear_f (void)
{
	r_showbboxes_filter_strings[0] = '\0';
	r_showbboxes_filter_byindex = false;
}

/*
====================
GL_Fullbrights_f -- johnfitz
====================
*/
static void GL_Fullbrights_f (cvar_t *var)
{
	TexMgr_ReloadNobrightImages ();
}

/*
====================
R_SetClearColor_f -- johnfitz
====================
*/
static void R_SetClearColor_f (cvar_t *var)
{
	byte	*rgb;
	int		s;

	s = (int)r_clearcolor.value & 0xFF;
	rgb = (byte*)(d_8to24table + s);
	glClearColor (rgb[0]/255.0,rgb[1]/255.0,rgb[2]/255.0,0);
}

/*
===============
R_Model_ExtraFlags_List_f -- johnfitz -- called when r_nolerp_list or r_noshadow_list cvar changes
===============
*/
static void R_Model_ExtraFlags_List_f (cvar_t *var)
{
	int i;
	for (i=0; i < MAX_MODELS; i++)
		Mod_SetExtraFlags (cl.model_precache[i]);
}

#if defined(USE_SIMD)
/*
====================
R_SIMD_f
====================
*/
static void R_SIMD_f (cvar_t *var)
{
#if defined(USE_SSE2)
	use_simd = SDL_HasSSE() && SDL_HasSSE2() && (var->value != 0.0f);
#else
	#error not implemented
#endif
}
#endif

/*
====================
R_SetWateralpha_f -- ericw
====================
*/
static void R_SetWateralpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWWATER) && var->value < 1)
		Con_Warning("Map does not appear to be water-vised\n");
	map_wateralpha = var->value;
	map_fallbackalpha = var->value;
}

/*
====================
R_SetLavaalpha_f -- ericw
====================
*/
static void R_SetLavaalpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWLAVA) && var->value && var->value < 1)
		Con_Warning("Map does not appear to be lava-vised\n");
	map_lavaalpha = var->value;
}

/*
====================
R_SetTelealpha_f -- ericw
====================
*/
static void R_SetTelealpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWTELE) && var->value && var->value < 1)
		Con_Warning("Map does not appear to be tele-vised\n");
	map_telealpha = var->value;
}

/*
====================
R_SetSlimealpha_f -- ericw
====================
*/
static void R_SetSlimealpha_f (cvar_t *var)
{
	if (cls.signon == SIGNONS && cl.worldmodel && !(cl.worldmodel->contentstransparent&SURF_DRAWSLIME) && var->value && var->value < 1)
		Con_Warning("Map does not appear to be slime-vised\n");
	map_slimealpha = var->value;
}

/*
====================
GL_WaterAlphaForTextureType
====================
*/
float GL_WaterAlphaForTextureType (textype_t type)
{
	if (type == TEXTYPE_LAVA)
		return map_lavaalpha > 0 ? map_lavaalpha : map_fallbackalpha;
	else if (type == TEXTYPE_TELE)
		return map_telealpha > 0 ? map_telealpha : map_fallbackalpha;
	else if (type == TEXTYPE_SLIME)
		return map_slimealpha > 0 ? map_slimealpha : map_fallbackalpha;
	else
		return map_wateralpha;
}


/*
===============
R_Init
===============
*/
void R_Init (void)
{
	cmd_function_t *cmd;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);
	cmd = Cmd_AddCommand ("r_showbboxes_filter", R_ShowbboxesFilter_f);
	if (cmd)
		cmd->completion = R_ShowbboxesFilter_Completion_f;
	Cmd_AddCommand ("r_showbboxes_filter_clear", R_ShowbboxesFilterClear_f);

	Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariable (&r_lightmap);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_SetCallback (&r_wateralpha, R_SetWateralpha_f);
	Cvar_RegisterVariable (&r_litwater);
	Cvar_RegisterVariable (&r_dynamic);
	Cvar_RegisterVariable (&r_novis);
#if defined(USE_SIMD)
	Cvar_RegisterVariable (&r_simd);
	Cvar_SetCallback (&r_simd, R_SIMD_f);
	R_SIMD_f(&r_simd);
#endif
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_pos);
	Cvar_RegisterVariable (&r_alphasort);
	Cvar_RegisterVariable (&r_oit);
	Cvar_RegisterVariable (&r_dither);

	Cvar_RegisterVariable (&gl_finish);
	Cvar_RegisterVariable (&gl_clear);
	Cvar_RegisterVariable (&gl_polyblend);
	Cvar_RegisterVariable (&gl_playermip);
	Cvar_RegisterVariable (&gl_nocolors);

	//johnfitz -- new cvars
	Cvar_RegisterVariable (&r_clearcolor);
	Cvar_SetCallback (&r_clearcolor, R_SetClearColor_f);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariable (&r_flatlightstyles);
	Cvar_RegisterVariable (&r_lerplightstyles);
	Cvar_RegisterVariable (&r_oldskyleaf);
	Cvar_RegisterVariable (&r_drawworld);
	Cvar_RegisterVariable (&r_showtris);
	Cvar_RegisterVariable (&r_showbboxes);
	Cvar_RegisterVariable (&r_showbboxes_think);
	Cvar_RegisterVariable (&r_showbboxes_health);
	Cvar_RegisterVariable (&r_showbboxes_links);
	Cvar_RegisterVariable (&r_showbboxes_targets);
	Cvar_RegisterVariable (&r_showfields);
	Cvar_RegisterVariable (&r_showfields_align);
	Cvar_RegisterVariable (&gl_farclip);
	Cvar_RegisterVariable (&gl_fullbrights);
	Cvar_SetCallback (&gl_fullbrights, GL_Fullbrights_f);
	Cvar_RegisterVariable (&gl_overbright_models);
	Cvar_RegisterVariable (&r_lerpmodels);
	Cvar_RegisterVariable (&r_lerpmove);
	Cvar_RegisterVariable (&r_nolerp_list);
	Cvar_SetCallback (&r_nolerp_list, R_Model_ExtraFlags_List_f);
	Cvar_RegisterVariable (&r_noshadow_list);
	Cvar_SetCallback (&r_noshadow_list, R_Model_ExtraFlags_List_f);
	//johnfitz

	Cvar_RegisterVariable (&gl_zfix); // QuakeSpasm z-fighting fix
	Cvar_RegisterVariable (&r_lavaalpha);
	Cvar_RegisterVariable (&r_telealpha);
	Cvar_RegisterVariable (&r_slimealpha);
	Cvar_RegisterVariable (&r_scale);
	Cvar_SetCallback (&r_lavaalpha, R_SetLavaalpha_f);
	Cvar_SetCallback (&r_telealpha, R_SetTelealpha_f);
	Cvar_SetCallback (&r_slimealpha, R_SetSlimealpha_f);

	R_InitParticles ();
	R_SetClearColor_f (&r_clearcolor); //johnfitz

	Sky_Init (); //johnfitz
	Fog_Init (); //johnfitz
}

/*
===============
R_TranslatePlayerSkin -- johnfitz -- rewritten.  also, only handles new colors, not new skins
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	int			top, bottom;

	top = (cl.scores[playernum].colors & 0xf0)>>4;
	bottom = cl.scores[playernum].colors &15;

	//FIXME: if gl_nocolors is on, then turned off, the textures may be out of sync with the scoreboard colors.
	if (!gl_nocolors.value)
	{
		if (playertextures[playernum])
			TexMgr_ReloadImage (playertextures[playernum], top, bottom);
	}
}

/*
===============
R_TranslateNewPlayerSkin -- johnfitz -- split off of TranslatePlayerSkin -- this is called when
the skin or model actually changes, instead of just new colors
added bug fix from bengt jardup
===============
*/
void R_TranslateNewPlayerSkin (int playernum)
{
	char		name[64];
	byte		*pixels;
	aliashdr_t	*paliashdr;
	entity_t	*e;
	int		skinnum;

//get correct texture pixels
	e = &cl_entities[1+playernum];

	if (!e->model || e->model->type != mod_alias)
		return;

	paliashdr = (aliashdr_t *)Mod_Extradata (e->model);

	skinnum = e->skinnum;

	//TODO: move these tests to the place where skinnum gets received from the server
	if (skinnum < 0 || skinnum >= paliashdr->numskins)
	{
		Con_DPrintf("(%d): Invalid player skin #%d\n", playernum, skinnum);
		skinnum = 0;
	}

	pixels = (byte *)paliashdr + paliashdr->texels[skinnum]; // This is not a persistent place!

//upload new image
	q_snprintf(name, sizeof(name), "player_%i", playernum);
	playertextures[playernum] = TexMgr_LoadImage (e->model, name, paliashdr->skinwidth, paliashdr->skinheight,
		paliashdr->gltextures[skinnum][0]->source_format, pixels, paliashdr->gltextures[skinnum][0]->source_file, paliashdr->gltextures[skinnum][0]->source_offset, TEXPREF_PAD | TEXPREF_OVERWRITE);

//now recolor it
	R_TranslatePlayerSkin (playernum);
}

/*
===============
R_NewGame -- johnfitz -- handle a game switch
===============
*/
void R_NewGame (void)
{
	int i;

	//clear playertexture pointers (the textures themselves were freed by texmgr_newgame)
	for (i=0; i<MAX_SCOREBOARD; i++)
		playertextures[i] = NULL;
}

/*
=============
R_ParseWorldspawn

called at map load
=============
*/
static void R_ParseWorldspawn (void)
{
	char key[128], value[4096];
	const char *data;

	map_fallbackalpha = r_wateralpha.value;
	map_wateralpha = (cl.worldmodel->contentstransparent&SURF_DRAWWATER)?r_wateralpha.value:1;
	map_lavaalpha = (cl.worldmodel->contentstransparent&SURF_DRAWLAVA)?r_lavaalpha.value:1;
	map_telealpha = (cl.worldmodel->contentstransparent&SURF_DRAWTELE)?r_telealpha.value:1;
	map_slimealpha = (cl.worldmodel->contentstransparent&SURF_DRAWSLIME)?r_slimealpha.value:1;

	data = COM_Parse(cl.worldmodel->entities);
	if (!data)
		return; // error
	if (com_token[0] != '{')
		return; // error

	while (1)
	{
		data = COM_Parse(data);
		if (!data)
			return; // error
		if (com_token[0] == '}')
			break; // end of worldspawn
		if (com_token[0] == '_')
			q_strlcpy(key, com_token + 1, sizeof(key));
		else
			q_strlcpy(key, com_token, sizeof(key));
		while (key[0] && key[strlen(key)-1] == ' ') // remove trailing spaces
			key[strlen(key)-1] = 0;
		data = COM_ParseEx(data, CPE_ALLOWTRUNC);
		if (!data)
			return; // error
		q_strlcpy(value, com_token, sizeof(value));

		if (!strcmp("wateralpha", key))
			map_wateralpha = atof(value);

		if (!strcmp("lavaalpha", key))
			map_lavaalpha = atof(value);

		if (!strcmp("telealpha", key))
			map_telealpha = atof(value);

		if (!strcmp("slimealpha", key))
			map_slimealpha = atof(value);
	}
}


/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	R_ClearEfrags ();
	r_viewleaf = NULL;
	R_ClearParticles ();
	VEC_CLEAR (r_pointfile);

	GL_BuildLightmaps ();
	GL_BuildBModelVertexBuffer ();
	GL_BuildBModelMarkBuffers ();
	//ericw -- no longer load alias models into a VBO here, it's done in Mod_LoadAliasModel

	r_framecount = 0; //johnfitz -- paranoid?
	r_visframecount = 0; //johnfitz -- paranoid?

	Sky_NewMap (); //johnfitz -- skybox in worldspawn
	Fog_NewMap (); //johnfitz -- global fog in worldspawn
	R_ParseWorldspawn (); //ericw -- wateralpha, lavaalpha, telealpha, slimealpha in worldspawn

	// Load pointfile if map has no vis data and either developer mode is on or the game was started from a map editing tool
	if (developer.value || map_checks.value)
		if (!cl.worldmodel->visdata && COM_FileExists (va ("maps/%s.pts", cl.mapname), NULL))
			Cbuf_AddText ("pointfile leak\n");
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int		i;
	double	start, stop, time;

	if (cls.state != ca_connected)
	{
		Con_Printf("Not connected to a server\n");
		return;
	}

	start = Sys_DoubleTime ();
	for (i = 0; i < 128; i++)
	{
		GL_BeginRendering(&glx, &gly, &glwidth, &glheight);
		r_refdef.viewangles[1] = i*(360.0/128.0);
		R_RenderView ();
		GL_EndRendering ();
	}

	glFinish ();
	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%lf seconds (%.1lf fps)\n", time, 128/time);
}

static GLuint current_array_buffer;
static GLuint current_element_array_buffer;
static GLuint current_shader_storage_buffer;
static GLuint current_draw_indirect_buffer;

/*
====================
GL_CreateBuffer
====================
*/
GLuint GL_CreateBuffer (GLenum target, GLenum usage, const char *name, size_t size, const void *data)
{
	GLuint buffer;
	GL_GenBuffersFunc (1, &buffer);
	GL_BindBuffer (target, buffer);
	if (name)
		GL_ObjectLabelFunc (GL_BUFFER, buffer, -1, name);
	GL_BufferDataFunc (target, size, data, usage);
	return buffer;
}

/*
====================
GL_BindBuffer

glBindBuffer wrapper
====================
*/
void GL_BindBuffer (GLenum target, GLuint buffer)
{
	GLuint *cache;

	switch (target)
	{
		case GL_ARRAY_BUFFER:
			cache = &current_array_buffer;
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			cache = &current_element_array_buffer;
			break;
		case GL_SHADER_STORAGE_BUFFER:
			cache = &current_shader_storage_buffer;
			break;
		case GL_DRAW_INDIRECT_BUFFER:
			cache = &current_draw_indirect_buffer;
			break;
		default:
			goto apply;
	}

	if (*cache != buffer)
	{
		*cache = buffer;
	apply:
		GL_BindBufferFunc (target, buffer);
	}
}

typedef struct {
	GLuint		buffer;
	GLintptr	offset;
	GLsizeiptr	size;
} bufferrange_t;

#define CACHED_BUFFER_RANGES 8

static bufferrange_t ssbo_ranges[CACHED_BUFFER_RANGES];

/*
====================
GL_BindBufferRange

glBindBufferRange wrapper
====================
*/
void GL_BindBufferRange (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
	if (target == GL_SHADER_STORAGE_BUFFER)
	{
		if (index < CACHED_BUFFER_RANGES)
		{
			bufferrange_t *range = &ssbo_ranges[index];
			if (range->buffer == buffer && range->offset == offset && range->size == size)
				return;
			range->buffer = buffer;
			range->offset = offset;
			range->size   = size;
		}
		current_shader_storage_buffer = buffer;
	}

	GL_BindBufferRangeFunc (target, index, buffer, offset, size);
}

/*
====================
GL_BindBuffersRange

glBindBuffersRange wrapper with fallback
if ARB_multi_bind is not present
====================
*/
void GL_BindBuffersRange (GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes)
{
	GLsizei i;
	if (gl_multi_bind_able)
	{
		if (target == GL_SHADER_STORAGE_BUFFER)
		{
			for (i = 0; i < count && first + i < countof (ssbo_ranges); i++)
			{
				bufferrange_t *range = &ssbo_ranges[first + i];
				range->buffer = buffers[i];
				range->offset = offsets[i];
				range->size   = sizes[i];
			}
		}
		GL_BindBuffersRangeFunc (target, first, count, buffers, offsets, sizes);
	}
	else
	{
		for (i = 0; i < count; i++)
			GL_BindBufferRange (target, first + i, buffers[i], offsets[i], sizes[i]);
	}
}

/*
====================
GL_DeleteBuffer
====================
*/
void GL_DeleteBuffer (GLuint buffer)
{
	int i;

	if (buffer == current_array_buffer)
		current_array_buffer = 0;
	if (buffer == current_element_array_buffer)
		current_element_array_buffer = 0;
	if (buffer == current_draw_indirect_buffer)
		current_draw_indirect_buffer = 0;
	if (buffer == current_shader_storage_buffer)
		current_shader_storage_buffer = 0;

	for (i = 0; i < countof(ssbo_ranges); i++)
		if (ssbo_ranges[i].buffer == buffer)
			ssbo_ranges[i].buffer = 0;

	GL_DeleteBuffersFunc (1, &buffer);
}

/*
====================
GL_ClearBufferBindings

This must be called if you do anything that could make the cached bindings
invalid (e.g. manually binding, destroying the context).
====================
*/
void GL_ClearBufferBindings (void)
{
	int i;

	current_array_buffer = 0;
	current_element_array_buffer = 0;
	current_draw_indirect_buffer = 0;
	current_shader_storage_buffer = 0;

	for (i = 0; i < countof(ssbo_ranges); i++)
		ssbo_ranges[i].buffer = 0;

	GL_BindBufferFunc (GL_ARRAY_BUFFER, 0);
	GL_BindBufferFunc (GL_ELEMENT_ARRAY_BUFFER, 0);
	GL_BindBufferFunc (GL_DRAW_INDIRECT_BUFFER, 0);
	GL_BindBufferFunc (GL_SHADER_STORAGE_BUFFER, 0);
}

/*
============================================================================
								FRAME RESOURCES
============================================================================
*/

#define FRAMES_IN_FLIGHT 3

typedef enum
{
	FRAMERES_HOST_BUFFER_BIT	= 1 << 0,
	FRAMERES_DEVICE_BUFFER_BIT	= 1 << 1,

	FRAMERES_ALL_BITS			= FRAMERES_HOST_BUFFER_BIT | FRAMERES_DEVICE_BUFFER_BIT
} frameres_bits_t;

typedef struct frameres_t
{
	GLsync			fence;
	GLuint			device_buffer;
	GLuint			host_buffer;
	GLubyte			*host_ptr;
	GLuint			*garbage;
} frameres_t;

static frameres_t	frameres[FRAMES_IN_FLIGHT];
static int			frameres_idx = 0;
static size_t		frameres_host_offset = 0;
static size_t		frameres_device_offset = 0;
static size_t		frameres_host_buffer_size = 1 * 1024 * 1024;
static size_t		frameres_device_buffer_size = 1 * 1024 * 1024;

/*
====================
GL_AddGarbageBuffer
====================
*/
void GL_AddGarbageBuffer (GLuint handle)
{
	VEC_PUSH (frameres[frameres_idx].garbage, handle);
}

/*
====================
GL_AllocFrameResources
====================
*/
static void GL_AllocFrameResources (frameres_bits_t bits)
{
	int i;
	for (i = 0; i < countof (frameres); i++)
	{
		char name[64];
		frameres_t *frame = &frameres[i];

		if (bits & FRAMERES_HOST_BUFFER_BIT)
		{
			GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

			if (frame->host_buffer)
			{
				if (frame->host_ptr)
				{
					GL_BindBuffer (GL_ARRAY_BUFFER, frame->host_buffer);
					GL_UnmapBufferFunc (GL_ARRAY_BUFFER);
				}
				GL_AddGarbageBuffer (frame->host_buffer);
			}

			GL_GenBuffersFunc (1, &frame->host_buffer);
			GL_BindBuffer (GL_ARRAY_BUFFER, frame->host_buffer);
			q_snprintf (name, sizeof (name), "dynamic host buffer %d", i);
			GL_ObjectLabelFunc (GL_BUFFER, frame->host_buffer, -1, name);
			if (gl_buffer_storage_able)
			{
				GL_BufferStorageFunc (GL_ARRAY_BUFFER, frameres_host_buffer_size, NULL, flags);
				frame->host_ptr = GL_MapBufferRangeFunc (GL_ARRAY_BUFFER, 0, frameres_host_buffer_size, flags);
				if (!frame->host_ptr)
					Sys_Error ("GL_AllocFrameResources: MapBufferRange failed on %" SDL_PRIu64 " bytes", (uint64_t)frameres_host_buffer_size);
			}
			else
			{
				GL_BufferDataFunc (GL_ARRAY_BUFFER, frameres_host_buffer_size, NULL, GL_STREAM_DRAW);
			}
		}

		if (bits & FRAMERES_DEVICE_BUFFER_BIT)
		{
			if (frame->device_buffer)
				GL_AddGarbageBuffer (frame->device_buffer);

			GL_GenBuffersFunc (1, &frame->device_buffer);
			GL_BindBuffer (GL_SHADER_STORAGE_BUFFER, frame->device_buffer);
			q_snprintf (name, sizeof (name), "dynamic device buffer %d", i);
			GL_ObjectLabelFunc (GL_BUFFER, frame->device_buffer, -1, name);
			GL_BufferDataFunc (GL_SHADER_STORAGE_BUFFER, frameres_device_buffer_size, NULL, GL_STREAM_DRAW);
		}
	}

	if (bits & FRAMERES_HOST_BUFFER_BIT)
		frameres_host_offset = 0;
	if (bits & FRAMERES_DEVICE_BUFFER_BIT)
		frameres_device_offset = 0;
}

/*
====================
GL_CreateFrameResources
====================
*/
void GL_CreateFrameResources (void)
{
	GL_AllocFrameResources (FRAMERES_ALL_BITS);
}

/*
====================
GL_DeleteFrameResources
====================
*/
void GL_DeleteFrameResources (void)
{
	size_t i, j, num_garbage_bufs;

	glFinish ();

	for (i = 0; i < countof (frameres); i++)
	{
		frameres_t *frame = &frameres[i];

		if (frame->fence)
		{
			GL_DeleteSyncFunc (frame->fence);
			frame->fence = NULL;
		}

		for (j = 0, num_garbage_bufs = VEC_SIZE (frame->garbage); j < num_garbage_bufs; j++)
			GL_DeleteBuffer (frame->garbage[j]);
		VEC_CLEAR (frame->garbage);

		if (frame->host_ptr)
		{
			GL_BindBuffer (GL_ARRAY_BUFFER, frame->host_buffer);
			GL_UnmapBufferFunc (GL_ARRAY_BUFFER);
			frame->host_ptr = NULL;
		}

		if (frame->host_buffer)
		{
			GL_DeleteBuffer (frame->host_buffer);
			frame->host_buffer = 0;
		}

		if (frame->device_buffer)
		{
			GL_DeleteBuffer (frame->device_buffer);
			frame->device_buffer = 0;
		}
	}
}

/*
====================
GL_AcquireFrameResources
====================
*/
void GL_AcquireFrameResources (void)
{
	frameres_t *prev_frame = &frameres[(frameres_idx + FRAMES_IN_FLIGHT - 1) % FRAMES_IN_FLIGHT];
	frameres_t *frame = &frameres[frameres_idx];
	size_t i, num_garbage_bufs;

	if (prev_frame->fence)
		GL_WaitSyncFunc (prev_frame->fence, 0, GL_TIMEOUT_IGNORED);

	if (frame->fence)
	{
		GLuint64 timeout = 1ull * 1000 * 1000 * 1000; // 1 second
		GLenum result = GL_ClientWaitSyncFunc (frame->fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
		if (result == GL_TIMEOUT_EXPIRED)
			glFinish ();
		else if (result == GL_WAIT_FAILED)
			Sys_Error ("GL_AcquireFrameResources: wait failed (0x%04X)", glGetError ());
		else if (result != GL_CONDITION_SATISFIED && result != GL_ALREADY_SIGNALED)
			Sys_Error ("GL_AcquireFrameResources: sync failed (0x%04X)", result);
		GL_DeleteSyncFunc (frame->fence);
		frame->fence = NULL;
	}

	num_garbage_bufs = VEC_SIZE (frame->garbage);
	for (i = 0; i < num_garbage_bufs; i++)
		GL_DeleteBuffer (frame->garbage[i]);
	VEC_CLEAR (frame->garbage);
}

/*
====================
GL_ReleaseFrameResources
====================
*/
void GL_ReleaseFrameResources (void)
{
	frameres_t *frame = &frameres[frameres_idx];

	SDL_assert (!frame->fence);
	frame->fence = GL_FenceSyncFunc (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	if (!frame->fence)
		Sys_Error ("glFenceSync failed (error code 0x%04X)", glGetError ());

	dev_stats.gpu_upload = frameres_host_offset;
	dev_peakstats.gpu_upload = q_max (dev_peakstats.gpu_upload, dev_stats.gpu_upload);

	if (++frameres_idx == countof (frameres))
		frameres_idx = 0;

	frameres_host_offset = 0;
	frameres_device_offset = 0;
}

/*
====================
GL_Upload
====================
*/
void GL_Upload (GLenum target, const void *data, size_t numbytes, GLuint *outbuf, GLbyte **outofs)
{
	size_t align;
	frameres_t *frame;

	align = (target == GL_UNIFORM_BUFFER) ? ubo_align : ssbo_align;
	frameres_host_offset = (frameres_host_offset + align) & ~align;

	if (frameres_host_offset + numbytes > frameres_host_buffer_size)
	{
		frameres_host_buffer_size = frameres_host_offset + ((numbytes + align) & ~align);
		frameres_host_buffer_size += frameres_host_buffer_size >> 1;
		GL_AllocFrameResources (FRAMERES_HOST_BUFFER_BIT);
	}

	frame = &frameres[frameres_idx];
	if (frame->host_ptr)
		memcpy (frame->host_ptr + frameres_host_offset, data, numbytes);
	else
	{
		GL_BindBuffer (target, frame->host_buffer);
		GL_BufferSubDataFunc (target, frameres_host_offset, numbytes, data);
	}

	*outbuf = frame->host_buffer;
	*outofs = (GLbyte*) frameres_host_offset;

	frameres_host_offset += numbytes;
}

/*
====================
GL_ReserveDeviceMemory
====================
*/
void GL_ReserveDeviceMemory (GLenum target, size_t numbytes, GLuint *outbuf, size_t *outofs)
{
	size_t align;
	frameres_t *frame;

	align = (target == GL_UNIFORM_BUFFER) ? ubo_align : ssbo_align;
	frameres_device_offset = (frameres_device_offset + align) & ~align;

	if (frameres_device_offset + numbytes > frameres_device_buffer_size)
	{
		frameres_device_buffer_size = frameres_device_offset + ((numbytes + align) & ~align);
		frameres_device_buffer_size += frameres_device_buffer_size >> 1;
		GL_AllocFrameResources (FRAMERES_DEVICE_BUFFER_BIT);
	}

	frame = &frameres[frameres_idx];

	*outbuf = frame->device_buffer;
	*outofs = frameres_device_offset;

	frameres_device_offset += numbytes;
}
