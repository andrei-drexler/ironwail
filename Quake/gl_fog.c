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
//gl_fog.c -- global and volumetric fog

#include "quakedef.h"

//==============================================================================
//
//  GLOBAL FOG
//
//==============================================================================

#define DEFAULT_DENSITY 0.0
#define DEFAULT_GRAY 0.3

static float fog_density;
static float fog_red;
static float fog_green;
static float fog_blue;

static float old_density;
static float old_red;
static float old_green;
static float old_blue;

static float fade_time; //duration of fade
static float fade_done; //time when fade will be done

extern float skyfog;

/*
=============
Fog_Update

update internal variables
=============
*/
void Fog_Update (float density, float red, float green, float blue, float time)
{
	//save previous settings for fade
	if (time > 0)
	{
		//check for a fade in progress
		if (fade_done > cl.time)
		{
			float f;

			f = (fade_done - cl.time) / fade_time;
			old_density = f * old_density + (1.0 - f) * fog_density;
			old_red = f * old_red + (1.0 - f) * fog_red;
			old_green = f * old_green + (1.0 - f) * fog_green;
			old_blue = f * old_blue + (1.0 - f) * fog_blue;
		}
		else
		{
			old_density = fog_density;
			old_red = fog_red;
			old_green = fog_green;
			old_blue = fog_blue;
		}
	}

	fog_density = density;
	fog_red = red;
	fog_green = green;
	fog_blue = blue;
	fade_time = time;
	fade_done = cl.time + time;
}

/*
=============
Fog_ParseServerMessage

handle an SVC_FOG message from server
=============
*/
void Fog_ParseServerMessage (void)
{
	float density, red, green, blue, time;

	density = MSG_ReadByte() / 255.0;
	red = MSG_ReadByte() / 255.0;
	green = MSG_ReadByte() / 255.0;
	blue = MSG_ReadByte() / 255.0;
	time = MSG_ReadShort() / 100.0;
	if (time < 0.0f) time = 0.0f;

	Fog_Update (density, red, green, blue, time);
}

/*
=============
Fog_FogCommand_f

handle the 'fog' console command
=============
*/
void Fog_FogCommand_f (void)
{
	float d, r, g, b, t;

	switch (Cmd_Argc())
	{
	default:
	case 1:
		Con_Printf("usage:\n");
		Con_Printf("   fog <density>\n");
		Con_Printf("   fog <red> <green> <blue>\n");
		Con_Printf("   fog <density> <red> <green> <blue>\n");
		Con_Printf("current values:\n");
		Con_Printf("   \"density\" is \"%g\"\n", fog_density);
		Con_Printf("   \"red\"     is \"%g\"\n", fog_red);
		Con_Printf("   \"green\"   is \"%g\"\n", fog_green);
		Con_Printf("   \"blue\"    is \"%g\"\n", fog_blue);
		return;
	case 2:
		d = Q_atof(Cmd_Argv(1));
		t = 0.0f;
		r = fog_red;
		g = fog_green;
		b = fog_blue;
		break;
	case 3: //TEST
		d = Q_atof(Cmd_Argv(1));
		t = Q_atof(Cmd_Argv(2));
		r = fog_red;
		g = fog_green;
		b = fog_blue;
		break;
	case 4:
		d = fog_density;
		t = 0.0f;
		r = Q_atof(Cmd_Argv(1));
		g = Q_atof(Cmd_Argv(2));
		b = Q_atof(Cmd_Argv(3));
		break;
	case 5:
		d = Q_atof(Cmd_Argv(1));
		r = Q_atof(Cmd_Argv(2));
		g = Q_atof(Cmd_Argv(3));
		b = Q_atof(Cmd_Argv(4));
		t = 0.0f;
		break;
	case 6: //TEST
		d = Q_atof(Cmd_Argv(1));
		r = Q_atof(Cmd_Argv(2));
		g = Q_atof(Cmd_Argv(3));
		b = Q_atof(Cmd_Argv(4));
		t = Q_atof(Cmd_Argv(5));
		break;
	}

	if      (d < 0.0f) d = 0.0f;
	if      (r < 0.0f) r = 0.0f;
	else if (r > 1.0f) r = 1.0f;
	if      (g < 0.0f) g = 0.0f;
	else if (g > 1.0f) g = 1.0f;
	if      (b < 0.0f) b = 0.0f;
	else if (b > 1.0f) b = 1.0f;
	Fog_Update(d, r, g, b, t);
}

