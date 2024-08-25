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
//gl_sky.c

#include "quakedef.h"

extern	int	rs_skypolys; //for r_speeds readout
extern	int rs_skypasses; //for r_speeds readout
float	skyflatcolor[3];

static skybox_t	*skybox_list;
skybox_t		*skybox;

extern cvar_t gl_farclip;
cvar_t r_fastsky = {"r_fastsky", "0", CVAR_NONE};
cvar_t r_skyalpha = {"r_skyalpha", "1", CVAR_NONE};
cvar_t r_skyfog = {"r_skyfog","0.5",CVAR_NONE};
cvar_t r_skywind = {"r_skywind","1",CVAR_ARCHIVE};

static const int skytexorder[6] = {0,2,1,3,4,5}; //for skybox

static const char st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},
	{1,3,2},
	{-1,-3,2},
 	{-2,-1,3},		// straight up
 	{2,-1,-3}		// straight down
};

float skyfog; // ericw

#define SKYWIND_CFG			"wind.cfg"

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
=============
Sky_LoadTexture

A sky texture is 256*128, with the left side being a masked overlay
==============
*/
void Sky_LoadTexture (qmodel_t *mod, texture_t *mt)
{
	char		texturename[64];
	unsigned	x, y, p, r, g, b, count, halfwidth, *rgba;
	byte		*src, *front_data, *back_data;

	if (mt->width != 256 || mt->height != 128)
	{
		Con_DWarning ("Sky texture %s is %d x %d, expected 256 x 128\n", mt->name, mt->width, mt->height);
		if (mt->width < 2 || mt->height < 1)
			return;
	}

	halfwidth = mt->width / 2;
	back_data = (byte *) Hunk_AllocName (halfwidth*mt->height*2, "skytex");
	front_data = back_data + halfwidth*mt->height;
	src = (byte *)(mt + 1);

// extract back layer and upload
	for (y=0 ; y<mt->height ; y++)
		memcpy (back_data + y*halfwidth, src + halfwidth + y*mt->width, halfwidth);

	q_snprintf(texturename, sizeof(texturename), "%s:%s_back", mod->name, mt->name);
	mt->gltexture = TexMgr_LoadImage (mod, texturename, halfwidth, mt->height, SRC_INDEXED, back_data, "", (src_offset_t)back_data, TEXPREF_BINDLESS);

// extract front layer and upload
	r = g = b = count = 0;
	for (y=0 ; y<mt->height ; src+=mt->width, front_data+=halfwidth, y++)
	{
		for (x=0 ; x<halfwidth ; x++)
		{
			p = src[x];
			if (p == 0)
				p = 255;
			else
			{
				rgba = &d_8to24table[p];
				r += ((byte *)rgba)[0];
				g += ((byte *)rgba)[1];
				b += ((byte *)rgba)[2];
				count++;
			}
			front_data[x] = p;
		}
	}

	front_data = back_data + halfwidth*mt->height;
	q_snprintf(texturename, sizeof(texturename), "%s:%s_front", mod->name, mt->name);
	mt->fullbright = TexMgr_LoadImage (mod, texturename, halfwidth, mt->height, SRC_INDEXED, front_data, "", (src_offset_t)front_data, TEXPREF_ALPHA|TEXPREF_BINDLESS);

// calculate r_fastsky color based on average of all opaque foreground colors
	skyflatcolor[0] = (float)r/(count*255);
	skyflatcolor[1] = (float)g/(count*255);
	skyflatcolor[2] = (float)b/(count*255);
}

