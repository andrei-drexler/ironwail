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

// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include "steam.h"
#include <time.h>

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full

*/


int			glx, gly, glwidth, glheight;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

//johnfitz -- new cvars
cvar_t		scr_menuscale = {"scr_menuscale", "1", CVAR_ARCHIVE};
cvar_t		scr_menubgalpha = {"scr_menubgalpha", "0.7", CVAR_ARCHIVE};
cvar_t		scr_menubgstyle = {"scr_menubgstyle", "-1", CVAR_ARCHIVE};
cvar_t		scr_centerprintbg = {"scr_centerprintbg", "2", CVAR_ARCHIVE}; // 0 = off; 1 = text box; 2 = menu box; 3 = menu strip
cvar_t		scr_sbarscale = {"scr_sbarscale", "1", CVAR_ARCHIVE};
cvar_t		scr_sbaralpha = {"scr_sbaralpha", "0.75", CVAR_ARCHIVE};
cvar_t		scr_conwidth = {"scr_conwidth", "0", CVAR_ARCHIVE};
cvar_t		scr_conscale = {"scr_conscale", "1", CVAR_ARCHIVE};
cvar_t		scr_crosshairscale = {"scr_crosshairscale", "1", CVAR_ARCHIVE};
cvar_t		scr_pixelaspect = {"scr_pixelaspect", "1", CVAR_ARCHIVE};
cvar_t		scr_showfps = {"scr_showfps", "0", CVAR_ARCHIVE};
cvar_t		scr_showspeed = {"scr_showspeed", "0", CVAR_ARCHIVE};
cvar_t		scr_showspeed_ofs = {"scr_showspeed_ofs", "0", CVAR_ARCHIVE};
cvar_t		scr_clock = {"scr_clock", "0", CVAR_ARCHIVE};
//johnfitz
cvar_t		scr_usekfont = {"scr_usekfont", "0", CVAR_NONE}; // 2021 re-release

cvar_t		scr_hudstyle = {"hudstyle", "2", CVAR_ARCHIVE};
cvar_t		cl_screenshotname = {"cl_screenshotname", "screenshots/%map%_%date%_%time%", CVAR_ARCHIVE};
cvar_t		scr_demobar_timeout = {"scr_demobar_timeout", "1", CVAR_ARCHIVE};

cvar_t		scr_viewsize = {"viewsize","100", CVAR_ARCHIVE};
cvar_t		scr_fov = {"fov","90",CVAR_ARCHIVE};	// 10 - 170
cvar_t		scr_fov_adapt = {"fov_adapt","1",CVAR_ARCHIVE};
cvar_t		scr_zoomfov = {"zoom_fov","30",CVAR_ARCHIVE};	// 10 - 170
cvar_t		scr_zoomspeed = {"zoom_speed","8",CVAR_ARCHIVE};
cvar_t		scr_conspeed = {"scr_conspeed","2000",CVAR_ARCHIVE};
cvar_t		scr_centertime = {"scr_centertime","2",CVAR_NONE};
cvar_t		scr_showturtle = {"showturtle","0",CVAR_NONE};
cvar_t		scr_showpause = {"showpause","1",CVAR_NONE};
cvar_t		scr_printspeed = {"scr_printspeed","8",CVAR_NONE};
cvar_t		gl_triplebuffer = {"gl_triplebuffer", "1", CVAR_ARCHIVE};

cvar_t		cl_gun_fovscale = {"cl_gun_fovscale","1",CVAR_ARCHIVE}; // Qrack
cvar_t		cl_gun_x = {"cl_gun_x","0",CVAR_ARCHIVE};
cvar_t		cl_gun_y = {"cl_gun_y","0",CVAR_ARCHIVE};
cvar_t		cl_gun_z = {"cl_gun_z","0",CVAR_ARCHIVE};

extern	char	crosshair_char;
extern	cvar_t	crosshair;
extern	cvar_t	con_notifyfade;
extern	cvar_t	con_notifyfadetime;

extern	edict_t	**bbox_linked;
extern	cvar_t	r_showfields;
extern	cvar_t	r_showfields_align;

qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_net;
qpic_t		*scr_turtle;

int			clearconsole;
int			clearnotify;

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

int	scr_tileclear_updates = 0; //johnfitz

hudstyle_t	hudstyle;

void SCR_ScreenShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_center_maxcols;
int			scr_erase_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (const char *str) //update centerprint data
{
	int cols;

	q_strlcpy (scr_centerstring, str, sizeof (scr_centerstring));
	if (!scr_centerstring[0])
	{
		scr_center_lines = 0;
		scr_center_maxcols = 0;
		return;
	}

	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;
	if (!cl.intermission)
		scr_centertime_off += q_max (0.f, con_notifyfade.value * con_notifyfadetime.value);

// count the number of lines for centering
	scr_center_lines = 1;
	scr_center_maxcols = 0;
	str = scr_centerstring;
	cols = 0;
	while (*str)
	{
		if (*str == '\n')
		{
			scr_center_lines++;
			scr_center_maxcols = q_max (scr_center_maxcols, cols);
			cols = -1; // compensate the following ++
		}
		str++;
		cols++;
	}
	scr_center_maxcols = q_max (scr_center_maxcols, cols);
}

static void SCR_DrawCenterStringBG (int y, float alpha)
{
	const char *str;
	int i, len, lines, x;

	if (cl.intermission || q_min (scr_center_lines, scr_center_maxcols) <= 0 || alpha <= 0.f)
		return;

	// skip leading empty lines (might be there just to reposition the text)
	str = scr_centerstring;
	while (*str == '\n')
	{
		str++;
		y += CHARSIZE;
	}

	// skip trailing empty lines
	len = (int) strlen (str);
	while (len > 0 && str[len - 1] == '\n')
		--len;

	// count remaining lines
	for (i = 0, lines = 1; i < len; i++)
		if (str[i] == '\n')
			lines++;

	// draw the background
	switch ((int)scr_centerprintbg.value)
	{
	case 1:
		len = (scr_center_maxcols + 3) & ~1;
		x = (320 - len * 8) / 2;
		GL_PushCanvasColor (1.f, 1.f, 1.f, alpha * scr_menubgalpha.value);
		M_DrawTextBox (x - 8, y - 12, len, lines + 1);
		GL_PopCanvasColor ();
		break;

	case 2:
		len = scr_center_maxcols + 2;
		x = (320 - len* 8) / 2;
		Draw_PartialFadeScreen (x, x + len * 8, y - 4, y + lines * 8 + 4, alpha);
		break;

	case 3:
		Draw_PartialFadeScreen (glcanvas.left, glcanvas.right, y - 4, y + lines * 8 + 4, alpha);
		break;

	default:
		return;
	}
}

void SCR_DrawCenterString (void) //actually do the drawing
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;
	float	alpha, forcedalpha;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

