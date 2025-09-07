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

#ifndef _QUAKE_DRAW_H
#define _QUAKE_DRAW_H

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

extern	qpic_t		*draw_disc;	// also used on sbar
extern	qboolean	custom_conchars;

extern	const vec3_t	rgb_black;
extern	const vec3_t	rgb_white;

#define CHARSIZE	8

void Draw_Init (void);
void Draw_Character (int x, int y, int num);
void Draw_CharacterEx (float x, float y, float dimx, float dimy, int num);
void Draw_Pic (int x, int y, qpic_t *pic);
void Draw_SubPic (float x, float y, float w, float h, qpic_t *pic, float s1, float t1, float s2, float t2, const float *rgb, float alpha);
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom); //johnfitz -- more parameters
void Draw_ConsoleBackground (void); //johnfitz -- removed parameter int lines
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c, float alpha); //johnfitz -- added alpha
void Draw_FillEx (float x, float y, float w, float h, const float *rgb, float alpha);
void Draw_PartialFadeScreen (float x0, float x1, float y0, float y1, float alpha);
void Draw_FadeScreen (float alpha);
void Draw_String (int x, int y, const char *str);
void Draw_StringEx (float x, float y, float dim, const char *str);
qpic_t *Draw_PicFromWad2 (const char *name, unsigned int texflags);
qpic_t *Draw_PicFromWad (const char *name);
qpic_t *Draw_CachePic (const char *path);
qpic_t *Draw_TryCachePic (const char *path, unsigned int texflags);
void Draw_Flush (void);
void Draw_NewGame (void);

void Draw_SetClipRect (float x, float y, float width, float height);
void Draw_ResetClipping (void);

void Draw_GetCanvasTransform (canvastype canvas, drawtransform_t *transform);
void Draw_GetTransformBounds (const drawtransform_t *transform, float *left, float *top, float *right, float *bottom);

void GL_SetCanvas (canvastype newcanvas); //johnfitz
void GL_SetCanvasColor (float r, float g, float b, float a);
void GL_PushCanvasColor (float r, float g, float b, float a);
void GL_PopCanvasColor (void);

#endif	/* _QUAKE_DRAW_H */