/*
=============
Sky_LoadTextureQ64

Quake64 sky textures are 32*64
==============
*/
void Sky_LoadTextureQ64 (qmodel_t *mod, texture_t *mt)
{
	char		texturename[64];
	unsigned	i, p, r, g, b, count, halfheight, *rgba;
	byte		*front, *back, *front_rgba;

	if (mt->width != 32 || mt->height != 64)
	{
		Con_DWarning ("Q64 sky texture %s is %d x %d, expected 32 x 64\n", mt->name, mt->width, mt->height);
		if (mt->width < 1 || mt->height < 2)
			return;
	}

	// pointers to both layer textures
	halfheight = mt->height / 2;
	front = (byte *)(mt+1);
	back = (byte *)(mt+1) + mt->width*halfheight;
	front_rgba = (byte *) Hunk_AllocName (4*mt->width*halfheight, "q64_skytex");

	// Normal indexed texture for the back layer
	q_snprintf(texturename, sizeof(texturename), "%s:%s_back", mod->name, mt->name);
	mt->gltexture = TexMgr_LoadImage (mod, texturename, mt->width, halfheight, SRC_INDEXED, back, "", (src_offset_t)back, TEXPREF_BINDLESS);

	// front layer, convert to RGBA and upload
	p = r = g = b = count = 0;

	for (i=mt->width*halfheight ; i!=0 ; i--)
	{
		rgba = &d_8to24table[*front++];

		// RGB
		front_rgba[p++] = ((byte*)rgba)[0];
		front_rgba[p++] = ((byte*)rgba)[1];
		front_rgba[p++] = ((byte*)rgba)[2];
		// Alpha
		front_rgba[p++] = 128; // this look ok to me!
		
		// Fast sky
		r += ((byte *)rgba)[0];
		g += ((byte *)rgba)[1];
		b += ((byte *)rgba)[2];
		count++;
	}

	q_snprintf(texturename, sizeof(texturename), "%s:%s_front", mod->name, mt->name);
	mt->fullbright = TexMgr_LoadImage (mod, texturename, mt->width, halfheight, SRC_RGBA, front_rgba, "", (src_offset_t)front_rgba, TEXPREF_ALPHA|TEXPREF_BINDLESS);

	// calculate r_fastsky color based on average of all opaque foreground colors
	skyflatcolor[0] = (float)r/(count*255);
	skyflatcolor[1] = (float)g/(count*255);
	skyflatcolor[2] = (float)b/(count*255);
}

/*
=================
Skywind_Clear
=================
*/
static void Skywind_Clear (void)
{
	if (!skybox)
		return;
	skybox->wind_dist = 0.f;
	skybox->wind_yaw = 45.f;
	skybox->wind_pitch = 0.f;
	skybox->wind_period = 30.f;
}

/*
=================
Skywind_Load_f
=================
*/
static void Skywind_Load_f (void)
{
	char relname[MAX_QPATH];
	char *buf;
	const char *data;

	if (!skybox)
	{
		Con_Printf ("No skybox loaded\n");
		return;
	}

	q_snprintf (relname, sizeof (relname), "gfx/env/%s" SKYWIND_CFG, skybox->name);
	buf = (char *) QFS_LoadMallocFile (relname, NULL, NULL);
	if (!buf)
	{
		Con_DPrintf ("Sky wind config not found '%s'.\n", relname);
		return;
	}

	data = COM_Parse (buf);
	if (!data)
		goto done;

	if (strcmp (com_token, "skywind") != 0)
	{
		Con_Printf ("Skywind_Load_f: first token must be 'skywind'.\n");
		goto done;
	}

	Skywind_Clear ();

	if ((data = COM_Parse (data)) != NULL)
		skybox->wind_dist = CLAMP (-2.0, atof (com_token), 2.0);

	if ((data = COM_Parse (data)) != NULL)
		skybox->wind_yaw = fmod (atof (com_token), 360.0);

	if ((data = COM_Parse (data)) != NULL)
		skybox->wind_period = atof (com_token);

	if ((data = COM_Parse (data)) != NULL)
		skybox->wind_pitch = fmod (atof (com_token) + 90.0, 180.0) - 90.0;

done:
	free (buf);
}