// the finale prints the characters one at a time
	if (cl.intermission)
	{
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
		alpha = 1.f;
	}
	else
	{
		float fade = q_max (con_notifyfade.value * con_notifyfadetime.value, 0.f);
		remaining = 9999;
		alpha = fade ? q_min (scr_centertime_off / fade, 1.f) : 1.f;
	}

	if (M_ForcedCenterPrint (&forcedalpha))
		alpha *= forcedalpha * forcedalpha;

	GL_PushCanvasColor (1.f, 1.f, 1.f, alpha);

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = 200*0.35;	//johnfitz -- 320x200 coordinate system
	else
		y = 48;
	if (crosshair.value && scr_viewsize.value < 130)
		y -= 8;

	SCR_DrawCenterStringBG (y, alpha);

	do
	{
	// scan the width of the line
		for (l=0 ; start[l] ; l++)
			if (start[l] == '\n')
				break;
		x = (320 - l*8)/2;	//johnfitz -- 320x200 coordinate system
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);	//johnfitz -- stretch overlays
			if (!remaining--)
				return;
		}

		y += 8;
		start += l;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);

	GL_PopCanvasColor ();
}

void SCR_CheckDrawCenterString (void)
{
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (!M_ForcedCenterPrint (NULL))
	{
		if (key_dest != key_game)
			return;
		if (cl.paused) //johnfitz -- don't show centerprint during a pause
			return;
	}

	SCR_DrawCenterString ();
}

void SCR_ClearCenterString (void)
{
	scr_centertime_off = 0;
}

//=============================================================================

/*
====================
SCR_ToggleZoom_f
====================
*/
static void SCR_ToggleZoom_f (void)
{
	if (cl.zoomdir)
		cl.zoomdir = -cl.zoomdir;
	else
		cl.zoomdir = cl.zoom > 0.5f ? -1.f : 1.f;
}

/*
====================
SCR_ZoomDown_f
====================
*/
static void SCR_ZoomDown_f (void)
{
	cl.zoomdir = 1.f;
}

/*
====================
SCR_ZoomUp_f
====================
*/
static void SCR_ZoomUp_f (void)
{
	cl.zoomdir = -1.f;
}

/*
====================
SCR_UpdateZoom
====================
*/
void SCR_UpdateZoom (void)
{
	float speed = scr_zoomspeed.value > 0.f ? scr_zoomspeed.value : 1e6;
	float delta = cl.zoomdir * speed * (cl.time - cl.oldtime);
	if (!delta)
		return;
	cl.zoom += delta;
	if (cl.zoom >= 1.f)
	{
		cl.zoom = 1.f;
		cl.zoomdir = 0.f;
	}
	else if (cl.zoom <= 0.f)
	{
		cl.zoom = 0.f;
		cl.zoomdir = 0.f;
	}
	vid.recalc_refdef = 1;
}

/*
====================
AdaptFovx
Adapt a 4:3 horizontal FOV to the current screen size using the "Hor+" scaling:
2.0 * atan(width / height * 3.0 / 4.0 * tan(fov_x / 2.0))
====================
*/
float AdaptFovx (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	if (!scr_fov_adapt.value)
		return fov_x;
	if ((x = height / width) == 0.75)
		return fov_x;
	a = atan(0.75 / x * tan(fov_x / 360 * M_PI));
	a = a * 360 / M_PI;
	return a;
}

/*
====================
CalcFovy
====================
*/
float CalcFovy (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);
	a = atan(height / x);
	a = a * 360 / M_PI;
	return a;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	float		size, scale; //johnfitz -- scale
	float		zoom;

// force the status bar to redraw
	Sbar_Changed ();

	scr_tileclear_updates = 0; //johnfitz

// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetQuick (&scr_viewsize, "30");
	if (scr_viewsize.value > 130)
		Cvar_SetQuick (&scr_viewsize, "130");

// bound fov
	if (scr_fov.value < 10)
		Cvar_SetQuick (&scr_fov, "10");
	if (scr_fov.value > 170)
		Cvar_SetQuick (&scr_fov, "170");
	if (scr_zoomfov.value < 10)
		Cvar_SetQuick (&scr_zoomfov, "10");
	if (scr_zoomfov.value > 170)
		Cvar_SetQuick (&scr_zoomfov, "170");

	vid.recalc_refdef = 0;

	//johnfitz -- rewrote this section
	size = scr_viewsize.value;
	scale = CLAMP (1.0f, scr_sbarscale.value, (float)glwidth / 320.0f);
	scale *= (float) vid.height / vid.guiheight;

	if (size >= 120 || cl.intermission || scr_sbaralpha.value < 1 || hudstyle != HUD_CLASSIC || cl.qcvm.extfuncs.CSQC_DrawHud) //johnfitz -- scr_sbaralpha.value
		sb_lines = 0;
	else if (size >= 110)
		sb_lines = 24 * scale;
	else
		sb_lines = 48 * scale;

	size = q_min(scr_viewsize.value, 100.f) / 100;
	//johnfitz

	//johnfitz -- rewrote this section
	r_refdef.vrect.width = q_max(glwidth * size, 96.0f); //no smaller than 96, for icons
	r_refdef.vrect.height = q_min((int)(glheight * size), glheight - sb_lines); //make room for sbar
	r_refdef.vrect.x = (glwidth - r_refdef.vrect.width)/2;
	r_refdef.vrect.y = (glheight - sb_lines - r_refdef.vrect.height)/2;
	//johnfitz

	zoom = cl.zoom;
	zoom *= zoom * (3.f - 2.f * zoom); // smoothstep
	r_refdef.basefov = LERP (scr_fov.value, scr_zoomfov.value, zoom);
	r_refdef.fov_x = AdaptFovx (r_refdef.basefov, vid.width, vid.height);
	r_refdef.fov_y = CalcFovy (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	scr_vrect = r_refdef.vrect;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value-10);
}

static void SCR_Callback_refdef (cvar_t *var)
{
	vid.recalc_refdef = 1;
}

/*
==================
SCR_Conwidth_f -- johnfitz -- called when scr_conwidth or scr_conscale changes
==================
*/
void SCR_Conwidth_f (cvar_t *var)
{
	vid.recalc_refdef = 1;
	VID_RecalcConsoleSize ();
}

/*
==================
SCR_AutoScale_f

Sets UI scale cvars to an automatic value based on resolution
==================
*/
void SCR_AutoScale_f (void)
{
	float scale = q_min (glwidth / 640.f, glheight / 480.f);
	scale = q_max (1.f, scale);
	Cvar_SetValueQuick (&scr_conscale, scale);
	Cvar_SetValueQuick (&scr_menuscale, scale);
	Cvar_SetValueQuick (&scr_sbarscale, scale);
	Cvar_SetValueQuick (&scr_crosshairscale, scale);
}

/*
==================
SCR_PixelAspect_f
==================
*/
void SCR_PixelAspect_f (cvar_t *cvar)
{
	vid.recalc_refdef = 1;
	VID_RecalcInterfaceSize ();
}

