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

#ifndef _QUAKE_SCREEN_H
#define _QUAKE_SCREEN_H

// screen.h

void SCR_Init (void);
void SCR_LoadPics (void);

void SCR_UpdateScreen (void);

void SCR_UpdateZoom (void);
void SCR_CenterPrint (const char *str);

void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);

int SCR_ModalMessage (const char *text, float timeout); //johnfitz -- added timeout

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int			sb_lines;

extern	int			clearnotify;	// set to 0 whenever notify text is drawn
extern	qboolean	scr_disabled_for_loading;
extern	qboolean	scr_skipupdate;

extern	cvar_t		scr_viewsize;

extern	cvar_t		scr_sbaralpha; //johnfitz

//johnfitz -- stuff for 2d drawing control
typedef enum {
	CANVAS_NONE,
	CANVAS_DEFAULT,
	CANVAS_CONSOLE,
	CANVAS_MENU,
	CANVAS_SBAR,
	CANVAS_SBAR_QW_INV,
	CANVAS_SBAR2,
	CANVAS_CROSSHAIR,
	CANVAS_BOTTOMLEFT,
	CANVAS_BOTTOMRIGHT,
	CANVAS_TOPRIGHT,
	CANVAS_CSQC,
	CANVAS_INVALID = -1
} canvastype;

typedef struct drawtransform_s
{
	float				scale[2];
	float				offset[2];
} drawtransform_t;

typedef struct glcanvas_s {
	canvastype			type;
	float				left, right, bottom, top;
	drawtransform_t		transform;
	uint32_t			colorstack[8];
	int					colorstacktop;
	unsigned			blendmode;
	struct gltexture_s	*texture;
} glcanvas_t;

extern	glcanvas_t	glcanvas;

typedef enum menubg_t
{
	MENUBG_GLQUAKE,
	MENUBG_WINQUAKE,
	MENUBG_WINQUAKE_SCALED,
	MENUBG_DOSQUAKE,

	MENUBG_NUMTYPES,
} menubgstyle_t;

extern	cvar_t		scr_menuscale;
extern	cvar_t		scr_menubgalpha;
extern	cvar_t		scr_menubgstyle;
extern	cvar_t		scr_centerprintbg;
extern	cvar_t		scr_sbarscale;
extern	cvar_t		scr_conwidth;
extern	cvar_t		scr_conscale;
extern	cvar_t		scr_conalpha;
extern	cvar_t		scr_conbrightness;
extern	cvar_t		scr_scale;
extern	cvar_t		scr_crosshairscale;
//johnfitz

typedef enum hudstyle_t
{
	HUD_CLASSIC,
	HUD_MODERN_CENTERAMMO,		// Modern 1
	HUD_MODERN_SIDEAMMO,		// Modern 2
	HUD_QUAKEWORLD,

	HUD_COUNT,
} hudstyle_t;

extern	cvar_t		scr_hudstyle;
extern	hudstyle_t	hudstyle;

extern int scr_tileclear_updates; //johnfitz

#endif	/* _QUAKE_SCREEN_H */