/*
=================
Skywind_Save_f
=================
*/
static void Skywind_Save_f (void)
{
	char relname[MAX_QPATH];
	char path[MAX_OSPATH];
	FILE *f;

	if (!skybox)
	{
		Con_Printf ("No skybox loaded\n");
		return;
	}

	q_snprintf (relname, sizeof (relname), "gfx/env/%s" SKYWIND_CFG, skybox->name);
	q_snprintf (path, sizeof (path), "%s/%s", com_gamedir, relname);
	f = Sys_fopen (path, "wt");
	if (!f)
	{
		Con_Printf ("Couldn't write '%s'.\n", relname);
		return;
	}

	fprintf (f,
		"// distance yaw period pitch\n"
		"skywind %g %g %g %g\n",
		skybox->wind_dist,
		skybox->wind_yaw,
		skybox->wind_period,
		skybox->wind_pitch
	);

	fclose (f);

	Con_SafePrintf ("Wrote ");
	Con_LinkPrintf (path, "%s", relname);
	Con_SafePrintf ("\n");
}

/*
=================
Skywind_LookDir_f
=================
*/
static void Skywind_LookDir_f (void)
{
	if (cls.state != ca_connected)
		return;

	if (!skybox)
	{
		Con_Printf ("No skybox loaded\n");
		return;
	}

	// invert view direction so that clouds move towards the player, not away from them
	skybox->wind_yaw = fmod (cl.viewangles[YAW] + 180.0, 360.0);
	skybox->wind_pitch = -cl.viewangles[PITCH];

	// first argument, if present, overrides the loop duration (default: 30 seconds)
	if (Cmd_Argc () >= 2)
		skybox->wind_period = atof (Cmd_Argv (1));
	else if (!skybox->wind_period)
		skybox->wind_period = 30.f;

	// second argument, if present, overrides the amplitude of the movement (default: 1.0)
	if (Cmd_Argc () >= 3)
		skybox->wind_dist = CLAMP (-2.0, atof (Cmd_Argv (2)), 2.0);
	else if (!skybox->wind_dist)
		skybox->wind_dist = 1.f;
}

/*
=================
Skywind_Rotate_f
=================
*/
static void Skywind_Rotate_f (void)
{
	if (cls.state != ca_connected)
		return;

	if (!skybox)
	{
		Con_Printf ("No skybox loaded\n");
		return;
	}

	if (Cmd_Argc () < 2)
	{
		Con_Printf (
			"usage:\n"
			"   %s <yawdelta> [pitchdelta]\n",
			Cmd_Argv (0)
		);
		return;
	}

	skybox->wind_yaw = fmod (skybox->wind_yaw + atof (Cmd_Argv (1)), 360.0);
	if (Cmd_Argc () >= 3)
		skybox->wind_pitch = fmod (skybox->wind_pitch + atof (Cmd_Argv (2)) + 90.0, 180.0) - 90.0;
}

/*
=================
Skywind_f
=================
*/
static void Skywind_f (void)
{
	if (cls.state != ca_connected)
		return;

	if (!skybox)
	{
		Con_Printf ("No skybox loaded\n");
		return;
	}

	if (Cmd_Argc () < 2)
	{
		Con_Printf (
			"usage:\n"
			"   %s [distance] [yaw] [period] [pitch]\n"
			"current values:\n"
			"   \"distance\" is \"%g\"\n"
			"   \"yaw\"      is \"%g\"\n"
			"   \"period\"   is \"%g\"\n"
			"   \"pitch\"    is \"%g\"\n",
			Cmd_Argv (0),
			skybox->wind_dist,
			skybox->wind_yaw,
			skybox->wind_period,
			skybox->wind_pitch
		);
		return;
	}

	skybox->wind_dist = CLAMP (-2.0, atof (Cmd_Argv (1)), 2.0);
	if (Cmd_Argc () >= 3)
		skybox->wind_yaw = fmod (atof (Cmd_Argv (2)), 360.0);
	if (Cmd_Argc () >= 4)
		skybox->wind_period = atof (Cmd_Argv (3));
	if (Cmd_Argc () >= 5)
		skybox->wind_pitch = fmod (atof (Cmd_Argv (4)) + 90.0, 180.0) - 90.0;
}