/*
==================
SCR_HUDStyle_f

Updates hudstyle variable and invalidates refdef when scr_hudstyle changes
==================
*/
void SCR_HUDStyle_f (cvar_t *cvar)
{
	int val = (int) cvar->value;
	hudstyle = (hudstyle_t) CLAMP (0, val, (int) HUD_COUNT - 1);
	vid.recalc_refdef = 1;
}

//============================================================================

/*
==================
SCR_LoadPics -- johnfitz
==================
*/
void SCR_LoadPics (void)
{
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");
}

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	//johnfitz -- new cvars
	Cvar_RegisterVariable (&scr_menuscale);
	Cvar_RegisterVariable (&scr_menubgalpha);
	Cvar_RegisterVariable (&scr_menubgstyle);
	Cvar_RegisterVariable (&scr_centerprintbg);
	Cvar_RegisterVariable (&scr_sbarscale);
	Cvar_SetCallback (&scr_sbaralpha, SCR_Callback_refdef);
	Cvar_RegisterVariable (&scr_sbaralpha);
	Cvar_SetCallback (&scr_conwidth, &SCR_Conwidth_f);
	Cvar_SetCallback (&scr_conscale, &SCR_Conwidth_f);
	Cvar_RegisterVariable (&scr_conwidth);
	Cvar_RegisterVariable (&scr_conscale);
	Cvar_RegisterVariable (&scr_crosshairscale);
	Cvar_RegisterVariable (&scr_showfps);
	Cvar_RegisterVariable (&scr_showspeed);
	Cvar_RegisterVariable (&scr_showspeed_ofs);
	Cvar_RegisterVariable (&scr_clock);
	Cvar_RegisterVariable (&cl_screenshotname);
	Cvar_RegisterVariable (&scr_demobar_timeout);
	//johnfitz
	Cvar_RegisterVariable (&scr_usekfont); // 2021 re-release
	Cvar_SetCallback (&scr_fov, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_fov_adapt, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_zoomfov, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_viewsize, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_hudstyle, SCR_HUDStyle_f);
	Cvar_RegisterVariable (&scr_hudstyle);
	Cvar_RegisterVariable (&scr_fov);
	Cvar_RegisterVariable (&scr_fov_adapt);
	Cvar_RegisterVariable (&scr_zoomfov);
	Cvar_RegisterVariable (&scr_zoomspeed);
	Cvar_RegisterVariable (&scr_viewsize);
	Cvar_RegisterVariable (&scr_conspeed);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_printspeed);
	Cvar_RegisterVariable (&gl_triplebuffer);
	Cvar_RegisterVariable (&cl_gun_fovscale);
	Cvar_RegisterVariable (&cl_gun_x);
	Cvar_RegisterVariable (&cl_gun_y);
	Cvar_RegisterVariable (&cl_gun_z);

	Cmd_AddCommand ("scr_autoscale",SCR_AutoScale_f);

	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	Cmd_AddCommand ("togglezoom", SCR_ToggleZoom_f);
	Cmd_AddCommand ("+zoom", SCR_ZoomDown_f);
	Cmd_AddCommand ("-zoom", SCR_ZoomUp_f);

	SCR_LoadPics (); //johnfitz

	scr_initialized = true;
}

//============================================================================

/*
==============
SCR_DrawFPS -- johnfitz
==============
*/
void SCR_DrawFPS (void)
{
	static double	oldtime = 0;
	static double	lastfps = 0;
	static int	oldframecount = 0;
	double	elapsed_time;
	int	frames;

	if (con_forcedup)
	{
		oldtime = realtime;
		oldframecount = r_framecount;
		lastfps = 0;
		return;
	}

	elapsed_time = realtime - oldtime;
	frames = r_framecount - oldframecount;

	if (elapsed_time < 0 || frames < 0)
	{
		oldtime = realtime;
		oldframecount = r_framecount;
		return;
	}
	// update value every 3/4 second
	if (elapsed_time > 0.75)
	{
		lastfps = frames / elapsed_time;
		oldtime = realtime;
		oldframecount = r_framecount;
	}

	if (scr_showfps.value && scr_viewsize.value < 130 && lastfps)
	{
		char	st[16];
		int	x, y;
		if (scr_showfps.value > 0.f)
			sprintf (st, "%4.0f fps", lastfps);
		else
			sprintf (st, "%.2f ms", 1000.f / lastfps);
		x = 320 - (strlen(st)<<3);
		if (hudstyle != HUD_CLASSIC)
		{
			x = 320 - 16 - (strlen(st)<<3);
			y = 8;
			if (scr_clock.value) y += 8; //make room for clock
			GL_SetCanvas (CANVAS_TOPRIGHT);
		}
		else
		{
			x = 320 - (strlen(st)<<3);
			y = 200 - 8;
			if (scr_clock.value) y -= 8; //make room for clock
			GL_SetCanvas (CANVAS_BOTTOMRIGHT);
		}
		Draw_String (x, y, st);
		scr_tileclear_updates = 0;
	}
}

/*
==============
SCR_DrawSpeed
==============
*/
void SCR_DrawSpeed (void)
{
	if (cl.intermission || CL_InCutscene () || scr_viewsize.value >= 130)
		return;

	const float show_speed_interval_value = 0.05f;
	static float maxspeed = 0, display_speed = -1;
	static double lastrealtime = 0;
	float speed;
	vec3_t vel;

	if (lastrealtime > realtime)
	{
		lastrealtime = 0;
		display_speed = -1;
		maxspeed = 0;
	}

	VectorCopy (cl.velocity, vel);
	vel[2] = 0;
	speed = VectorLength (vel);

	if (speed > maxspeed)
		maxspeed = speed;

	if (scr_showspeed.value)
	{
		if (display_speed >= 0)
		{
			float y;
			char str[12];

			sprintf (str, "%d", (int) display_speed);
			GL_SetCanvas (CANVAS_CROSSHAIR);
			y = CLAMP (glcanvas.top, 4.f + scr_showspeed_ofs.value, glcanvas.bottom - 8.f);
			Draw_String (-(int)strlen(str)*4, y, str);
		}
	}

	if (realtime - lastrealtime >= show_speed_interval_value)
	{
		lastrealtime = realtime;
		display_speed = maxspeed;
		maxspeed = 0;
	}
}

/*
==============
SCR_DrawClock -- johnfitz
==============
*/
void SCR_DrawClock (void)
{
	char	str[12];

	if (scr_clock.value == 1 && scr_viewsize.value < 130)
	{
		int minutes, seconds;

		minutes = cl.time / 60;
		seconds = ((int)cl.time)%60;

		sprintf (str,"%i:%i%i", minutes, seconds/10, seconds%10);
	}
	else
		return;

	//draw it
	if (hudstyle == HUD_CLASSIC)
	{
		GL_SetCanvas (CANVAS_BOTTOMRIGHT);
		Draw_String (320 - (strlen(str)<<3), 200 - 8, str);
	}
	else
	{
		GL_SetCanvas (CANVAS_TOPRIGHT);
		Draw_String (320 - 16 - (strlen(str)<<3), 8, str);
	}

	scr_tileclear_updates = 0;
}