/*
=============
Fog_ParseWorldspawn

called at map load
=============
*/
void Fog_ParseWorldspawn (void)
{
	char key[128], value[4096];
	const char *data;

	//initially no fog
	fog_density = DEFAULT_DENSITY;
	fog_red = DEFAULT_GRAY;
	fog_green = DEFAULT_GRAY;
	fog_blue = DEFAULT_GRAY;

	old_density = DEFAULT_DENSITY;
	old_red = DEFAULT_GRAY;
	old_green = DEFAULT_GRAY;
	old_blue = DEFAULT_GRAY;

	fade_time = 0.0;
	fade_done = 0.0;

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

		if (!strcmp("fog", key))
		{
			sscanf(value, "%f %f %f %f", &fog_density, &fog_red, &fog_green, &fog_blue);
		}
	}
}

/*
=============
Fog_GetColor

calculates fog color for this frame, taking into account fade times
=============
*/
float *Fog_GetColor (void)
{
	static float c[4];
	float f;
	int i;

	if (fade_done > cl.time)
	{
		f = (fade_done - cl.time) / fade_time;
		c[0] = f * old_red + (1.0 - f) * fog_red;
		c[1] = f * old_green + (1.0 - f) * fog_green;
		c[2] = f * old_blue + (1.0 - f) * fog_blue;
		c[3] = 1.0;
	}
	else
	{
		c[0] = fog_red;
		c[1] = fog_green;
		c[2] = fog_blue;
		c[3] = 1.0;
	}

	for (i = 0; i < 3; i++) {
		c[i] = CLAMP (0.f, c[i], 1.f);
	}

	//find closest 24-bit RGB value, so solid-colored sky can match the fog perfectly
	for (i = 0; i < 3; i++) {
		c[i] = (float)(Q_rint(c[i] * 255)) / 255.0f;
	}

	return c;
}

/*
=============
Fog_GetDensity

returns current density of fog
=============
*/
float Fog_GetDensity (void)
{
	float f;

	if (fade_done > cl.time)
	{
		f = (fade_done - cl.time) / fade_time;
		return f * old_density + (1.0 - f) * fog_density;
	}
	else
		return fog_density;
}

/*
=============
Fog_SetupFrame

called at the beginning of each frame
=============
*/
void Fog_SetupFrame (void)
{
	const float ExpAdjustment = 1.20112241f; // sqrt(log2(e))
	const float SphericalCorrection = 0.85f; // compensate higher perceived density with spherical fog
	const float DensityScale = ExpAdjustment * SphericalCorrection / 64.0f;
	float density = Fog_GetDensity() * DensityScale;
	memcpy(r_framedata.fogdata, Fog_GetColor(), 3 * sizeof(float));
	memcpy(r_framedata.skyfogdata, r_framedata.fogdata, 3 * sizeof(float));
	r_framedata.fogdata[3] = density * density;
	r_framedata.skyfogdata[3] = density > 0.f ? CLAMP (0.f, skyfog, 1.f) : 0.f;
}

/*
=============
Fog_EnableGFog

called before drawing stuff that should be fogged
=============
*/
void Fog_EnableGFog (void)
{
	Fog_SetupFrame();
}

/*
=============
Fog_DisableGFog

called after drawing stuff that should be fogged
=============
*/
void Fog_DisableGFog (void)
{
	memset(r_framedata.fogdata, 0, sizeof(r_framedata.fogdata));
}

//==============================================================================
//
//  VOLUMETRIC FOG
//
//==============================================================================

cvar_t r_vfog = {"r_vfog", "1", CVAR_NONE};

void Fog_DrawVFog (void){}
void Fog_MarkModels (void){}

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
=============
Fog_NewMap

called whenever a map is loaded
=============
*/
void Fog_NewMap (void)
{
	Fog_ParseWorldspawn (); //for global fog
	Fog_MarkModels (); //for volumetric fog
}

/*
=============
Fog_Init

called when carnifex initializes
=============
*/
void Fog_Init (void)
{
	Cmd_AddCommand ("fog",Fog_FogCommand_f);

	//Cvar_RegisterVariable (&r_vfog);

	//set up global fog
	fog_density = DEFAULT_DENSITY;
	fog_red = DEFAULT_GRAY;
	fog_green = DEFAULT_GRAY;
	fog_blue = DEFAULT_GRAY;
}