/*
==================
Sky_LoadSkyBox
==================
*/
static const char *const suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
void Sky_LoadSkyBox (const char *name)
{
	int			i, mark, width[6], height[6], samesize, numloaded;
	char		filename[MAX_OSPATH];
	byte		*data[6];
	skybox_t	newsky;
	enum srcformat	fmt;

	if (skybox && strcmp(skybox->name, name) == 0)
		return; //no change

	//turn off skybox if sky is set to ""
	if (name[0] == 0)
	{
		skybox = NULL;
		return;
	}

	//check if already loaded
	for (i = 0, numloaded = VEC_SIZE (skybox_list); i < numloaded; i++)
	{
		if (strcmp (skybox_list[i].name, name) == 0)
		{
			skybox = &skybox_list[i];
			return;
		}
	}

	//load textures
	mark = Hunk_LowMark ();
	for (i = 0, numloaded = 0, samesize = 0; i < 6; i++)
	{
		q_snprintf (filename, sizeof(filename), "gfx/env/%s%s", name, suf[i]);
		data[i] = Image_LoadImage (filename, &width[i], &height[i], &fmt);
		if (data[i])
		{
			if (fmt != SRC_RGBA)
				Sys_Error ("Bad format %i for skybox side %s", fmt, filename);

			numloaded++;
			if (width[i] != height[i])
				samesize = -1;
			else if (samesize == 0)
				samesize = width[i];
			else if (samesize != width[i])
				samesize = -1;
		}
		else
		{
			Con_Printf ("Couldn't load %s\n", filename);
		}
	}

	if (numloaded == 0) // go back to scrolling sky if skybox is totally missing
	{
		skybox = NULL;
		return;
	}

	memset (&newsky, 0, sizeof (newsky));

	if (samesize > 0) // create a single cubemap texture if all faces are the same size
	{
		const int cubemap_order[6] = {3, 1, 4, 5, 0, 2}; // ft/bk/up/dn/rt/lf
		size_t numfacebytes = samesize * samesize * 4;
		size_t aligneddatasize = (numfacebytes * 6 + sizeof (void *) - 1) & ~(sizeof (void *) - 1);

		newsky.cubemap_pixels = (byte *) malloc (aligneddatasize + sizeof (void *) * 6);
		if (!newsky.cubemap_pixels)
		{
			Con_Warning ("Sky_LoadSkyBox: out of memory on %" SDL_PRIu64 " bytes\n", (uint64_t) numfacebytes);
			skybox = NULL;
			Hunk_FreeToLowMark (mark);
			return;
		}
		newsky.cubemap_offsets = (void **) (newsky.cubemap_pixels + aligneddatasize);

		for (i = 0; i < 6; i++)
		{
			byte *dstpixels = newsky.cubemap_pixels + numfacebytes * i;
			byte *srcpixels = data[cubemap_order[i]];
			if (srcpixels)
				memcpy (dstpixels, srcpixels, numfacebytes);
			else
				memset (dstpixels, 0, numfacebytes); // TODO: average out existing faces instead?
			newsky.cubemap_offsets[i] = dstpixels;
		}

		q_snprintf (filename, sizeof(filename), "gfx/env/%s", name);
		newsky.cubemap = TexMgr_LoadImage (cl.worldmodel, filename,
			samesize, samesize, SRC_RGBA,
			(byte *)newsky.cubemap_offsets, "", (src_offset_t)newsky.cubemap_offsets,
			TEXPREF_CUBEMAP | TEXPREF_NOPICMIP | TEXPREF_MIPMAP | TEXPREF_ALPHA
		);
	}
	else // create a separate texture for each side
	{
		for (i = 0; i < 6; i++)
		{
			q_snprintf (filename, sizeof(filename), "gfx/env/%s%s", name, suf[i]);
			newsky.textures[i] = TexMgr_LoadImage (cl.worldmodel, filename, width[i], height[i], SRC_RGBA, data[i], filename, 0, TEXPREF_NONE);
		}
	}
	Hunk_FreeToLowMark (mark);

	q_strlcpy (newsky.name, name, sizeof(newsky.name));
	VEC_PUSH (skybox_list, newsky);
	skybox = &skybox_list[VEC_SIZE (skybox_list) - 1];

	Skywind_Load_f ();
}

/*
==================
Sky_FreeSkyBox
==================
*/
static void Sky_FreeSkyBox (skybox_t *sky)
{
	free (sky->cubemap_pixels);
	// Note: textures are freed by Mod_ClearAll / Mod_ResetAll
	memset (sky, 0, sizeof (*sky));
}