/*
==============
SCR_PrintMirrored
==============
*/
static void SCR_PrintMirrored (int x, int y, const char *str)
{
	x += strlen (str) * 8;
	while (*str)
	{
		Draw_CharacterEx (x, y, -8, 8, 0x80 ^ (*str++));
		x -= 8;
	}
}

/*
==============
SCR_DrawDemoControls
==============
*/
void SCR_DrawDemoControls (void)
{
	static const int	TIMEBAR_CHARS = 38;
	static float		prevspeed = 1.0f;
	static float		prevbasespeed = 1.0f;
	static float		showtime = 1.0f;
	int					i, len, x, y, min, sec;
	float				frac;
	const char			*str;
	char				name[31]; // size chosen to avoid overlap with side text

	if (!cls.demoplayback || scr_demobar_timeout.value < 0.f)
	{
		showtime = 0.f;
		return;
	}

	// Determine for how long the demo playback info should be displayed
	if (cls.demospeed != prevspeed || cls.basedemospeed != prevbasespeed ||			// speed/base speed changed
		fabs (cls.demospeed) > cls.basedemospeed ||									// fast forward/rewind
		!scr_demobar_timeout.value)													// controls always shown
	{
		prevspeed = cls.demospeed;
		prevbasespeed = cls.basedemospeed;
		showtime = scr_demobar_timeout.value > 0.f ? scr_demobar_timeout.value : 1.f;
	}
	else
	{
		showtime -= host_rawframetime;
		if (showtime < 0.f)
		{
			showtime = 0.f;
			return;
		}
	}

	// Approximate the fraction of the demo that's already been played back
	// based on the current file offset and total demo size
	frac = QFS_Tell (cls.inpdemo) / (double)cls.demofilesize;
	frac = CLAMP (0.f, frac, 1.f);

	if (cl.intermission)
	{
		GL_SetCanvas (CANVAS_MENU);
		y = LERP (glcanvas.bottom, glcanvas.top, 0.125f) + 8;
	}
	else
	{
		GL_SetCanvas (CANVAS_SBAR);
		y = glcanvas.bottom - 68;
	}
	x = (glcanvas.left + glcanvas.right) / 2 - TIMEBAR_CHARS / 2 * 8;

	// Draw status box background
	GL_SetCanvasColor (1.f, 1.f, 1.f, scr_sbaralpha.value);
	M_DrawTextBox (x - 8, y - 8, TIMEBAR_CHARS, 1);
	GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);

	// Print playback status on the left (paused/playing/fast-forward/rewind)
	// Note: character #13 works well as a forward symbol, but Alkaline 1.2 changes it to a disk.
	// If we have a custom conchars texture we switch to a safer alternative, the '>' character.
	if (!cls.demospeed)
		str = "II";
	else if (fabs (cls.demospeed) > 1.f)
		str = custom_conchars ? ">>" : "\xD\xD";
	else 
		str = custom_conchars ? ">" : "\xD";
	if (cls.demospeed >= 0.f)
		M_Print (x, y, str);
	else
		SCR_PrintMirrored (x, y, str);

	// Print base playback speed on the right
	if (!cls.basedemospeed)
		str = "";
	else if (fabs (cls.basedemospeed) >= 1.f)
		str = va ("%gx", fabs (cls.basedemospeed));
	else
		str = va ("1/%gx", 1.f / fabs (cls.basedemospeed));
	M_Print (x + (TIMEBAR_CHARS - strlen (str)) * 8, y, str);

	// Print demo name in the center
	COM_StripExtension (COM_SkipPath (cls.demofilename), name, sizeof (name));
	x = (glcanvas.left + glcanvas.right) / 2;
	M_Print (x - strlen (name) * 8 / 2, y, name);

	// Draw seek bar rail
	x = (glcanvas.left + glcanvas.right) / 2 - TIMEBAR_CHARS / 2 * 8;
	y -= 8;
	Draw_Character (x - 8, y, 128);
	for (i = 0; i < TIMEBAR_CHARS; i++)
		Draw_Character (x + i * 8, y, 129);
	Draw_Character (x + i * 8, y, 130);

	// Draw seek bar cursor
	x += (TIMEBAR_CHARS - 1) * 8 * frac;
	Draw_Character (x, y, 131);

	// Print current time above the cursor
	y -= 11;
	sec = (int) cl.time;
	min = sec / 60;
	sec %= 60;
	str = va ("%i:%02i", min, sec);
	x -= (strchr (str, ':') - str) * 8; // align ':' with cursor
	len = strlen (str);
	// M_DrawTextBox effectively rounds width up to a multiple of 2,
	// so if our length is odd we pad by half a character on each side
	GL_SetCanvasColor (1.f, 1.f, 1.f, scr_sbaralpha.value);
	M_DrawTextBox (x - 8 - (len & 1) * 8 / 2, y - 8, len + (len & 1), 1);
	GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);
	Draw_String (x, y, str);
}

/*
==============
SCR_DrawDevStats
==============
*/
void SCR_DrawDevStats (void)
{
	char	str[40];
	int		y = 25-10; //10=number of lines to print
	int		x = 0; //margin

	if (!devstats.value)
		return;

	GL_SetCanvas (CANVAS_BOTTOMLEFT);

	Draw_Fill (x, y*8, 21*8, 10*8, 0, 0.5); //dark rectangle

	sprintf (str, "devstats | Curr  Peak");
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "---------+-----------");
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Edicts   |%5i %5i", dev_stats.edicts, dev_peakstats.edicts);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Packet   |%5i %5i", dev_stats.packetsize, dev_peakstats.packetsize);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Visedicts|%5i %5i", dev_stats.visedicts, dev_peakstats.visedicts);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Efrags   |%5i %5i", dev_stats.efrags, dev_peakstats.efrags);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Dlights  |%5i %5i", dev_stats.dlights, dev_peakstats.dlights);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Beams    |%5i %5i", dev_stats.beams, dev_peakstats.beams);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Tempents |%5i %5i", dev_stats.tempents, dev_peakstats.tempents);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "GL upload|%4iK %4iK", dev_stats.gpu_upload/1024, dev_peakstats.gpu_upload/1024);
	Draw_String (x, (y++)*8-x, str);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;
	float	alpha;

	if (!cl.paused)
		return;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	if (M_ForcedCenterPrint (&alpha))
	{
		alpha = 1.f - alpha;
		if (alpha <= 0.f)
			return;
	}
	else
		alpha = 1.f;

	GL_SetCanvas (CANVAS_MENU); //johnfitz
	GL_PushCanvasColor (1.f, 1.f, 1.f, alpha);

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

	GL_PopCanvasColor ();

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	qpic_t	*pic;

	if (!scr_drawloading)
		return;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
SCR_DrawSaving
==============
*/
void SCR_DrawSaving (void)
{
	int x, y;

	if (!Host_IsSaving ())
		return;

	GL_SetCanvas (CANVAS_TOPRIGHT);

	x = 320 - 16 - draw_disc->width;
	y = 8;
	if (hudstyle != HUD_CLASSIC && scr_viewsize.value < 130)
	{
		if (scr_clock.value) y += 8;
		if (scr_showfps.value) y += 8;
		if (y != 8)
			y += 8;
	}

	Draw_Pic (x, y, draw_disc);
}

/*
==============
SCR_DrawCrosshair -- johnfitz
==============
*/
void SCR_DrawCrosshair (void)
{
	if (cl.intermission || CL_InCutscene () || !crosshair.value || scr_viewsize.value >= 130)
		return;

	GL_SetCanvas (CANVAS_CROSSHAIR);
	Draw_Character(-4, -4, crosshair_char);
}

typedef struct
{
	float	scale[2];
	float	offset[2];
} canvasmap_t;

/*
==============
SCR_SetupProjToCanvasMap

Computes mapping from NDC coordinates (-1..1) to canvas coordinates
Note: canvas Y increases from top to bottom, GL Y is bottom to top
==============
*/
static void SCR_SetupProjToCanvasMap (canvasmap_t *remap)
{
	float	xmin, ymin, width, height;

	width = r_refdef.vrect.width / (float)glwidth * (glcanvas.right - glcanvas.left);
	height = r_refdef.vrect.height / (float)glheight * (glcanvas.top - glcanvas.bottom);
	xmin = glcanvas.left + (r_refdef.vrect.x / (float)glwidth) * (glcanvas.right - glcanvas.left);
	ymin = glcanvas.bottom - ((glheight - r_refdef.vrect.y - r_refdef.vrect.height) / (float)glheight) * (glcanvas.bottom - glcanvas.top);

	remap->scale[0]  = width * 0.5f;
	remap->offset[0] = width * 0.5f + xmin;
	remap->scale[1]  = height * 0.5f;
	remap->offset[1] = height * 0.5f + ymin;
}

/*
==============
SCR_ProjToCanvas
==============
*/
static void SCR_ProjToCanvas (const vec3_t proj, const canvasmap_t *remap, float *outx, float *outy)
{
	*outx = proj[0] * remap->scale[0] + remap->offset[0];
	*outy = proj[1] * remap->scale[1] + remap->offset[1];
}

/*
==============
SCR_GetEntityBottom
==============
*/
static void SCR_GetEntityBottom (const edict_t *ed, vec3_t pos)
{
	pos[0] = ed->v.origin[0] + (ed->v.mins[0] + ed->v.maxs[0]) * 0.5f;
	pos[1] = ed->v.origin[1] + (ed->v.mins[1] + ed->v.maxs[1]) * 0.5f;
	pos[2] = ed->v.origin[2] + ed->v.mins[2];

	// If it's a point entity, move anchor point down by 8 units to avoid overlapping debug visualization
	if (VectorCompare (ed->v.mins, ed->v.maxs))
		pos[2] -= 8.f;
}

/*
==============
SCR_GetEntityCenter
==============
*/
static void SCR_GetEntityCenter (const edict_t *ed, vec3_t pos)
{
	if (!VectorCompare (ed->v.mins, ed->v.maxs))
	{
		pos[0] = ed->v.origin[0] + (ed->v.mins[0] + ed->v.maxs[0]) * 0.5f;
		pos[1] = ed->v.origin[1] + (ed->v.mins[1] + ed->v.maxs[1]) * 0.5f;
		pos[2] = ed->v.origin[2] + (ed->v.mins[2] + ed->v.maxs[2]) * 0.5f;
	}
	else
	{
		VectorCopy (ed->v.origin, pos);
	}
}

/*
==============
SCR_ClipToFrustum

Clips the position to the view frustum towards a reference point.
==============
*/
static void SCR_ClipToFrustum (vec3_t pos, const vec3_t ref)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		mplane_t *plane = &frustum[i];
		float dist = DotProduct (plane->normal, pos) - plane->dist;
		if (dist < 0.f)
		{
			float ref_dist = DotProduct (plane->normal, ref) - plane->dist;
			if (ref_dist != dist)
				VectorLerp (pos, ref, -dist / (ref_dist - dist), pos);
		}
	}
}

/*
==============
SCR_DrawKeyValueOverlay
==============
*/
static void SCR_DrawKeyValueOverlay (float x, float y, const char *multistr, const vec3_t bgcolor)
{
	int			ofs, numlines, keylen, vallen, maxkey, maxval;
	const char	*key, *val;

	// Compute column widths and total number of lines
	for (ofs = numlines = maxkey = maxval = 0; ofs < (int) VEC_SIZE (multistr); numlines++)
	{
		key = multistr + ofs;
		keylen = (int) strlen (key);
		ofs += keylen + 1;

		val = multistr + ofs;
		vallen = (int) strlen (val);
		ofs += vallen + 1;

		if (keylen == 0)
		{
			// empty key, value is printed centered
			keylen = vallen * CHARSIZE / 2;
			maxkey = q_max (maxkey, keylen);
			maxval = q_max (maxval, keylen);
		}
		else
		{
			// key/value pair, we leave half a character on each side for spacing
			maxkey = q_max (maxkey, keylen * CHARSIZE + CHARSIZE / 2);
			maxval = q_max (maxval, vallen * CHARSIZE + CHARSIZE / 2);
		}
	}

	// Try to keep as much text as possible on screen
	if (y > glcanvas.bottom - numlines * CHARSIZE)
		y = glcanvas.bottom - numlines * CHARSIZE;
	if (y < glcanvas.top)
		y = glcanvas.top;
	if (x > glcanvas.right - maxval)
		x = glcanvas.right - maxval;
	if (x < glcanvas.left + maxkey)
		x = glcanvas.left + maxkey;

	// Draw background box
	Draw_FillEx (x - maxkey, y, maxkey + maxval, numlines * CHARSIZE, bgcolor, 0.75f);

	// Print each line
	for (ofs = 0; ofs < (int) VEC_SIZE (multistr); y += CHARSIZE)
	{
		key = multistr + ofs;
		keylen = (int) strlen (key);
		ofs += keylen + 1;

		val = multistr + ofs;
		vallen = (int) strlen (val);
		ofs += vallen + 1;

		if (keylen == 0)
		{
			// empty key, center-print the value
			Draw_StringEx (x - vallen * CHARSIZE / 2, y, CHARSIZE, val);
		}
		else
		{
			// key/value pair, leave half a character on each side for spacing
			Draw_StringEx (x - CHARSIZE / 2 - keylen * CHARSIZE, y, CHARSIZE, key);
			Draw_StringEx (x + CHARSIZE / 2, y, CHARSIZE, val);
		}
	}
}

/*
==============
SCR_DrawEdictInfo

Show info for the highlighted entity with r_showfields/r_showbboxes
==============
*/

static char *scr_edictoverlaystrings = NULL;