/*
=================
Sky_ClearAll

Called on map unload/game change
=================
*/
void Sky_ClearAll (void)
{
	int i, count;

	skybox = NULL;
	for (i = 0, count = VEC_SIZE (skybox_list); i < count; i++)
		Sky_FreeSkyBox (&skybox_list[i]);
	VEC_CLEAR (skybox_list);

	Cvar_SetQuick (&r_skyfog, r_skyfog.default_string);
}

/*
=================
Sky_NewMap
=================
*/
void Sky_NewMap (void)
{
	char	key[128], value[4096];
	const char	*data;

	skyfog = r_skyfog.value;

	//
	// read worldspawn (this is so ugly, and shouldn't it be done on the server?)
	//
	data = cl.worldmodel->entities;
	if (!data)
		return; //FIXME: how could this possibly ever happen? -- if there's no
	// worldspawn then the sever wouldn't send the loadmap message to the client

	data = COM_Parse(data);
	if (!data) //should never happen
		return; // error
	if (com_token[0] != '{') //should never happen
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

		if (!strcmp("sky", key))
			Sky_LoadSkyBox(value);

		if (!strcmp("skyfog", key))
			skyfog = atof(value);

#if 1 //also accept non-standard keys
		else if (!strcmp("skyname", key)) //half-life
			Sky_LoadSkyBox(value);
		else if (!strcmp("qlsky", key)) //quake lives
			Sky_LoadSkyBox(value);
#endif
	}
}

/*
=================
Sky_SkyCommand_f
=================
*/
void Sky_SkyCommand_f (void)
{
	switch (Cmd_Argc())
	{
	case 1:
		Con_Printf("\"sky\" is \"%s\"\n", skybox ? skybox->name : "");
		break;
	case 2:
		Sky_LoadSkyBox(Cmd_Argv(1));
		break;
	default:
		Con_Printf("usage: sky <skyname>\n");
	}
}

/*
====================
R_SetSkyfog_f -- ericw
====================
*/
static void R_SetSkyfog_f (cvar_t *var)
{
// clear any skyfog setting from worldspawn
	skyfog = var->value;
}

/*
=============
Sky_Init
=============
*/
void Sky_Init (void)
{
	Cvar_RegisterVariable (&r_fastsky);
	Cvar_RegisterVariable (&r_skyalpha);
	Cvar_RegisterVariable (&r_skyfog);
	Cvar_RegisterVariable (&r_skywind);
	Cvar_SetCallback (&r_skyfog, R_SetSkyfog_f);

	Cmd_AddCommand ("sky",Sky_SkyCommand_f);
	Cmd_AddCommand ("skywind",Skywind_f);
	Cmd_AddCommand ("skywind_save",Skywind_Save_f);
	Cmd_AddCommand ("skywind_load",Skywind_Load_f);
	Cmd_AddCommand ("skywind_lookdir",Skywind_LookDir_f);
	Cmd_AddCommand ("skywind_rotate",Skywind_Rotate_f);
}

//==============================================================================
//
//  RENDER SKYBOX
//
//==============================================================================

/*
==============
Sky_EmitSkyBoxVertex
==============
*/
void Sky_EmitSkyBoxVertex (float s, float t, int axis, float *uv, float *pos)
{
	vec3_t		v, b;
	int			j, k;
	float		w, h;

	b[0] = s * gl_farclip.value / sqrt(3.0);
	b[1] = t * gl_farclip.value / sqrt(3.0);
	b[2] = gl_farclip.value / sqrt(3.0);

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	// convert from range [-1,1] to [0,1]
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	// avoid bilerp seam
	w = skybox->textures[skytexorder[axis]]->width;
	h = skybox->textures[skytexorder[axis]]->height;
	s = s * (w-1)/w + 0.5/w;
	t = t * (h-1)/h + 0.5/h;

	t = 1.0 - t;
	uv[0] = s;
	uv[1] = t;
	VectorCopy(v, pos);
}