void SCR_DrawEdictInfo (void)
{
	char		tinted[1024];
	int			i;
	float		x, y;
	canvasmap_t	proj2canvas;
	vec3_t		crosshair, focus, anchor, proj, bgcolor;
	edict_t		*ed;

	if (VEC_SIZE (bbox_linked) == 0 && VEC_SIZE (r_pointfile) == 0)
		return;

	GL_SetCanvas (CANVAS_BOTTOMRIGHT);
	SCR_SetupProjToCanvasMap (&proj2canvas);
	VectorMA (r_origin, 8.f, vpn, crosshair);

	// If a pointfile was loaded, print "Leak" at the beginning
	if (VEC_SIZE (r_pointfile) != 0)
	{
		VectorCopy (r_pointfile[0], anchor);
		SCR_ClipToFrustum (anchor, crosshair);
		ProjectVector (anchor, r_matviewproj, proj);
		SCR_ProjToCanvas (proj, &proj2canvas, &x, &y);

		VEC_CLEAR (scr_edictoverlaystrings);
		MultiString_Append (&scr_edictoverlaystrings, "");
		COM_TintString ("Leak", tinted, sizeof (tinted));
		MultiString_Append (&scr_edictoverlaystrings, tinted);

		SCR_DrawKeyValueOverlay (x, y, scr_edictoverlaystrings, rgb_black);
	}

	if (VEC_SIZE (bbox_linked) == 0)
		return;

	PR_SwitchQCVM (&sv.qcvm);

	SCR_GetEntityCenter (bbox_linked[0], focus);
	SCR_ClipToFrustum (focus, crosshair);

	// Show edict numbers and classnames for all highlighted entities.
	// Note: reversed order, so that the focused entity is drawn last,
	// on top of all the others.
	for (i = (int) VEC_SIZE (bbox_linked) - 1; i >= 0; i--)
	{
		ed = bbox_linked[i];

		// Compute anchor point
		if (i == 0)
		{
			// With r_showfields_align 0 the field overlay tracks the focused entity,
			// so we disable the simple one (number + classname) to avoid overlap.
			if (!r_showfields_align.value)
				continue;
			//SCR_GetEntityBottom (ed, anchor);
			SCR_GetEntityCenter (ed, anchor);
			SCR_ClipToFrustum (anchor, crosshair);
		}
		else
		{
			SCR_GetEntityCenter (ed, anchor);
			SCR_ClipToFrustum (anchor, focus);
		}
		ProjectVector (anchor, r_matviewproj, proj);
		SCR_ProjToCanvas (proj, &proj2canvas, &x, &y);

		// Simple overlay: centered edict number and classname (if not empty)
		VEC_CLEAR (scr_edictoverlaystrings);
		MultiString_Append (&scr_edictoverlaystrings, "");
		MultiString_Append (&scr_edictoverlaystrings, va ("edict %d", NUM_FOR_EDICT (ed)));
		if (ed->v.classname)
		{
			MultiString_Append (&scr_edictoverlaystrings, "");
			MultiString_Append (&scr_edictoverlaystrings, PR_GetString (ed->v.classname));
		}

		// Set background color based on link type
		switch (ed->showbboxflags)
		{
		default:
		case SHOWBBOX_LINK_NONE:		VectorSet (bgcolor, 0.f,	0.f,	0.f); break;
		case SHOWBBOX_LINK_INCOMING:	VectorSet (bgcolor, 0.25f,	0.125f,	0.125f); break;
		case SHOWBBOX_LINK_OUTGOING:	VectorSet (bgcolor, 0.125f,	0.125f,	0.25f); break;
		case SHOWBBOX_LINK_BOTH:		VectorSet (bgcolor, 0.25f,	0.125f,	0.25f); break;
		}

		SCR_DrawKeyValueOverlay (x, y, scr_edictoverlaystrings, bgcolor);
	}

	// Show all relevant entity fields for the highlighted entity (first in bbox_linked)
	if (r_showfields.value)
	{
		ed = bbox_linked[0];

		// Compute anchor point
		SCR_GetEntityBottom (ed, anchor);
		ProjectVector (anchor, r_matviewproj, proj);
		SCR_ProjToCanvas (proj, &proj2canvas, &x, &y);

		// r_showfields_align 1 locks the overlay to the bottom-right
		if (r_showfields_align.value)
		{
			x = glcanvas.right;
			y = glcanvas.bottom;
		}

		// Build overlay text: first 2 lines are entity number and classname
		VEC_CLEAR (scr_edictoverlaystrings);
		MultiString_Append (&scr_edictoverlaystrings, "Edict");
		MultiString_Append (&scr_edictoverlaystrings, va ("%d", NUM_FOR_EDICT (ed)));
		COM_TintString ("classname", tinted, sizeof (tinted));
		MultiString_Append (&scr_edictoverlaystrings, tinted);
		MultiString_Append (&scr_edictoverlaystrings, PR_GetString (ed->v.classname));

		// Add all relevant fields, excluding classname (already added to the header)
		for (i = 1; i < qcvm->progs->numfielddefs; i++)
		{
			stringview_t	line;
			const char		*val;
			ddef_t			*d = &qcvm->fielddefs[i];

			if (d->ofs*4 == offsetof (entvars_t, classname) || !ED_IsRelevantField (ed, d))
				continue;

			COM_TintString (PR_GetString (d->s_name), tinted, sizeof (tinted));
			val = ED_FieldValueString (ed, d);

			while (COM_ParseLine (&val, &line))
			{
				MultiString_Append (&scr_edictoverlaystrings, tinted);
				MultiString_AppendN (&scr_edictoverlaystrings, line.data, line.len);
				tinted[0] = ' ';
				tinted[1] = '\0';
			}
		}

		SCR_DrawKeyValueOverlay (x, y, scr_edictoverlaystrings, rgb_black);
	}

	PR_SwitchQCVM (NULL);
}


//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	//johnfitz -- let's hack away the problem of slow console when host_timescale is <0
	extern cvar_t host_timescale;
	float timescale, conspeed;
	//johnfitz

	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = glheight; //full screen //johnfitz -- glheight instead of vid.height
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console || M_WantsConsole (NULL))
		scr_conlines = glheight/2; //half screen //johnfitz -- glheight instead of vid.height
	else
		scr_conlines = 0; //none visible

	timescale = (host_timescale.value > 0) ? host_timescale.value : 1; //johnfitz -- timescale
	conspeed = (scr_conspeed.value > 0 && !cls.timedemo) ? scr_conspeed.value : 1e6f;

	// make sure the console isn't too slow to come down if we're forcing it open to preview a specific option
	if (key_dest == key_menu)
		conspeed = q_max (conspeed, 2000.f);

	if (scr_conlines < scr_con_current)
	{
		// ericw -- (glheight/600.0) factor makes conspeed resolution independent, using 800x600 as a baseline
		scr_con_current -= conspeed*(glheight/600.0)*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;
	}
	else if (scr_conlines > scr_con_current)
	{
		// ericw -- (glheight/600.0)
		scr_con_current += conspeed*(glheight/600.0)*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
		Sbar_Changed ();

	if (!con_forcedup && scr_con_current)
		scr_tileclear_updates = 0; //johnfitz
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		if (key_dest != key_menu || (!con_forcedup || M_WantsConsole (NULL)))
			Con_DrawConsole (scr_con_current, true, key_dest == key_console);
		else
			Draw_ConsoleBackground ();
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/

/*
==================
SCR_GetCleanMapTitle
==================
*/
static void SCR_GetCleanMapTitle (char *buf, size_t maxchars)
{
	char clean[countof (cl.levelname)];
	size_t i, j;

	for (i = j = 0; i + 1 < countof (cl.levelname) && cl.levelname[i]; i++)
	{
		char c = cl.levelname[i] & 0x7f;
		switch (c)
		{
		case '/':
		case '|':
		case ':':
		case '\n':
			c = '-';
			break;
		case '*':
			c = '=';
			break;
		case '<':
			c = '[';
			break;
		case '>':
			c = ']';
			break;
		case '?':
		case '!':
			c = '.';
			break;
		case '"':
			c = '\'';
			break;
		case '\t':
			c = ' ';
			break;
		case '\\':
			if (i + 1 < countof (cl.levelname) && cl.levelname[i] == 'n')
				i++;
			c = '-';
			break;
		default:
			break;
		}
		// remove leading spaces, replace consecutive spaces with a single one
		if (c != ' ' || (j > 0 && clean[j - 1] != c))
			clean[j++] = c;
	}
	clean[j++] = '\0';

	UTF8_FromQuake (buf, maxchars, clean);
}

/*
==================
SCR_ExpandVariables

Returns true if format contains any variables
==================
*/
static qboolean SCR_ExpandVariables (const char *fmt, char *dst, size_t maxchars)
{
	time_t		now;
	struct tm	*lt;
	char		var[256];
	size_t		i, j, k, numvars;

	if (!maxchars)
		return false;
	--maxchars;

	time (&now);
	lt = localtime (&now);

	for (i = j = numvars = 0; j < maxchars && fmt[i]; /**/)
	{
		if (fmt[i] != '%')
		{
			dst[j++] = fmt[i];
			i++;
			continue;
		}

		i++;
		if (fmt[i] == '%')
		{
			dst[j++] = '%';
			i++;
			continue;
		}

		// find closing %
		for (k = 0; fmt[i + k]; k++)
			if (fmt[i + k] == '%')
				break;

		#define IS_VAR(s)	(k == strlen (s) && q_strncasecmp (fmt + i, s, k) == 0)

		if (IS_VAR ("map"))
		{
			if (cls.state == ca_connected && cls.signon == SIGNONS)
				q_strlcpy (var, cl.mapname, sizeof (var));
			else if (key_dest == key_menu)
				q_strlcpy (var, "menu", sizeof (var));
			else
				q_strlcpy (var, "console", sizeof (var));
		}
		else if (IS_VAR ("maptitle"))
		{
			if (cls.state == ca_connected && cls.signon == SIGNONS)
				SCR_GetCleanMapTitle (var, sizeof (var));
			else if (key_dest == key_menu)
				q_strlcpy (var, "menu", sizeof (var));
			else
				q_strlcpy (var, "console", sizeof (var));
		}
		else if (IS_VAR ("date"))
			q_snprintf (var, sizeof (var), "%04d-%02d-%02d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
		else if (IS_VAR ("time"))
			q_snprintf (var, sizeof (var), "%02d-%02d-%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
		else if (IS_VAR ("year"))
			q_snprintf (var, sizeof (var), "%04d", lt->tm_year + 1900);
		else if (IS_VAR ("month"))
			q_snprintf (var, sizeof (var), "%02d", lt->tm_mon + 1);
		else if (IS_VAR ("day"))
			q_snprintf (var, sizeof (var), "%02d", lt->tm_mday);
		else if (IS_VAR ("hour"))
			q_snprintf (var, sizeof (var), "%02d", lt->tm_hour);
		else if (IS_VAR ("min"))
			q_snprintf (var, sizeof (var), "%02d", lt->tm_min);
		else if (IS_VAR ("sec"))
			q_snprintf (var, sizeof (var), "%02d", lt->tm_sec);
		else // unknown variable, write name and percentage signs
			q_snprintf (var, sizeof (var), "%%%.*s%%", (int)k, fmt + i);

		#undef IS_VAR

		// advance format cursor
		i += k;
		if (fmt[i]) // skip closing %
			i++;

		// append variable value
		k = strlen (var);
		if (k > maxchars - j)
			k = maxchars - j;
		memcpy (dst + j, var, k);
		j += k;
		numvars++;
	}

	dst[j++] = '\0';

	return numvars != 0;
}

static void SCR_ScreenShot_Usage (void)
{
	Con_Printf ("usage: screenshot <format> <quality>\n");
	Con_Printf ("   format must be \"png\" or \"tga\" or \"jpg\"\n");
	Con_Printf ("   quality must be 1-100\n");
}

/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (void)
{
	byte	*buffer;
	char	ext[4];
	int		i, quality;

	Q_strncpy (ext, "png", sizeof(ext));

	if (Cmd_Argc () >= 2)
	{
		const char	*requested_ext = Cmd_Argv (1);

		if (!q_strcasecmp ("png", requested_ext)
		    || !q_strcasecmp ("tga", requested_ext)
		    || !q_strcasecmp ("jpg", requested_ext))
			Q_strncpy (ext, requested_ext, sizeof(ext));
		else
		{
			SCR_ScreenShot_Usage ();
			return;
		}
	}

// read quality as the 3rd param (only used for JPG)
	quality = 90;
	if (Cmd_Argc () >= 3)
		quality = Q_atoi (Cmd_Argv(2));
	if (quality < 1 || quality > 100)
	{
		SCR_ScreenShot_Usage ();
		return;
	}

//get data
	if (!(buffer = (byte *) malloc(glwidth*glheight*3)))
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't allocate memory\n");
		return;
	}

	if (scr_viewsize.value >= 130)
	{
		qboolean oldskip = scr_skipupdate;
		Con_ClearNotify ();
		SCR_ClearCenterString ();
		scr_skipupdate = 1; // don't swap buffers at end of frame
		SCR_UpdateScreen ();
		scr_skipupdate = oldskip;
	}

	glPixelStorei (GL_PACK_ALIGNMENT, 1);/* for widths that aren't a multiple of 4 */
	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer);

// now write the file
	if (!Steam_SaveScreenshot (buffer, glwidth, glheight))
	{
		char		basename[MAX_OSPATH];
		char		imagename[MAX_OSPATH];
		char		checkname[MAX_OSPATH];
		qboolean	ok, has_vars;

	// find a file name to save it to
		has_vars = SCR_ExpandVariables (cl_screenshotname.string, basename, sizeof (basename));
		if (!basename[0])
			q_strlcpy (basename, SCREENSHOT_PREFIX, sizeof (basename));

		if (!has_vars)
			goto append_index;

		q_snprintf (imagename, sizeof (imagename), "%s.%s", basename, ext);
		q_snprintf (checkname, sizeof (checkname), "%s/%s", com_gamedir, imagename);
		if (Sys_FileType (checkname) != FS_ENT_NONE) // base name already used, try appending an index
		{
		append_index:
			// append underscore if basename ends with a digit
			i = (int) strlen (basename);
			if (i && i + 1 < (int) countof (basename) && (unsigned int)(basename[i - 1] - '0') < 10u)
			{
				basename[i] = '_';
				basename[i + 1] = '\0';
			}

			for (i = has_vars; i < 10000; i++)
			{
				q_snprintf (imagename, sizeof (imagename), "%s%04i.%s", basename, i, ext);
				q_snprintf (checkname, sizeof (checkname), "%s/%s", com_gamedir, imagename);
				if (Sys_FileType (checkname) == FS_ENT_NONE)
					break;	// file doesn't exist
			}
			if (i == 10000)
			{
				Con_Printf ("SCR_ScreenShot_f: Couldn't find an unused filename\n");
				free (buffer);
				return;
			}
		}

		if (!q_strncasecmp (ext, "png", sizeof(ext)))
			ok = Image_WritePNG (imagename, buffer, glwidth, glheight, 24, false);
		else if (!q_strncasecmp (ext, "tga", sizeof(ext)))
			ok = Image_WriteTGA (imagename, buffer, glwidth, glheight, 24, false);
		else if (!q_strncasecmp (ext, "jpg", sizeof(ext)))
			ok = Image_WriteJPG (imagename, buffer, glwidth, glheight, 24, quality, false);
		else
			ok = false;

		UTF8_ToQuake (basename, sizeof (basename), imagename);
		if (ok)
		{
			Con_SafePrintf ("Wrote ");
			Con_LinkPrintf (va("%s/%s", com_gamedir, imagename), "%s", basename);
			Con_SafePrintf ("\n");
		}
		else
			Con_Printf ("SCR_ScreenShot_f: Couldn't create %s\n", basename);
	}

	free (buffer);
}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

// redraw with no console and the loading plaque
	if (key_dest != key_console)
	{
		Con_ClearNotify ();
		// don't reset console when advancing to a new demo while previewing a console option
		if (!M_WantsConsole (NULL))
			scr_con_current = 0;
		scr_drawloading = true;
	}

	scr_centertime_off = 0;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	Con_ClearNotify ();
}