/*
==============
Sky_DrawSkyBox
==============
*/
void Sky_DrawSkyBox (void)
{
	int i, j;

	vec4_t fog;
	fog[0] = r_framedata.fogdata[0];
	fog[1] = r_framedata.fogdata[1];
	fog[2] = r_framedata.fogdata[2];
	fog[3] = r_framedata.fogdata[3] > 0.f ? skyfog : 0.f;

	GL_UseProgram (glprogs.skyboxside[softemu == SOFTEMU_COARSE]);
	GL_SetState (GLS_BLEND_OPAQUE | GLS_NO_ZTEST | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS(2));

	GL_UniformMatrix4fvFunc (0, 1, GL_FALSE, r_matviewproj);
	GL_Uniform3fvFunc (1, 1, r_refdef.vieworg);
	GL_Uniform4fvFunc (2, 1, fog);
	GL_Uniform1fFunc (3, r_framedata.screendither);

	for (i = 0; i < 6; i++)
	{
		struct skyboxvert_s {
			vec3_t pos;
			float uv[2];
		} verts[4];

		GLuint buf;
		GLbyte *ofs;

		float st[2] = {1.f, 1.f};
		for (j = 0; j < 4; j++)
		{
			Sky_EmitSkyBoxVertex(st[0], st[1], i, verts[j].uv, verts[j].pos);
			st[j & 1] *= -1.f;
		}

		GL_Upload (GL_ARRAY_BUFFER, verts, sizeof(verts), &buf, &ofs);
		GL_BindBuffer (GL_ARRAY_BUFFER, buf);
		GL_VertexAttribPointerFunc (0, 3, GL_FLOAT, GL_FALSE, sizeof(verts[0]), ofs + offsetof(struct skyboxvert_s, pos));
		GL_VertexAttribPointerFunc (1, 2, GL_FLOAT, GL_FALSE, sizeof(verts[0]), ofs + offsetof(struct skyboxvert_s, uv));

		GL_Bind (GL_TEXTURE0, skybox->textures[skytexorder[i]]);
		glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
	}
}

/*
==============
Sky_DrawSky
==============
*/
void Sky_DrawSky (void)
{
	entity_t **ents;
	int count;

	GL_BeginGroup ("Sky");

	ents = R_GetVisEntities (mod_brush, false, &count);

	if (skybox && skybox->cubemap)
	{
		R_DrawBrushModels_SkyCubemap (ents, count);
	}
	else if (skybox)
	{
		glEnable (GL_STENCIL_TEST);
		glStencilMask (1);
		glStencilFunc (GL_ALWAYS, 1, 1);
		glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);
		glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		R_DrawBrushModels_SkyStencil (ents, count);

		glStencilFunc (GL_EQUAL, 1, 1);
		glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);
		glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		Sky_DrawSkyBox ();

		glDisable (GL_STENCIL_TEST);
	}
	else
	{
		R_DrawBrushModels_SkyLayers (ents, count);
	}

	GL_EndGroup ();
}

/*
==============
Sky_IsAnimated
==============
*/
qboolean Sky_IsAnimated (void)
{
	return r_skywind.value != 0.f && skybox && skybox->wind_dist != 0.f;
}

/*
==============
Sky_SetupFrame
==============
*/
void Sky_SetupFrame (void)
{
	float yaw = skybox ? DEG2RAD (skybox->wind_yaw) : 0.f;
	float pitch = skybox ? DEG2RAD (skybox->wind_pitch) : 0.f;
	float sy = sin (yaw);
	float sp = sin (pitch);
	float cy = cos (yaw);
	float cp = cos (pitch);
	float dist = skybox ? CLAMP (-2.f, skybox->wind_dist, 2.f) : 0.f;
	float period = r_skywind.value && skybox ? skybox->wind_period / r_skywind.value : 0.0;
	double phase = period ? cl.time * 0.5 / period : 0.5;

	phase -= floor (phase) + 0.5; // [-0.5, 0.5)

	r_framedata.winddir[0] =  dist * cp * sy;
	r_framedata.winddir[1] =  dist * sp;
	r_framedata.winddir[2] = -dist * cp * cy;
	r_framedata.windphase = phase;
}