//=============================================================================

const char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	const char	*start;
	int		l;
	int		j;
	int		x, y;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	start = scr_notifystring;

	y = 200 * 0.35; //johnfitz -- stretched overlays

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2; //johnfitz -- stretched overlays
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage (const char *text, float timeout) //johnfitz -- timeout
{
	double time1, time2; //johnfitz -- timeout
	int lastkey, lastchar;

	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;

// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;

	S_ClearBuffer ();		// so dma doesn't loop current sound

	time1 = Sys_DoubleTime () + timeout; //johnfitz -- timeout
	time2 = 0.0f; //johnfitz -- timeout

	Key_BeginInputGrab ();
	do
	{
		Sys_SendKeyEvents ();
		Key_GetGrabbedInput (&lastkey, &lastchar);
		Sys_Sleep (16);
		if (timeout) time2 = Sys_DoubleTime (); //johnfitz -- zero timeout means wait forever.
	} while (lastchar != 'y' && lastchar != 'Y' &&
		 lastchar != 'n' && lastchar != 'N' &&
		 lastkey != K_ESCAPE &&
		 lastkey != K_ABUTTON &&
		 lastkey != K_BBUTTON &&
		 lastkey != K_MOUSE2 &&
		 lastkey != K_MOUSE4 &&
		 time2 <= time1);
	Key_EndInputGrab ();

//	SCR_UpdateScreen (); //johnfitz -- commented out

	//johnfitz -- timeout
	if (time2 > time1)
		return false;
	//johnfitz

	return (lastchar == 'y' || lastchar == 'Y' || lastkey == K_ABUTTON);
}


//=============================================================================

/*
==================
SCR_TileClear
johnfitz -- modified to use glwidth/glheight instead of vid.width/vid.height
	    also fixed the dimentions of right and top panels
	    also added scr_tileclear_updates
==================
*/
void SCR_TileClear (void)
{
	//ericw -- added check for glsl gamma. TODO: remove this ugly optimization?
	if (scr_tileclear_updates >= vid.numpages && !gl_clear.value && vid_gamma.value == 1)
		return;
	scr_tileclear_updates++;

	if (r_refdef.vrect.x > 0)
	{
		// left
		Draw_TileClear (0,
						0,
						r_refdef.vrect.x,
						glheight - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width,
						0,
						glwidth - r_refdef.vrect.x - r_refdef.vrect.width,
						glheight - sb_lines);
	}

	if (r_refdef.vrect.y > 0)
	{
		// top
		Draw_TileClear (r_refdef.vrect.x,
						0,
						r_refdef.vrect.width,
						r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x,
						r_refdef.vrect.y + r_refdef.vrect.height,
						r_refdef.vrect.width,
						glheight - r_refdef.vrect.y - r_refdef.vrect.height - sb_lines);
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	vid.numpages = (gl_triplebuffer.value) ? 3 : 2;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet


	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	//
	// determine size of refresh window
	//
	if (vid.recalc_refdef)
		SCR_CalcRefdef ();
	r_refdef.scale = CLAMP (1, (int)r_scale.value, vid.maxscale);

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();

	V_UpdateBlend (); //johnfitz -- V_UpdatePalette cleaned up and renamed

	V_RenderView ();

	GL_BeginGroup ("2D");

	GL_Set2D ();

	//FIXME: only call this when needed
	SCR_TileClear ();

	if (scr_drawdialog) //new game confirm
	{
		if (con_forcedup)
			Draw_ConsoleBackground ();
		else
			Sbar_Draw ();
		Draw_FadeScreen (1.f);
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading) //loading
	{
		SCR_DrawLoading ();
		Sbar_Draw ();
		M_Draw ();
	}
	else if (cl.intermission == 1 && key_dest == key_game) //end of level
	{
		Sbar_IntermissionOverlay ();
		SCR_DrawDemoControls ();
	}
	else if (cl.intermission == 2 && key_dest == key_game) //end of episode
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
		SCR_DrawDemoControls ();
	}
	else
	{
		SCR_DrawCrosshair (); //johnfitz
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_CheckDrawCenterString ();
		Sbar_Draw ();
		SCR_DrawDevStats (); //johnfitz
		SCR_DrawClock (); //johnfitz
		SCR_DrawDemoControls ();
		SCR_DrawSpeed ();
		SCR_DrawEdictInfo ();
		SCR_DrawConsole ();
		M_Draw ();
		SCR_DrawFPS (); //johnfitz
		SCR_DrawSaving ();
	}

	Draw_Flush ();

	GL_EndGroup ();

	GL_EndRendering ();
}

