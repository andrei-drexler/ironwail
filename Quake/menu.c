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

#include "quakedef.h"
#include "bgmusic.h"
#include "q_ctype.h"

#include <time.h>

cvar_t ui_mouse	= {"ui_mouse", "1", CVAR_ARCHIVE};
cvar_t ui_live_preview = {"ui_live_preview", "1", CVAR_ARCHIVE};
cvar_t ui_mouse_sound = {"ui_mouse_sound", "0", CVAR_ARCHIVE};
cvar_t ui_sound_throttle = {"ui_sound_throttle", "0.1", CVAR_ARCHIVE};
cvar_t ui_search_timeout = {"ui_search_timeout", "1", CVAR_ARCHIVE};

extern cvar_t crosshair;
extern cvar_t language;
extern cvar_t scr_fov;
extern cvar_t cl_gun_fovscale;
extern cvar_t v_gunkick;
extern cvar_t cl_bob;
extern cvar_t cl_rollangle;
extern cvar_t cl_maxpitch;
extern cvar_t cl_minpitch;
extern cvar_t gl_cshiftpercent;
extern cvar_t sv_autoload;
extern cvar_t r_particles;
extern cvar_t gl_texturemode;
extern cvar_t gl_texture_anisotropy;
extern cvar_t host_maxfps;
extern cvar_t scr_showfps;
extern cvar_t scr_showspeed;
extern cvar_t scr_clock;
extern cvar_t vid_width;
extern cvar_t vid_height;
extern cvar_t vid_refreshrate;
extern cvar_t vid_fullscreen;
extern cvar_t vid_borderless;
extern cvar_t vid_vsync;
extern cvar_t vid_fsaamode;
extern cvar_t vid_fsaa;
extern cvar_t r_softemu;
extern cvar_t r_softemu_mdl_warp;
extern cvar_t r_waterwarp;
extern cvar_t r_oit;
extern cvar_t r_alphasort;
extern cvar_t r_enhancedmodels;
extern cvar_t r_lerpmodels;
extern cvar_t r_lerpmove;
extern cvar_t snd_waterfx;
extern cvar_t joy_deadzone_look;
extern cvar_t joy_deadzone_move;
extern cvar_t joy_deadzone_trigger;
extern cvar_t joy_sensitivity_yaw;
extern cvar_t joy_sensitivity_pitch;
extern cvar_t joy_invert;
extern cvar_t joy_exponent;
extern cvar_t joy_exponent_move;
extern cvar_t joy_swapmovelook;
extern cvar_t joy_flick;
extern cvar_t joy_rumble;
extern cvar_t gyro_enable;
extern cvar_t gyro_mode;
extern cvar_t gyro_turning_axis;
extern cvar_t gyro_pitchsensitivity;
extern cvar_t gyro_yawsensitivity;
extern cvar_t gyro_noise_thresh;

extern char crosshair_char;

extern qboolean quake64;

enum m_state_e m_state;
extern qboolean	keydown[256];
float m_mousex, m_mousey;
int m_lastkey = -1; // last key pressed
qboolean m_ignoremouseframe;
static int m_left, m_top, m_width, m_height;

static void M_UpdateBounds (void);

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
		void M_Menu_Maps_f (void);
		void M_Menu_Skill_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_Net_f (void);
		void M_Menu_LanConfig_f (void);
		void M_Menu_GameOptions_f (void);
		void M_Menu_Search_f (void);
		void M_Menu_ServerList_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Video_f (void);
		void M_Menu_Gamepad_f (void);
	void M_Menu_Mods_f (void);
		void M_Menu_ModInfo_f (const filelist_item_t *item);
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
		void M_Maps_Draw (void);
		void M_Skill_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
		void M_Net_Draw (void);
		void M_LanConfig_Draw (void);
		void M_GameOptions_Draw (void);
		void M_Search_Draw (void);
		void M_ServerList_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		//void M_Video_Draw (void);
		//void M_Gamepad_Draw (void);
	void M_Mods_Draw (void);
		void M_ModInfo_Draw (void);
	void M_Help_Draw (void);
	void M_Quit_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
		void M_Maps_Key (int key);
		void M_Skill_Key (int key);
	void M_MultiPlayer_Key (int key);
		void M_Setup_Key (int key);
		void M_Net_Key (int key);
		void M_LanConfig_Key (int key);
		void M_GameOptions_Key (int key);
		void M_Search_Key (int key);
		void M_ServerList_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
		//void M_Video_Key (int key);
		//void M_Gamepad_Key (int key);
	void M_Mods_Key (int key);
		void M_ModInfo_Key (int key);
	void M_Help_Key (int key);
	void M_Quit_Key (int key);

void M_Main_Mousemove (float cx, float cy);
	void M_SinglePlayer_Mousemove (float cx, float cy);
		void M_Load_Mousemove (float cx, float cy);
		void M_Save_Mousemove (float cx, float cy);
		void M_Maps_Mousemove (float cx, float cy);
		void M_Skill_Mousemove (float cx, float cy);
	void M_MultiPlayer_Mousemove (float cx, float cy);
		void M_Setup_Mousemove (float cx, float cy);
		void M_Net_Mousemove (float cx, float cy);
		void M_LanConfig_Mousemove (float cx, float cy);
		void M_GameOptions_Mousemove (float cx, float cy);
		//void M_Search_Mousemove (float cx, float cy);
		void M_ServerList_Mousemove (float cx, float cy);
	void M_Options_Mousemove (float cx, float cy);
		void M_Keys_Mousemove (float cx, float cy);
		//void M_Video_Mousemove (float cx, float cy);
		//void M_Gamepad_Mousemove (float cx, float cy);
	void M_Mods_Mousemove (float cx, float cy);
	//void M_Help_Mousemove (float cx, float cy);
	//void M_Quit_Mousemove (float cx, float cy);

static double m_lastsoundtime;
static char m_lastsound[MAX_QPATH];

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

enum m_state_e	m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define	IPXConfig		(m_net_cursor == 0)
#define	TCPIPConfig		(m_net_cursor == 1)

void M_ConfigureNetSubsystem(void);
void M_SetSkillMenuMap (const char *name);
void M_Options_SelectMods (void);
void M_Options_Init (enum m_state_e state);
void M_ChooseQuitMessage (void);
void M_DrawQuitMessage (void);
qboolean M_ForcedQuitMessage (float *alpha);

#define PREVIEW_FADEIN_TIME				0.125
#define PREVIEW_FADEOUT_TIME			0.125
#define PREVIEW_HOLD_TIME				1.25
#define PREVIEW_LONG_HOLD_TIME			2.25

#define SEARCH_FADE_TIMEOUT				0.5
#define SEARCH_TYPE_TIMEOUT				1.5
#define SEARCH_ERASE_TIMEOUT			1.5
#define SEARCH_NAV_TIMEOUT				2.0
#define SEARCH_ERROR_STATUS_TIMEOUT		0.25
#define SEARCH_BACKSPACE_COOLDOWN		0.75

static void M_ThrottledSound (const char *sound)
{
	if (strcmp (m_lastsound, sound) == 0 && realtime - m_lastsoundtime < ui_sound_throttle.value)
		return;
	q_strlcpy (m_lastsound, sound, sizeof (m_lastsound));
	m_lastsoundtime = realtime;
	S_LocalSound (sound);
}

static void M_MouseSound (const char *sound)
{
	if (!ui_mouse_sound.value)
		return;
	M_ThrottledSound (sound);
}

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character (cx, line, num);
}

void M_DrawArrowCursor (int cx, int cy)
{
	M_DrawCharacter (cx, cy, 12+((int)(realtime*4)&1));
}

void M_DrawTextCursor (int cx, int cy)
{
	M_DrawCharacter (cx, cy, 10+((int)(realtime*4)&1));
}

void M_PrintEx (int cx, int cy, int dim, const char *str)
{
	while (*str)
	{
		Draw_CharacterEx (cx, cy, dim, dim, (*str)+128);
		str++;
		cx += dim;
	}
}

void M_Print (int cx, int cy, const char *str)
{
	M_PrintEx (cx, cy, 8, str);
}

void M_PrintWhiteEx (int cx, int cy, int dim, const char *str)
{
	while (*str)
	{
		Draw_CharacterEx (cx, cy, dim, dim, *str);
		str++;
		cx += dim;
	}
}

void M_PrintWhite (int cx, int cy, const char *str)
{
	M_PrintWhiteEx (cx, cy, 8, str);
}

#define ALIGN_LEFT		0
#define ALIGN_CENTER	1
#define ALIGN_RIGHT		2

void M_PrintAlignedEx (int cx, int cy, int align, int dim, qboolean color, const char *str)
{
	cx -= strlen (str) * align / 2 * dim;
	if (color)
		M_PrintEx (cx, cy, dim, str);
	else
		M_PrintWhiteEx (cx, cy, dim, str);
}

void M_PrintAligned (int cx, int cy, int align, const char *str)
{
	M_PrintAlignedEx (cx, cy, align, 8, true, str);
}

// TODO: smooth scrolling
void M_PrintScroll (int x, int y, int maxwidth, const char *str, double time, qboolean color)
{
	int maxchars = maxwidth / 8;
	int len = strlen (str);
	int i, ofs;
	char mask = color ? QCHAR_COLOR_MASK : 0;

	if (len <= maxchars)
	{
		if (color)
			M_Print (x, y, str);
		else
			M_PrintWhite (x, y, str);
		return;
	}

	ofs = (int) floor (time * 4.0);
	ofs %= len + 5;
	if (ofs < 0)
		ofs += len + 5;

	for (i = 0; i < maxchars; i++)
	{
		char c = (ofs < len) ? str[ofs] : " /// "[ofs - len];
		M_DrawCharacter (x, y, c ^ mask);
		x += 8;
		if (++ofs >= len + 5)
			ofs = 0;
	}
}

static void M_PrintSubstring (int x, int y, const char *text, int numchars, qboolean color)
{
	char mask = color ? 0x80 : 0;
	while (*text && numchars)
	{
		M_DrawCharacter (x, y, *text++ ^ mask);
		x += 8;
		--numchars;
	}
}

static void M_PrintDotFill (int x, int y, const char *text, int cols, qboolean color)
{
	char mask = color ? 0x80 : 0;
	while (*text && cols >= 2)
	{
		M_DrawCharacter (x, y, *text++ ^ mask);
		x += 8;
		--cols;
	}

	GL_SetCanvasColor (1.f, 1.f, 1.f, 0.375f);
	while (cols --> 0)
	{
		M_DrawCharacter (x, y, '.' ^ mask);
		x += 8;
	}
	GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);
}

static int M_WordLength (const char *text)
{
	const char *start = text;
	while (*text && !q_isblank (*text))
		text++;
	return text - start;
}

static int M_LineWrap (const char **text, int maxchars)
{
	const char *str = *text;
	int i;

	for (i = 0; i < maxchars && str[i]; /**/)
	{
		if (str[i] == '\n')
		{
			*text += i + 1;
			return i;
		}

		// new word
		if (!q_isblank (str[i]) && (i == 0 || q_isblank (str[i - 1])))
		{
			int len = M_WordLength (str + i);
			// split word if longer than given limit
			if (len > maxchars)
			{
				*text += maxchars;
				return maxchars;
			}
			// not enough space left? push word to next line
			if (i + len > maxchars)
			{
				*text += i;
				return i;
			}
			// word fits, continue
			i += len;
		}
		else
			i++;
	}

	// avoid starting next line with a space
	*text += i + (q_isblank (str[i]) ? 1 : 0);

	return i;
}

int M_PrintWordWrap (int x, int y, const char *text, int width, int height, qboolean color)
{
	int maxcols = width / 8;
	int maxlines = height / 8;
	int numlines = 0;

	while (*text && numlines < maxlines)
	{
		const char *line = text;
		int len = M_LineWrap (&text, maxcols);
		M_PrintSubstring (x, y + numlines * 8, line, len, color);
		numlines++;
	}

	return numlines;
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y, pic); //johnfitz -- simplified becuase centering is handled elsewhere
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y, pic); //johnfitz -- simplified becuase centering is handled elsewhere
}

void M_DrawSubpic (int x, int y, qpic_t *pic, int left, int top, int width, int height)
{
	float s1 = left   / (float)pic->width;
	float t1 = top    / (float)pic->height;
	float s2 = width  / (float)pic->width;
	float t2 = height / (float)pic->height;
	Draw_SubPic (x, y, width, height, pic, s1, t1, s2, t2, NULL, 1.f);
}

void M_DrawTransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom) //johnfitz -- more parameters
{
	Draw_TransPicTranslate (x, y, pic, top, bottom); //johnfitz -- simplified becuase centering is handled elsewhere
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

void M_DrawQuakeCursor (int cx, int cy)
{
	qpic_t *pic = Draw_CachePic( va("gfx/menudot%i.lmp", (int)(realtime*10)%6+1 ) );
	M_DrawTransPic (cx, cy, pic);
}

void M_DrawQuakeBar (int x, int y, int cols)
{
	M_DrawCharacter (x, y, '\35');
	x += 8;
	cols -= 2;
	while (cols-- > 0)
	{
		M_DrawCharacter (x, y, '\36');
		x += 8;
	}
	M_DrawCharacter (x, y, '\37');
}

void M_DrawEllipsisBar (int x, int y, int cols)
{
	while (cols > 0)
	{
		M_DrawCharacter (x, y, '.' | 128);
		cols -= 2;
		x += 16;
	}
}

//=============================================================================
/* Mouse helpers */

qboolean M_IsMouseKey (int key)
{
	switch (key)
	{
	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
	case K_MOUSE4:
	case K_MOUSE5:
	case K_MWHEELUP:
	case K_MWHEELDOWN:
		return true;
	default:
		return false;
	}
}

void M_ForceMousemove (void)
{
	int x, y;
	SDL_GetMouseState (&x, &y);
	M_Mousemove (x, y);
}

void M_UpdateCursor (int mousey, int starty, int itemheight, int numitems, int *cursor)
{
	int pos = (mousey - starty) / itemheight;
	if (pos > numitems - 1)
		pos = numitems - 1;
	if (pos < 0)
		pos = 0;
	*cursor = pos;
}

void M_UpdateCursorWithTable (int mousey, const int *table, int numitems, int *cursor)
{
	int i, dy;
	for (i = 0; i < numitems; i++)
	{
		dy = mousey - table[i];
		if (dy >= 0 && dy < 8)
		{
			*cursor = i;
			break;
		}
	}
}

//=============================================================================
/* Listbox */

typedef struct
{
	int				len;
	int				maxlen;
	qboolean		(*match_fn) (int index);
	double			timeout;
	double			errtimeout;
	double			backspacecooldown;
	char			text[256];
} listsearch_t;

typedef struct
{
	int				cursor;
	int				numitems;
	int				viewsize;
	int				scroll;
	qboolean		(*isactive_fn) (int index);
	listsearch_t	search;
} menulist_t;

void M_List_CheckIntegrity (const menulist_t *list)
{
	SDL_assert (list->numitems >= 0);
	SDL_assert (list->cursor >= 0);
	SDL_assert (list->cursor < list->numitems);
	SDL_assert (list->scroll >= 0);
	SDL_assert (list->scroll < list->numitems);
	SDL_assert (list->viewsize > 0);
}

void M_List_ClearSearch (menulist_t *list)
{
	list->search.timeout = 0.0;
	list->search.errtimeout = 0.0;
	list->search.len = 0;
	list->search.text[0] = '\0';
}

void M_List_KeepSearchVisible (menulist_t *list, double duration)
{
	if (!list->search.len)
		return;
	if (duration <= 0.0)
		duration = SEARCH_TYPE_TIMEOUT;
	list->search.timeout = q_max (list->search.timeout, duration);
}

void M_List_AutoScroll (menulist_t *list)
{
	if (list->numitems <= list->viewsize)
		return;
	if (list->cursor < list->scroll)
	{
		list->scroll = list->cursor;
		if (list->isactive_fn)
		{
			while (list->scroll > 0 &&
				list->scroll > list->cursor - list->viewsize + 1 &&
				!list->isactive_fn (list->scroll - 1))
			{
				--list->scroll;
			}
		}
	}
	else if (list->cursor >= list->scroll + list->viewsize)
		list->scroll = list->cursor - list->viewsize + 1;
}

void M_List_CenterCursor (menulist_t *list)
{
	if (list->cursor >= list->viewsize)
	{
		if (list->cursor + list->viewsize >= list->numitems)
			list->scroll = list->numitems - list->viewsize; // last page, scroll to the end
		else
			list->scroll = list->cursor - list->viewsize / 2; // keep centered
		list->scroll = CLAMP (0, list->scroll, list->numitems - list->viewsize);
	}
	else
		list->scroll = 0;
}

int M_List_GetOverflow (const menulist_t *list)
{
	return list->numitems - list->viewsize;
}

// Note: y is in pixels, height is in chars!
qboolean M_List_GetScrollbar (const menulist_t *list, int *y, int *height)
{
	if (list->numitems <= list->viewsize)
	{
		*y = *height = 0;
		return false;
	}

	*height = (int)(list->viewsize * list->viewsize / (float)list->numitems + 0.5f);
	*height = q_max (*height, 2);
	*y = (int)(list->scroll * 8 / (float)(list->numitems - list->viewsize) * (list->viewsize - *height) + 0.5f);

	return true;
}

void M_List_DrawScrollbar (const menulist_t *list, int cx, int cy)
{
	int y, h;
	if (!M_List_GetScrollbar (list, &y, &h))
		return;
	M_DrawTextBox (cx - 4, cy + y - 4, 0, h - 1);
}

void M_List_DrawSearch (const menulist_t *list, int cx, int cy, int maxlen)
{
	int i, ofs;
	float alpha;

	if (!list->search.len)
		return;

	ofs = q_max (0, list->search.len + 1 - maxlen);
	M_DrawTextBox (cx - 8, cy - 8, maxlen, 1);
	for (i = ofs; i < list->search.len; i++)
		M_DrawCharacter (cx + (i-ofs)*8, cy, list->search.text[i]);

	alpha = CLAMP (0.f, (float) list->search.timeout / SEARCH_FADE_TIMEOUT, 1.f);
	GL_SetCanvasColor (1.f, 1.f, 1.f, alpha);
	M_DrawCharacter (cx + (i-ofs)*8, cy, list->search.errtimeout ? 11^128 : 11);
	GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);
}

qboolean M_List_UseScrollbar (menulist_t *list, int yrel)
{
	int scrolly, scrollh, range;
	if (!M_List_GetScrollbar (list, &scrolly, &scrollh))
		return false;

	yrel -= scrollh * 4; // half the thumb height, in pixels
	range = (list->viewsize - scrollh) * 8;
	list->scroll = (int)(yrel * (float)(list->numitems - list->viewsize) / range + 0.5f);

	if (list->scroll > list->numitems - list->viewsize)
		list->scroll = list->numitems - list->viewsize;
	if (list->scroll < 0)
		list->scroll = 0;

	return true;
}

void M_List_GetVisibleRange (const menulist_t *list, int *first, int *count)
{
	*first = list->scroll;
	*count = q_min (list->scroll + list->viewsize, list->numitems) - list->scroll;
}

qboolean M_List_IsItemVisible (const menulist_t *list, int i)
{
	int first, count;
	M_List_GetVisibleRange (list, &first, &count);
	return (unsigned)(i - first) < (unsigned)count;
}

void M_List_Rescroll (menulist_t *list)
{
	int overflow = M_List_GetOverflow (list);
	if (overflow < 0)
		overflow = 0;
	if (list->scroll > overflow)
		list->scroll = overflow;
	if (list->cursor >= 0 && list->cursor < list->numitems && !M_List_IsItemVisible (list, list->cursor))
		M_List_AutoScroll (list);
}

qboolean M_List_SelectNextMatch (menulist_t *list, qboolean (*match_fn) (int idx), int start, int dir, qboolean wrap)
{
	int i, j;

	if (list->numitems <= 0)
		return false;

	if (!wrap)
		start = CLAMP (0, start, list->numitems - 1);

	for (i = 0, j = start; i < list->numitems; i++, j+=dir)
	{
		if (j < 0)
		{
			if (!wrap)
				return false;
			j = list->numitems - 1;
		}
		else if (j >= list->numitems)
		{
			if (!wrap)
				return false;
			j = 0;
		}
		if (list->isactive_fn && !list->isactive_fn (j))
			continue;
		if (!match_fn || match_fn (j))
		{
			list->cursor = j;
			M_List_AutoScroll (list);
			return true;
		}
	}

	return false;
}

qboolean M_List_SelectNextSearchMatch (menulist_t *list, int start, int dir)
{
	if (!list->search.match_fn)
		return false;
	return M_List_SelectNextMatch (list, list->search.match_fn, start, dir, true);
}

qboolean M_List_SelectNextActive (menulist_t *list, int start, int dir, qboolean wrap)
{
	return M_List_SelectNextMatch (list, NULL, start, dir, wrap);
}

void M_List_UpdateMouseSelection (menulist_t *list)
{
	M_ForceMousemove ();
	if (list->cursor < list->scroll)
		M_List_SelectNextActive (list, list->scroll, 1, false);
	else if (list->cursor >= list->scroll + list->viewsize)
		M_List_SelectNextActive (list, list->scroll + list->viewsize, -1, false);
}

void M_List_Update (menulist_t *list)
{
	list->search.errtimeout = q_max (0.0, list->search.errtimeout - host_rawframetime);
	list->search.backspacecooldown = q_max (0.0, list->search.backspacecooldown - host_rawframetime);
	if (list->search.timeout && ui_search_timeout.value > 0.f)
	{
		list->search.timeout -= host_rawframetime / ui_search_timeout.value;
		if (list->search.timeout <= 0.0)
			M_List_ClearSearch (list);
	}
}

qboolean M_List_Key (menulist_t *list, int key)
{
	qboolean overflow = M_List_GetOverflow (list) > 0;

	switch (key)
	{
	case K_BACKSPACE:
		if (list->search.len)
		{
			if (keydown[K_CTRL])
				M_List_ClearSearch (list);
			else
			{
				list->search.len--;
				list->search.text[list->search.len] = '\0';
				list->search.timeout = SEARCH_ERASE_TIMEOUT;
			}
			list->search.backspacecooldown = SEARCH_BACKSPACE_COOLDOWN;
			return true;
		}
		if (list->search.backspacecooldown)
			list->search.backspacecooldown = SEARCH_BACKSPACE_COOLDOWN;
		return false;

	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		if (list->search.len)
		{
			M_List_ClearSearch (list);
			return true;
		}
		return false;

	case K_HOME:
	case K_KP_HOME:
		M_ThrottledSound ("misc/menu1.wav");
		if (list->search.len)
		{
			M_List_SelectNextSearchMatch (list, 0, 1);
			list->search.timeout = SEARCH_NAV_TIMEOUT;
		}
		else
		{
			M_List_SelectNextActive (list, 0, 1, false);
			list->scroll = 0;
			M_List_AutoScroll (list);
		}
		return true;

	case K_END:
	case K_KP_END:
		M_ThrottledSound ("misc/menu1.wav");
		if (list->search.len)
		{
			M_List_SelectNextSearchMatch (list, list->numitems - 1, -1);
			list->search.timeout = SEARCH_NAV_TIMEOUT;
		}
		else
		{
			M_List_SelectNextActive (list, list->numitems - 1, -1, false);
		}
		return true;

	case K_PGDN:
	case K_KP_PGDN:
		M_ThrottledSound ("misc/menu1.wav");
		if (list->search.len)
		{
			M_List_SelectNextSearchMatch (list, list->cursor + 1, 1);
			list->search.timeout = SEARCH_NAV_TIMEOUT;
		}
		else
		{
			qboolean sel;
			if (list->cursor - list->scroll < list->viewsize - 1)
				sel = M_List_SelectNextActive (list, list->scroll + list->viewsize - 1, 1, false);
			else
				sel = M_List_SelectNextActive (list, list->cursor + list->viewsize - 1, 1, false);
			if (!sel)
				M_List_SelectNextActive (list, list->numitems - 1, -1, false);
		}
		return true;

	case K_PGUP:
	case K_KP_PGUP:
		M_ThrottledSound ("misc/menu1.wav");
		if (list->search.len)
		{
			M_List_SelectNextSearchMatch (list, list->cursor - 1, -1);
			list->search.timeout = SEARCH_NAV_TIMEOUT;
		}
		else
		{
			qboolean sel;
			if (list->cursor > list->scroll)
				sel = M_List_SelectNextActive (list, list->scroll, -1, false);
			else
				sel = M_List_SelectNextActive (list, list->cursor - list->viewsize + 1, -1, false);
			if (!sel)
				M_List_SelectNextActive (list, 0, 1, false);
		}
		return true;

	case K_UPARROW:
	case K_KP_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (list->search.len)
		{
			M_List_SelectNextSearchMatch (list, list->cursor - 1, -1);
			list->search.timeout = SEARCH_NAV_TIMEOUT;
		}
		else
		{
			M_List_SelectNextActive (list, list->cursor - 1, -1, true);
		}
		return true;

	case K_MWHEELUP:
		if (!overflow)
			return false;
		list->scroll -= 3;
		if (list->scroll < 0)
			list->scroll = 0;
		M_List_UpdateMouseSelection (list);
		return true;

	case K_MWHEELDOWN:
		if (!overflow)
			return false;
		list->scroll += 3;
		if (list->scroll > list->numitems - list->viewsize)
			list->scroll = list->numitems - list->viewsize;
		if (list->scroll < 0)
			list->scroll = 0;
		M_List_UpdateMouseSelection (list);
		return true;

	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (list->search.len)
		{
			M_List_SelectNextSearchMatch (list, list->cursor + 1, 1);
			list->search.timeout = SEARCH_NAV_TIMEOUT;
		}
		else
		{
			M_List_SelectNextActive (list, list->cursor + 1, 1, true);
		}
		return true;

	default:
		return false;
	}
}

void M_List_Char (menulist_t *list, int key)
{
	int maxlen, start;

	if (list->numitems <= 0 || !list->search.match_fn)
		return;

	// don't allow starting with a space
	if (list->search.len <= 0 && key == ' ')
		return;

	maxlen = (int) countof (list->search.text) - 1;
	if (list->search.maxlen)
		maxlen = q_min (maxlen, list->search.maxlen);

	if (list->search.len >= maxlen)
	{
		list->search.timeout = SEARCH_ERASE_TIMEOUT;
		list->search.errtimeout = SEARCH_ERROR_STATUS_TIMEOUT;
		M_ThrottledSound ("misc/menu2.wav");
		return;
	}

	list->search.text[list->search.len++] = (char) key;
	list->search.text[list->search.len] = '\0';
	list->search.timeout = SEARCH_TYPE_TIMEOUT;

	if (list->cursor < 0)
		list->cursor = 0;

	start = list->cursor;
	if (list->search.len == 1)
		start++;

	if (!M_List_SelectNextSearchMatch (list, start, 1))
	{
		list->search.len--;
		list->search.text[list->search.len] = '\0';
		list->search.timeout = SEARCH_ERASE_TIMEOUT;
		list->search.errtimeout = SEARCH_ERROR_STATUS_TIMEOUT;
		M_ThrottledSound ("misc/menu2.wav");
	}
}

void M_List_Mousemove (menulist_t *list, int yrel)
{
	int i, firstvis, numvis;

	M_List_GetVisibleRange (list, &firstvis, &numvis);
	if (!numvis || yrel < 0)
		return;
	i = yrel / 8;
	if (i >= numvis)
		return;

	i += firstvis;
	if (list->cursor == i)
		return;

	if (list->isactive_fn && !list->isactive_fn (i))
	{
		int before, after;
		yrel += firstvis * 8;

		for (before = i - 1; before >= firstvis; before--)
			if (list->isactive_fn (before))
				break;
		for (after = i + 1; after < firstvis + numvis; after++)
			if (list->isactive_fn (after))
				break;

		if (before >= firstvis && after < firstvis + numvis)
		{
			int distbefore = yrel - 4 - before * 8;
			int distafter = after * 8 + 4 - yrel;
			i = distbefore < distafter ? before : after;
		}
		else if (before >= firstvis)
			i = before;
		else if (after < firstvis + numvis)
			i = after;
		else
			return;

		if (list->cursor == i)
			return;
	}

	list->cursor = i;

	M_MouseSound ("misc/menu1.wav");
}


//=============================================================================
/* Scrolling ticker */

typedef struct
{
	double			scroll_time;
	double			scroll_wait_time;
} menuticker_t;

static void M_Ticker_Init (menuticker_t *ticker)
{
	ticker->scroll_time = 0.0;
	ticker->scroll_wait_time = 1.0;
}

static void M_Ticker_Update (menuticker_t *ticker)
{
	if (ticker->scroll_wait_time <= 0.0)
		ticker->scroll_time += host_rawframetime;
	else
		ticker->scroll_wait_time = q_max (0.0, ticker->scroll_wait_time - host_rawframetime);
}

static qboolean M_Ticker_Key (menuticker_t *ticker, int key)
{
	switch (key)
	{
	case K_RIGHTARROW:
		ticker->scroll_time += 0.25;
		ticker->scroll_wait_time = 1.5;
		M_ThrottledSound ("misc/menu3.wav");
		return true;

	case K_LEFTARROW:
		ticker->scroll_time -= 0.25;
		ticker->scroll_wait_time = 1.5;
		M_ThrottledSound ("misc/menu3.wav");
		return true;

	default:
		return false;
	}
}

//=============================================================================

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f ();
			return;
		}

		IN_Activate();
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		M_Menu_Main_f ();
	}
}

/*
================
M_GetBaseState
================
*/
static enum m_state_e M_GetBaseState (enum m_state_e state)
{
	switch (state)
	{
	case m_video:
	case m_graphics:
	case m_gamepad:
	case m_interface:
	case m_game:
		return m_options;
	default:
		return state;
	}
}

//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
int m_main_mods;

enum
{
	MAIN_SINGLEPLAYER,
	MAIN_MULTIPLAYER,
	MAIN_OPTIONS,
	MAIN_MODS,
	MAIN_HELP,
	MAIN_QUIT,

	MAIN_ITEMS,
};

void M_Menu_Main_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;

	// When switching to a mod with a custom UI the 'Mods' option
	// is no longer available in the main menu, so we move the cursor
	// to 'Options' to nudge the player toward the secondary location.
	// TODO (maybe): inform the user about the missing option
	// and its alternative location?
	if (!m_main_mods && m_main_cursor == MAIN_MODS)
	{
		m_main_cursor = MAIN_OPTIONS;
		M_Options_SelectMods ();
	}
}


void M_Main_Draw (void)
{
	int		cursor;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_main.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	p = Draw_CachePic ("gfx/mainmenu.lmp");
	if (m_main_mods)
	{
		int split = 60;
		M_DrawSubpic (72, 32, p, 0, 0, p->width, split);
		if (m_main_mods > 0)
			M_DrawTransPic (72, 32 + split, Draw_CachePic ("gfx/menumods.lmp"));
		else
			M_PrintEx (74, 32 + split + 1, 16, "MODS");
		M_DrawSubpic (72, 32 + split + 20, p, 0, split, p->width, p->height - split);
	}
	else
		M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mainmenu.lmp"));

	cursor = m_main_cursor;
	if (!m_main_mods && cursor > MAIN_MODS)
		--cursor;
	M_DrawQuakeCursor (54, 32 + cursor * 20);
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		IN_Activate();
		key_dest = key_game;
		m_state = m_none;
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		else if (!m_main_mods && m_main_cursor == MAIN_MODS)
			++m_main_cursor;
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		else if (!m_main_mods && m_main_cursor == MAIN_MODS)
			--m_main_cursor;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case MAIN_SINGLEPLAYER:
			M_Menu_SinglePlayer_f ();
			break;

		case MAIN_MULTIPLAYER:
			M_Menu_MultiPlayer_f ();
			break;

		case MAIN_OPTIONS:
			M_Menu_Options_f ();
			break;

		case MAIN_HELP:
			M_Menu_Help_f ();
			break;

		case MAIN_MODS:
			M_Menu_Mods_f ();
			break;

		case MAIN_QUIT:
			M_Menu_Quit_f ();
			break;
		}
	}
}

void M_Main_Mousemove (float cx, float cy)
{
	int prev = m_main_cursor;
	M_UpdateCursor (cy, 32, 20, MAIN_ITEMS - !m_main_mods, &m_main_cursor);
	if (m_main_cursor >= MAIN_MODS && !m_main_mods)
		++m_main_cursor;
	if (m_main_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* SINGLE PLAYER MENU */

qboolean m_singleplayer_showlevels;
int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	(3 + m_singleplayer_showlevels)

void M_Menu_SinglePlayer_f (void)
{
	IN_DeactivateForMenu();
	if (m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
		m_singleplayer_cursor = 0;
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw (void)
{
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_sgl.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/sp_menu.lmp") );
	if (m_singleplayer_showlevels)
		M_DrawTransPic (72, 92, Draw_CachePic ("gfx/sp_maps.lmp") );

	M_DrawQuakeCursor (54, 32 + m_singleplayer_cursor * 20);
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game? (y/n)\n", 0.0f))
					break;
			if (quake64)
			{
				M_SetSkillMenuMap ("start");
				M_Menu_Skill_f ();
				break;
			}
			IN_Activate();
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("deathmatch 0\n"); //johnfitz
			Cbuf_AddText ("coop 0\n"); //johnfitz
			Cbuf_AddText ("campaign 1\n");
			Cbuf_AddText ("map start\n");
			break;

		case 1:
			Cbuf_AddText ("menu_load\n");
			break;

		case 2:
			Cbuf_AddText ("menu_save\n");
			break;

		case 3:
			Cbuf_AddText ("menu_maps\n");
			break;
		}
	}
}


void M_SinglePlayer_Mousemove (float cx, float cy)
{
	int prev = m_singleplayer_cursor;
	M_UpdateCursor (cy, 32, 20, SINGLEPLAYER_ITEMS, &m_singleplayer_cursor);
	if (m_singleplayer_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		20	/* johnfitz -- increased from 12 */
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int	i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int	version;

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		q_snprintf (name, sizeof(name), "%s/s%i.sav", com_gamedir, i);
		f = Sys_fopen (name, "r");
		if (!f) {
			continue;
		}
		if (fscanf(f, "%i\n", &version) != 1 ||
		    fscanf(f, "%79s\n", name)   != 1) {
			fclose(f);
			continue;
		}
		q_strlcpy (m_filenames[i], name, SAVEGAME_COMMENT_LENGTH+1);

	// change _ back to space
		for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
		{
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		}
		loadable[i] = true;
		fclose (f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;

	IN_DeactivateForMenu();
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;

	IN_DeactivateForMenu();
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_load.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawArrowCursor (8, 32 + load_cursor*8);
}


void M_Save_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_save.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawArrowCursor (8, 32 + load_cursor*8);
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		M_ThrottledSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		M_ThrottledSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		M_ThrottledSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_state = m_none;
		IN_Activate();
		key_dest = key_game;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		M_ThrottledSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		M_ThrottledSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

void M_Load_Mousemove (float cx, float cy)
{
	int prev = load_cursor;
	M_UpdateCursor (cy, 32, 8, MAX_SAVEGAMES, &load_cursor);
	if (load_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

void M_Save_Mousemove (float cx, float cy)
{
	int prev = load_cursor;
	M_UpdateCursor (cy, 32, 8, MAX_SAVEGAMES, &load_cursor);
	if (load_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* Maps menu */

#define MAPLIST_OFS			32
#define MAPLIST_MAXCOLS		64

typedef struct
{
	const char				*name;
	const filelist_item_t	*source;
	int						mapidx;
	qboolean				active;
} mapitem_t;

static struct
{
	menulist_t		list;
	menuticker_t	ticker;
	int				x, y, cols;
	int				mapcount;			// not all items represent actual maps!
	qboolean		scrollbar_grab;
	int				prev_cursor;
	mapitem_t		*items;
} mapsmenu;

const char *M_Maps_GetMessage (const mapitem_t *item)
{
	if (!item->source)
		return item->name;
	return ExtraMaps_GetMessage (item->source);
}

static qboolean M_Maps_IsActive (const char *map)
{
	return cls.state == ca_connected && cls.signon == SIGNONS && !strcmp (cl.mapname, map);
}

static void M_Maps_AddDecoration (const char *text)
{
	mapitem_t item;
	memset (&item, 0, sizeof (item));
	item.name = text;
	item.mapidx = -1;
	VEC_PUSH (mapsmenu.items, item);
	mapsmenu.list.numitems++;
}

static void M_Maps_AddSeparator (maptype_t before, maptype_t after)
{
	#define QBAR "\35\36\37"

	if (after >= MAPTYPE_ID_START)
	{
		if (before < MAPTYPE_ID_START)
		{
			M_Maps_AddDecoration ("");
			M_Maps_AddDecoration (QBAR " Original Quake levels " QBAR);
		}
		M_Maps_AddDecoration ("");
	}
	else if (after >= MAPTYPE_CUSTOM_ID_START && before < MAPTYPE_CUSTOM_ID_START)
	{
		M_Maps_AddDecoration ("");
		M_Maps_AddDecoration (QBAR " Custom Quake levels " QBAR);
		M_Maps_AddDecoration ("");
	}
	else if (after >= MAPTYPE_MOD_START && before < MAPTYPE_MOD_START)
	{
		M_Maps_AddDecoration ("");
		M_Maps_AddDecoration (QBAR " Official mod levels " QBAR);
		M_Maps_AddDecoration ("");
	}

	#undef QBAR
}

static qboolean M_Maps_Match (int index)
{
	const char *message;
	if (mapsmenu.items[index].mapidx < 0)
		return false;

	if (q_strcasestr (mapsmenu.items[index].name, mapsmenu.list.search.text))
		return true;

	message = M_Maps_GetMessage (&mapsmenu.items[index]);
	return message && q_strcasestr (message, mapsmenu.list.search.text);
}

static qboolean M_Maps_IsSelectable (int index)
{
	return mapsmenu.items[index].source != NULL;
}

static void M_Maps_UpdateLayout (void)
{
	int height;

	M_UpdateBounds ();

	height = mapsmenu.list.numitems * 8 + MAPLIST_OFS + 16;
	height = q_min (height, m_height);
	mapsmenu.cols = m_width / 8 - 2;
	mapsmenu.cols = q_min (mapsmenu.cols, MAPLIST_MAXCOLS);
	mapsmenu.x = m_left + (m_width - mapsmenu.cols * 8) / 2;
	mapsmenu.y = m_top + (((m_height - height) / 2) & ~7);
	mapsmenu.list.viewsize = (height - MAPLIST_OFS - 16) / 8;
}

static void M_Maps_Init (void)
{
	int i, active, type, prev_type;

	M_Maps_UpdateLayout ();

	mapsmenu.scrollbar_grab = false;
	memset (&mapsmenu.list.search, 0, sizeof (mapsmenu.list.search));
	mapsmenu.list.search.match_fn = M_Maps_Match;
	mapsmenu.list.isactive_fn = M_Maps_IsSelectable;
	mapsmenu.list.cursor = -1;
	mapsmenu.list.scroll = 0;
	mapsmenu.list.numitems = 0;
	mapsmenu.mapcount = 0;
	VEC_CLEAR (mapsmenu.items);

	M_Ticker_Init (&mapsmenu.ticker);

	for (i = 0, active = -1, prev_type = -1; extralevels_sorted[i]; i++)
	{
		mapitem_t map;
		filelist_item_t *item = extralevels_sorted[i];

		type = ExtraMaps_GetType (item);
		if (type >= MAPTYPE_BMODEL)
			continue;
		if (prev_type != -1 && prev_type != type)
			M_Maps_AddSeparator (prev_type, type);
		prev_type = type;

		map.name = item->name;
		map.active = M_Maps_IsActive (item->name);
		map.source = item;
		map.mapidx = mapsmenu.mapcount++;
		if (map.active && active == -1)
			active = VEC_SIZE (mapsmenu.items);
		if ((map.active && !cls.demoplayback) || (mapsmenu.list.cursor == -1 && ExtraMaps_IsStart (type)))
			mapsmenu.list.cursor = VEC_SIZE (mapsmenu.items);
		VEC_PUSH (mapsmenu.items, map);
		mapsmenu.list.numitems++;
	}

	if (mapsmenu.list.cursor == -1)
		mapsmenu.list.cursor = (active != -1) ? active : 0;

	M_List_CenterCursor (&mapsmenu.list);

	mapsmenu.prev_cursor = mapsmenu.list.cursor;
}

void M_Menu_Maps_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_maps;
	m_entersound = true;
	M_Maps_Init ();

	// Handle optional map argument
	if (Cmd_Argc() >= 2)
	{
		char		mapname[MAX_QPATH];
		size_t		i;

		COM_StripExtension (Cmd_Argv (1), mapname, sizeof (mapname));

		for (i = 0; i < VEC_SIZE (mapsmenu.items); i++)
			if (q_strcasecmp (mapname, mapsmenu.items[i].name) == 0)
				break;

		if (i == VEC_SIZE (mapsmenu.items))
		{
			Con_SafePrintf ("Couldn't find map \"%s\".\n", mapname);
			return;
		}

		mapsmenu.list.cursor = i;
		M_SetSkillMenuMap (mapname);
		M_Menu_Skill_f ();
	}
}

void M_Maps_Draw (void)
{
	const char *str;
	int x, y, i, j, cols;
	int firstvis, numvis;
	int firstvismap, numvismaps;
	int namecols, desccols;

	M_Maps_UpdateLayout ();

	namecols = (int) CLAMP (14, mapsmenu.cols * 0.375f, 56) & ~1;
	desccols = mapsmenu.cols - 1 - namecols;

	if (!keydown[K_MOUSE1])
		mapsmenu.scrollbar_grab = false;

	M_List_Update (&mapsmenu.list);

	if (mapsmenu.prev_cursor != mapsmenu.list.cursor)
	{
		mapsmenu.prev_cursor = mapsmenu.list.cursor;
		M_Ticker_Init (&mapsmenu.ticker);
	}
	else
		M_Ticker_Update (&mapsmenu.ticker);

	x = mapsmenu.x;
	y = mapsmenu.y;
	cols = mapsmenu.cols;

	Draw_StringEx (x, y + 4, 12, "Levels");
	M_DrawQuakeBar (x - 8, y + 16, namecols + 1);
	M_DrawQuakeBar (x + namecols * 8, y + 16, cols + 1 - namecols);

	y += MAPLIST_OFS;

	firstvismap = -1;
	numvismaps = 0;
	M_List_GetVisibleRange (&mapsmenu.list, &firstvis, &numvis);
	for (i = 0; i < numvis; i++)
	{
		int idx = i + firstvis;
		const mapitem_t *item = &mapsmenu.items[idx];
		const char *message = M_Maps_GetMessage (item);
		int mask = item->active ? 128 : 0;
		qboolean selected = (idx == mapsmenu.list.cursor);

		if (!item->source)
		{
			M_PrintWhite (x + (cols - strlen (item->name))/2*8, y + i*8, item->name);
		}
		else
		{
			char buf[256];
			if (mapsmenu.list.search.len > 0)
				COM_TintSubstring (item->name, mapsmenu.list.search.text, buf, sizeof (buf));
			else
				q_strlcpy (buf, item->name, sizeof (buf));

			if (firstvismap == -1)
				firstvismap = item->mapidx;
			numvismaps++;

			for (j = 0; j < namecols - 2 && buf[j]; j++)
				M_DrawCharacter (x + j*8, y + i*8, buf[j] ^ mask);

			if (!message || message[0])
			{
				if (!message)
				{
					memset (buf, '.' | 0x80, desccols);
					buf[desccols] = '\0';
				}
				else if (mapsmenu.list.search.len > 0)
					COM_TintSubstring (message, mapsmenu.list.search.text, buf, sizeof (buf));
				else
					q_strlcpy (buf, message, sizeof (buf));

				GL_SetCanvasColor (1, 1, 1, 0.375f);
				for (/**/; j < namecols; j++)
					M_DrawCharacter (x + j*8, y + i*8, '.' | mask);
				if (message)
					GL_SetCanvasColor (1, 1, 1, 1);

				M_PrintScroll (x + namecols*8, y + i*8, desccols*8, buf,
					selected ? mapsmenu.ticker.scroll_time : 0.0, true);
				
				if (!message)
					GL_SetCanvasColor (1, 1, 1, 1);
			}
		}

		if (selected)
			M_DrawArrowCursor (x - 8, y + i*8);
	}

	str = va("%d-%d of %d", firstvismap + 1, firstvismap + numvismaps, mapsmenu.mapcount);
	M_Print (x + (cols - strlen (str))*8, y - 24, str);

	if (M_List_GetOverflow (&mapsmenu.list) > 0)
	{
		M_List_DrawScrollbar (&mapsmenu.list, x + cols*8 - 8, y);

		if (mapsmenu.list.scroll > 0)
			M_DrawEllipsisBar (x, y - 8, cols);
		if (mapsmenu.list.scroll + mapsmenu.list.viewsize < mapsmenu.list.numitems)
			M_DrawEllipsisBar (x, y + mapsmenu.list.viewsize*8, cols);
	}

	M_List_DrawSearch (&mapsmenu.list, x, y + mapsmenu.list.viewsize*8 + 4, namecols);
}

void M_Maps_Char (int key)
{
	M_List_Char (&mapsmenu.list, key);
}

textmode_t M_Maps_TextEntry (void)
{
	return mapsmenu.scrollbar_grab ? TEXTMODE_OFF : TEXTMODE_NOPOPUP;
}

void M_Maps_Key (int key)
{
	int x, y;

	if (mapsmenu.scrollbar_grab)
	{
		switch (key)
		{
		case K_ESCAPE:
		case K_BBUTTON:
		case K_MOUSE4:
		case K_MOUSE2:
			mapsmenu.scrollbar_grab = false;
			break;
		}
		return;
	}

	if (M_List_Key (&mapsmenu.list, key))
		return;

	if (M_Ticker_Key (&mapsmenu.ticker, key))
	{
		M_List_KeepSearchVisible (&mapsmenu.list, 1.0);
		return;
	}

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_List_ClearSearch (&mapsmenu.list);
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	enter:
		M_List_ClearSearch (&mapsmenu.list);
		if (mapsmenu.items[mapsmenu.list.cursor].name[0])
		{
			M_SetSkillMenuMap (mapsmenu.items[mapsmenu.list.cursor].name);
			M_Menu_Skill_f ();
		}
		else
			M_ThrottledSound ("misc/menu3.wav");
		break;

	case K_MOUSE1:
		x = m_mousex - mapsmenu.x - (mapsmenu.cols - 1) * 8;
		y = m_mousey - mapsmenu.y;
		if (x < -8 || !M_List_UseScrollbar (&mapsmenu.list, y))
			goto enter;
		mapsmenu.scrollbar_grab = true;
		M_Maps_Mousemove (m_mousex, m_mousey);
		break;

	default:
		break;
	}
}


void M_Maps_Mousemove (float cx, float cy)
{
	cy -= mapsmenu.y + MAPLIST_OFS;

	if (mapsmenu.scrollbar_grab)
	{
		if (!keydown[K_MOUSE1])
		{
			mapsmenu.scrollbar_grab = false;
			return;
		}
		M_List_UseScrollbar (&mapsmenu.list, cy);
		// Note: no return, we also update the cursor
	}

	M_List_Mousemove (&mapsmenu.list, cy);
}

//=============================================================================
/* SKILL MENU */

int				m_skill_cursor;
qboolean		m_skill_usegfx;
qboolean		m_skill_usecustomtitle;
qboolean		m_skill_canresume;
time_t			m_skill_lastplayed;
int				m_skill_numoptions;
char			m_skill_mapname[MAX_QPATH];
char			m_skill_maptitle[1024];
menuticker_t	m_skill_ticker;

enum m_state_e m_skill_prevmenu;

void M_SetSkillMenuMap (const char *name)
{
	q_strlcpy (m_skill_mapname, name, sizeof (m_skill_mapname));
	if (!Mod_LoadMapDescription (m_skill_maptitle, sizeof (m_skill_maptitle), name) || !m_skill_maptitle[0])
		q_strlcpy (m_skill_maptitle, name, sizeof (m_skill_maptitle));
}

void M_Menu_Skill_f (void)
{
	char autosave[MAX_OSPATH];

	m_skill_canresume = false;
	m_skill_lastplayed = 0;
	q_snprintf (autosave, sizeof (autosave), "%s/autosave/%s.sav", com_gamedir, m_skill_mapname);
	if (Sys_FileExists (autosave))
	{
		time_t now, lastplayed;
		m_skill_canresume = true;
		time (&now);
		if (Sys_GetFileTime (autosave, &lastplayed) && lastplayed <= now)
			m_skill_lastplayed = lastplayed;
	}

	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_skill_prevmenu = m_state;
	m_state = m_skill;
	m_entersound = true;
	M_Ticker_Init (&m_skill_ticker);

	if (m_skill_canresume)
	{
		// Select "Resume" option initially if available
		m_skill_cursor = 4;
	}
	else
	{
		// Select current skill level initially if there's no autosave
		m_skill_cursor = (int)skill.value;
		m_skill_cursor = CLAMP (0, m_skill_cursor, 3);
	}
	m_skill_numoptions = 4 + m_skill_canresume;
}

void M_Skill_Draw (void)
{
	int		x, y, f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic (m_skill_usecustomtitle && !m_skill_canresume ? "gfx/p_skill.lmp" : "gfx/ttl_sgl.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	x = 72;
	y = 32;

	M_Ticker_Update (&m_skill_ticker);
	M_PrintScroll (x, 32, 30*8, m_skill_maptitle, m_skill_ticker.scroll_time, false);

	y += 16;

	if (m_skill_usegfx)
	{
		M_DrawTransPic (x, y, Draw_CachePic ("gfx/skillmenu.lmp") );
		if (m_skill_cursor < 4)
			M_DrawQuakeCursor (x - 18, y + m_skill_cursor * 20);
		y += 4 * 20;
	}
	else
	{
		static const char *const skills[] =
		{
			"EASY",
			"NORMAL",
			"HARD",
			"NIGHTMARE",
		};

		for (f = 0; f < 4; f++)
			M_PrintEx (x, y + f*16 + 2, 12, skills[f]);
		if (m_skill_cursor < 4)
			M_DrawArrowCursor (x - 16, y + m_skill_cursor*16 + 4);
		y += 4 * 16;
	}

	if (m_skill_canresume)
	{
		y += 8;
		M_Print (x, y, "Resume last game");
		if (m_skill_lastplayed)
		{
			char	duration[32];
			time_t	now;

			time (&now);
			COM_DescribeDuration (duration, sizeof (duration), difftime (m_skill_lastplayed, now));
			M_Print (x, y + 8, va ("from %s ago", duration));
		}
		if (m_skill_cursor == 4)
			M_DrawArrowCursor (x - 16, y);
	}
}

void M_Skill_Key (int key)
{
	if (M_Ticker_Key (&m_skill_ticker, key))
		return;

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		m_state = m_skill_prevmenu;
		m_entersound = true;
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (++m_skill_cursor > m_skill_numoptions - 1)
			m_skill_cursor = 0;
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (--m_skill_cursor < 0)
			m_skill_cursor = m_skill_numoptions - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		IN_Activate();
		key_dest = key_game;
		if (sv.active)
			Cbuf_AddText ("disconnect\n");
		if (m_skill_cursor == 4)
		{
			// Resume autosave
			Cbuf_AddText (va ("load \"autosave/%s\"\n", m_skill_mapname));
		}
		else
		{
			// Fresh start
			Cbuf_AddText (va ("skill %d\n", m_skill_cursor));
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("deathmatch 0\n"); //johnfitz
			Cbuf_AddText ("coop 0\n"); //johnfitz
			Cbuf_AddText ("campaign 0\n");
			Cbuf_AddText (va ("map \"%s\"\n", m_skill_mapname));
		}
		break;
	}
}

void M_Skill_Mousemove (float cx, float cy)
{
	int ybase = 48;
	int itemheight = m_skill_usegfx ? 20 : 16;
	int prev = m_skill_cursor;

	if (m_skill_numoptions > 4 && cy > ybase + 4 * itemheight + 8/2)
		m_skill_cursor = 4;
	else
		M_UpdateCursor (cy, ybase, itemheight, 4, &m_skill_cursor);

	if (m_skill_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	3


void M_Menu_MultiPlayer_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
}


void M_MultiPlayer_Draw (void)
{
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	M_DrawQuakeCursor (54, 32 + m_multiplayer_cursor * 20);

	if (ipxAvailable || tcpipAvailable)
		return;
	M_PrintWhite ((320/2) - ((27*8)/2), 148, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (ipxAvailable || tcpipAvailable)
				M_Menu_Net_f ();
			break;

		case 1:
			if (ipxAvailable || tcpipAvailable)
				M_Menu_Net_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;
		}
	}
}


void M_MultiPlayer_Mousemove (float cx, float cy)
{
	int prev = m_multiplayer_cursor;
	M_UpdateCursor (cy, 32, 20, MULTIPLAYER_ITEMS, &m_multiplayer_cursor);
	if (m_multiplayer_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* SETUP MENU */

int		setup_cursor = 4;
int		setup_cursor_table[] = {40, 56, 80, 104, 140};

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	5

void M_Menu_Setup_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	Q_strcpy(setup_myname, cl_name.string);
	Q_strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
}


void M_Setup_Draw (void)
{
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_hostname);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_Print (168, 56, setup_myname);

	M_Print (64, 80, "Shirt color");
	M_Print (64, 104, "Pants color");

	M_DrawTextBox (64, 140-8, 14, 1);
	M_Print (72, 140, "Accept Changes");

	p = Draw_CachePic ("gfx/bigbox.lmp");
	M_DrawTransPic (160, 64, p);
	p = Draw_CachePic ("gfx/menuplyr.lmp");
	M_DrawTransPicTranslate (172, 72, p, setup_top, setup_bottom);

	M_DrawArrowCursor (56, setup_cursor_table [setup_cursor]);

	if (setup_cursor == 0)
		M_DrawTextCursor (168 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor]);

	if (setup_cursor == 1)
		M_DrawTextCursor (168 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor]);
}


void M_Setup_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
	//case K_MOUSE2:
		if (setup_cursor < 2)
			return;
		M_ThrottledSound ("misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top - 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		M_ThrottledSound ("misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top + 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom + 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3)
			goto forward;

		// setup_cursor == 4 (OK)
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (Q_strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;
	}

	if (setup_top > 13)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 13;
	if (setup_bottom > 13)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 13;
}


void M_Setup_Char (int k)
{
	int l;

	switch (setup_cursor)
	{
	case 0:
		l = strlen(setup_hostname);
		if (l < 15)
		{
			setup_hostname[l+1] = 0;
			setup_hostname[l] = k;
		}
		break;
	case 1:
		l = strlen(setup_myname);
		if (l < 15)
		{
			setup_myname[l+1] = 0;
			setup_myname[l] = k;
		}
		break;
	}
}


textmode_t M_Setup_TextEntry (void)
{
	return (setup_cursor == 0 || setup_cursor == 1) ? TEXTMODE_ON : TEXTMODE_OFF;
}


void M_Setup_Mousemove (float cx, float cy)
{
	int prev = setup_cursor;
	M_UpdateCursorWithTable (cy, setup_cursor_table, NUM_SETUP_CMDS, &setup_cursor);
	if (setup_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* NET MENU */

int	m_net_cursor;
int m_net_items;

const char *net_helpMessage [] =
{
/* .........1.........2.... */
  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 2;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	f = 32;

	if (ipxAvailable)
		p = Draw_CachePic ("gfx/netmen3.lmp");
	else
		p = Draw_CachePic ("gfx/dim_ipx.lmp");
	M_DrawTransPic (72, f, p);

	f += 19;
	if (tcpipAvailable)
		p = Draw_CachePic ("gfx/netmen4.lmp");
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
	M_DrawTransPic (72, f, p);

	f = (320-26*8)/2;
	M_DrawTextBox (f, 96, 24, 4);
	f += 8;
	M_Print (f, 104, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 112, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 120, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 128, net_helpMessage[m_net_cursor*4+3]);

	M_DrawQuakeCursor (54, 32 + m_net_cursor * 20);
}


void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		m_entersound = true;
		M_Menu_LanConfig_f ();
		break;
	}

	if (m_net_cursor == 0 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 1 && !tcpipAvailable)
		goto again;
}


void M_Net_Mousemove (float cx, float cy)
{
	int prev = m_net_cursor;
	M_UpdateCursor (cy, 32, 20, m_net_items, &m_net_cursor);
	if (m_net_cursor == 0 && !ipxAvailable)
		m_net_cursor = 1;
	if (m_net_cursor == 1 && !tcpipAvailable)
		m_net_cursor = 0;
	if (m_net_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* VIDEO MENU */

//TODO: replace these fixed-length arrays with hunk_allocated buffers
#define MAX_BPPS_LIST	5
#define MAX_RATES_LIST	20

typedef struct
{
	int width,height;
} vid_menu_mode;

static vid_menu_mode vid_menu_modes[MAX_MODE_LIST];
static int vid_menu_nummodes = 0;

static int vid_menu_rates[MAX_RATES_LIST];
static int vid_menu_numrates=0;

/*
================
VID_Menu_Init
================
*/
void VID_Menu_Init (void)
{
	int i, j, h, w;

	for (i = 0; i < nummodes; i++)
	{
		w = modelist[i].width;
		h = modelist[i].height;

		for (j = 0; j < vid_menu_nummodes; j++)
		{
			if (vid_menu_modes[j].width == w &&
				vid_menu_modes[j].height == h)
				break;
		}

		if (j == vid_menu_nummodes)
		{
			vid_menu_modes[j].width = w;
			vid_menu_modes[j].height = h;
			vid_menu_nummodes++;
		}
	}
}

/*
================
VID_Menu_RebuildRateList

regenerates rate list based on current vid_width and vid_height
================
*/
static void VID_Menu_RebuildRateList (void)
{
	int i, j, r;

	vid_menu_numrates = 0;

	for (i = 0; i < nummodes; i++)
	{
		//rate list is limited to rates available with current width/height
		if (modelist[i].width != vid_width.value ||
		    modelist[i].height != vid_height.value ||
		    modelist[i].bpp < 24)
			continue;

		r = modelist[i].refreshrate;

		for (j = 0; j < vid_menu_numrates; j++)
		{
			if (vid_menu_rates[j] == r)
				break;
		}

		if (j == vid_menu_numrates)
		{
			vid_menu_rates[j] = r;
			vid_menu_numrates++;
		}
	}

	//if there are no valid fullscreen refreshrates for this width/height, just pick one
	if (vid_menu_numrates == 0)
	{
		Cvar_SetValue ("vid_refreshrate",(float)modelist[0].refreshrate);
		return;
	}

	//if vid_refreshrate is not in the new list, change vid_refreshrate
	for (i = 0; i < vid_menu_numrates; i++)
		if (vid_menu_rates[i] == (int)(vid_refreshrate.value))
			break;

	if (i == vid_menu_numrates)
		Cvar_SetValue ("vid_refreshrate",(float)vid_menu_rates[0]);
}

/*
================
VID_Menu_ChooseNextResolution

chooses next resolution in order, then updates vid_width and
vid_height cvars, then updates refreshrate list
================
*/
static void VID_Menu_ChooseNextResolution (int dir)
{
	int i;

	if (vid_menu_nummodes)
	{
		for (i = 0; i < vid_menu_nummodes; i++)
		{
			if (vid_menu_modes[i].width == vid_width.value &&
				vid_menu_modes[i].height == vid_height.value)
				break;
		}

		if (i == vid_menu_nummodes) //can't find it in list, so it must be a custom windowed res
		{
			i = 0;
		}
		else
		{
			i += dir;
			if (i >= vid_menu_nummodes)
				i = 0;
			else if (i < 0)
				i = vid_menu_nummodes-1;
		}

		Cvar_SetValueQuick (&vid_width, (float)vid_menu_modes[i].width);
		Cvar_SetValueQuick (&vid_height, (float)vid_menu_modes[i].height);
		VID_Menu_RebuildRateList ();
	}
}

/*
================
VID_Menu_ChooseNextRate

chooses next refresh rate in order, then updates vid_refreshrate cvar
================
*/
static void VID_Menu_ChooseNextRate (int dir)
{
	int i;

	for (i = 0; i < vid_menu_numrates; i++)
	{
		if (vid_menu_rates[i] == vid_refreshrate.value)
			break;
	}

	if (i == vid_menu_numrates) //can't find it in list
	{
		i = 0;
	}
	else
	{
		i += dir;
		if (i >= vid_menu_numrates)
			i = 0;
		else if (i < 0)
			i = vid_menu_numrates-1;
	}

	Cvar_SetValue ("vid_refreshrate",(float)vid_menu_rates[i]);
}

typedef enum
{
	DISPLAYMODE_FULLSCREEN,
	DISPLAYMODE_WINDOWED,
	DISPLAYMODE_BORDERLESS,

	DISPLAYMODE_COUNT,
} windowmode_t;

static windowmode_t VID_Menu_GetDisplayMode (void)
{
	if (vid_fullscreen.value)
		return DISPLAYMODE_FULLSCREEN;
	return vid_borderless.value ? DISPLAYMODE_BORDERLESS : DISPLAYMODE_WINDOWED;
}

static void VID_Menu_SetDisplayMode (windowmode_t mode)
{
	Cvar_SetValueQuick (&vid_fullscreen, mode == DISPLAYMODE_FULLSCREEN);
	if (mode != DISPLAYMODE_FULLSCREEN)
		Cvar_SetValueQuick (&vid_borderless, mode == DISPLAYMODE_BORDERLESS);
}

/*
================
VID_Menu_ChooseNextDisplayMode

chooses next window mode in order, then updates vid_fullscreen and vid_borderless cvars
================
*/
static void VID_Menu_ChooseNextDisplayMode (int dir)
{
	windowmode_t mode = VID_Menu_GetDisplayMode ();
	mode = (mode + DISPLAYMODE_COUNT - dir) % DISPLAYMODE_COUNT;
	VID_Menu_SetDisplayMode (mode);
}

/*
================
VID_Menu_ChooseNextAA

chooses next AA level in order, then updates vid_fsaa cvar
================
*/
static void VID_Menu_ChooseNextAA (int dir)
{
	int samples = framebufs.scene.samples;
	if (dir < 0)
		samples <<= 1;
	else
		samples >>= 1;
	Cvar_SetValueQuick (&vid_fsaa, CLAMP (1, samples, framebufs.max_samples));
}

/*
================
VID_Menu_ChooseNextAnisotropy

chooses next anisotropy level in order, then updates gl_texture_anisotropy cvar
================
*/
static void VID_Menu_ChooseNextAnisotropy (int dir)
{
	int aniso = (int)gl_texture_anisotropy.value;
	if (dir < 0)
		aniso <<= 1;
	else
		aniso >>= 1;
	Cvar_SetValueQuick (&gl_texture_anisotropy, CLAMP (1, aniso, (int)gl_max_anisotropy));
}

/*
================
VID_Menu_GetTexFilterDesc
================
*/
static const char *VID_Menu_GetTexFilterDesc (void)
{
	if ((unsigned int) gl_texfilter.mode < (unsigned int)NUM_GLMODES)
		return glmodes[gl_texfilter.mode].uiname;
	return "";
}

/*
================
VID_Menu_ChooseNextFPSLimit

chooses next fps limit in order, then updates host_maxfps cvar
================
*/
static void VID_Menu_ChooseNextFPSLimit (int dir)
{
	static const int values[] = {0, 60, 72, 100, 120, 144, 165, 180, 200, 240, 300, 360, 500};
	int i, current = (int)host_maxfps.value;

	if (dir < 0)
		for (i = 0; i < countof (values) && values[i] <= current; i++)
			;
	else
		for (i = countof (values) - 1; i >= 0 && values[i] >= current; i--)
			;

	if (i < 0)
		i = countof (values) - 1;
	else if (i == countof (values))
		i = 0;

	Cvar_SetValueQuick (&host_maxfps, values[i]);
}

/*
================
VID_Menu_GetSoftEmuDesc
================
*/
static const char *VID_Menu_GetSoftEmuDesc (void)
{
	switch (softemu)
	{
	case SOFTEMU_BANDED: return "Raw";
	case SOFTEMU_COARSE: return "Balanced";
	case SOFTEMU_FINE: return "Subtle";
	case SOFTEMU_OFF: return "Off";
	default: return "";
	}
}

/*
================
VID_Menu_GetWaterWarpDesc
================
*/
static const char *VID_Menu_GetWaterWarpDesc (void)
{
	switch ((int)r_waterwarp.value)
	{
	case 0: return "Off";
	case 1: return "Classic";
	case 2: return "glQuake";
	default: return "";
	}
}

/*
================
VID_Menu_GetParticlesDesc
================
*/
static const char *VID_Menu_GetParticlesDesc (void)
{
	switch ((int)r_particles.value)
	{
	case 0: return "Off";
	case 1: return "glQuake";
	case 2: return "Classic";
	default: return "";
	}
}

/*
================
VID_Menu_ChooseNextAlphaMode
================
*/
static void VID_Menu_ChooseNextAlphaMode (int dir)
{
	R_SetAlphaMode ((R_GetAlphaMode () + ALPHAMODE_COUNT + dir) % ALPHAMODE_COUNT);
}

/*
================
VID_Menu_GetAlphaModeDesc
================
*/
static const char *VID_Menu_GetAlphaModeDesc (void)
{
	switch (R_GetEffectiveAlphaMode ())
	{
	case ALPHAMODE_BASIC:		return "Basic";
	case ALPHAMODE_SORTED:		return "Dynamic";
	case ALPHAMODE_OIT:			return "Modern";
	default:					return "";
	}
}

/*
================
M_Menu_Video_f
================
*/
void M_Menu_Video_f (void)
{
	M_Options_Init (m_video);
}

//=============================================================================
/* CALIBRATION SCREEN */

static enum
{
	CALIBRATION_INTRO_TEXT,
	CALIBRATION_IN_ROGRESS,
	CALIBRATION_FINISHED,
} calibration_state;

static double calibration_finished_delay;

void M_Menu_Calibration_f (void)
{
	IN_DeactivateForMenu();
	m_state = m_calibration;
	calibration_state = CALIBRATION_INTRO_TEXT;
	calibration_finished_delay = 1.0;
}

void M_Calibration_Draw (void)
{
	char anim[16];
	int i, progress;
	int y = 72;

	switch (calibration_state)
	{
	case CALIBRATION_INTRO_TEXT:
		M_PrintAligned (160, y - 8,	ALIGN_CENTER, "Before calibration,");
		M_PrintAligned (160, y,		ALIGN_CENTER, "place the controller");
		M_PrintAligned (160, y + 8, ALIGN_CENTER, "on a flat, stable surface");
		y += 24;
		M_DrawTextBox (160 - 5*8, y, 8, 1);
		M_DrawArrowCursor (160 - 6*8, y + 8);
		M_PrintAligned (160, y + 8, ALIGN_CENTER, "Continue");
		break;

	case CALIBRATION_IN_ROGRESS:
		progress = (int) (IN_GetGyroCalibrationProgress () * (Q_COUNTOF (anim) - 1) + 0.5f);
		for (i = 0; i < (int) Q_COUNTOF (anim) - 1; i++)
			anim[i] = i < progress ? QCHAR_COLORED ('.') : '.';
		anim[i] = '\0';
		M_PrintAligned (160, y, ALIGN_CENTER, "Calibrating, please wait...");
		M_PrintAligned (160, y + 16, ALIGN_CENTER, anim);
		if (!IN_IsCalibratingGyro ())
			calibration_state = CALIBRATION_FINISHED;
		break;

	case CALIBRATION_FINISHED:
		M_PrintAligned (160, y, ALIGN_CENTER, "Calibration complete!");
		calibration_finished_delay -= host_rawframetime;
		if (calibration_finished_delay < 0.0)
			M_Menu_Gamepad_f ();
		break;

	default:
		break;
	}
}

void M_Calibration_Key (int key)
{
	if (calibration_state != CALIBRATION_INTRO_TEXT)
		return;

	switch (key)
	{
	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		calibration_state = CALIBRATION_IN_ROGRESS;
		M_ThrottledSound ("misc/menu2.wav");
		IN_StartGyroCalibration ();
		break;

	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Gamepad_f ();
		break;

	default:
		break;
	}
}

//=============================================================================
/* GAMEPAD MENU */

#define MIN_JOY_SENS			60.f
#define MAX_JOY_SENS			720.f
#define MIN_JOY_EXPONENT		1.f
#define MAX_JOY_EXPONENT		5.f
#define MIN_STICK_DEADZONE		0.f
#define MAX_STICK_DEADZONE		0.75f
#define MIN_TRIGGER_DEADZONE	0.f
#define MAX_TRIGGER_DEADZONE	0.75f
#define MIN_GYRO_SENS			0.1f
#define MAX_GYRO_SENS			20.f
#define MIN_GYRO_NOISE_THRESH	0.f
#define MAX_GYRO_NOISE_THRESH	5.f

/*
================
M_Menu_Gamepad_f
================
*/
void M_Menu_Gamepad_f (void)
{
	M_Options_Init (m_gamepad);
}

//=============================================================================
/* OPTIONS MENU */

#define PP_CONCAT2(a,b)		a##b
#define PP_CONCAT(a,b)		PP_CONCAT2 (a, b)
#define SPACER				PP_CONCAT (_SPACER_, __COUNTER__)
#define TITLE_BAR			"\35\36\37"
#define TITLE(str)			TITLE_BAR " " str " " TITLE_BAR

////////////////////////////////////////////////////////////////////////
#define OPTIONS_LIST(begin_menu, item, end_menu)						\
	begin_menu (OPTIONS, m_options, "")									\
		item (OPT_PREVIEW,				"Live Preview")					\
		item (OPT_GAMMA,				"Brightness")					\
		item (OPT_CONTRAST,				"Contrast")						\
		item (OPT_VIDEO,				"Display")						\
		item (OPT_GRAPHICS,				"Graphics")						\
		item (OPT_INTERFACE,			"Interface")					\
		item (SPACER,					"")								\
		item (OPT_CUSTOMIZE,			"Key Setup")					\
		item (OPT_GAMEPAD,				"Controller")					\
		item (OPT_MOUSESPEED,			"Mouse Speed")					\
		item (OPT_INVMOUSE,				"Invert Mouse")					\
		item (OPT_SNDVOL,				"Sound Volume")					\
		item (OPT_MUSICVOL,				"Music Volume")					\
		item (OPT_GAME,					"Game")							\
		item (OPT_MODS,					"Mods")							\
		item (SPACER,					"")								\
		item (OPT_CONSOLE,				"Console")						\
		item (OPT_DEFAULTS,				"Reset All")					\
	end_menu ()															\
	begin_menu (VIDEO_OPTIONS, m_video, TITLE("Display"))				\
		item (OPT_RESOLUTION,			"Resolution")					\
		item (OPT_DISPLAYMODE,			"Display Mode")					\
		item (OPT_REFRESHRATE,			"Refresh Rate")					\
		item (OPT_TEST_VIDEO,			"Test changes")					\
		item (OPT_APPLY_VIDEO,			"Apply changes")				\
		item (SPACER,					"")								\
		item (OPT_VSYNC,				"Vertical Sync")				\
		item (OPT_FPSLIMIT,				"FPS Limit")					\
	end_menu ()															\
	begin_menu (GRAPHICS_OPTIONS, m_graphics, TITLE("Graphics"))		\
		item (OPT_SOFTEMU,				"8-bit Mode")					\
		item (OPT_SOFTEMU_MDL,			"Model Warping")				\
		item (OPT_ENHANCEDMODELS,		"Models")						\
		item (OPT_ANIMLERP,				"Animations")					\
		item (OPT_TEXFILTER,			"Textures")						\
		item (OPT_ANISO,				"Anisotropic")					\
		item (SPACER,					"")								\
		item (OPT_RENDERSCALE,			"Render Scale")					\
		item (OPT_FSAA,					"Antialiasing")					\
		item (OPT_FSAA_MODE,			"AA Mode")						\
		item (SPACER,					"")								\
		item (OPT_ALPHAMODE,			"Transparency")					\
		item (OPT_PARTICLES,			"Particles")					\
		item (OPT_WATERWARP,			"Underwater FX")				\
		item (OPT_DLIGHTS,				"Dynamic Lights")				\
	end_menu ()															\
	begin_menu (INTERFACE_OPTIONS, m_interface, TITLE("Interface"))		\
		item (OPT_UISCALE,				"Scale")						\
		item (OPT_PIXELASPECT,			"Pixels")						\
		item (OPT_UIMOUSE,				"Mouse")						\
		item (SPACER,					"")								\
		item (OPT_HUDSTYLE,				"HUD Layout")					\
		item (OPT_SBALPHA,				"HUD Alpha")					\
		item (OPT_HUDLEVEL,				"HUD Detail")					\
		item (OPT_CROSSHAIR,			"Crosshair")					\
		item (OPT_SHOWSPEED,			"Show Speed")					\
		item (OPT_SHOWTIME,				"Show Time")					\
		item (OPT_SHOWFPS,				"Show FPS")						\
		item (SPACER,					"")								\
		item (OPT_MENUBGSTYLE,			"Background")					\
		item (OPT_MENUBGALPHA,			"BG Alpha")						\
		item (OPT_CENTERPRINTBG,		"Center Text BG")				\
		item (OPT_CONALPHA,				"Console Alpha")				\
		item (OPT_CONBRIGHTNESS,		"Darken Console")				\
		item (OPT_CONFIRMQUIT,			"Quit Prompt")					\
		item (OPT_STARTDEMOS,			"Start Demos")					\
	end_menu ()															\
	begin_menu (GAMEPAD_OPTIONS, m_gamepad, TITLE("Controller"))		\
		item (GPAD_OPT_DEVICE,			"Device")						\
		item (SPACER,					"")								\
		item (SPACER,					"")								\
		item (GPAD_OPT_SENSX,			"Yaw Speed")					\
		item (GPAD_OPT_SENSY,			"Pitch Speed")					\
		item (GPAD_OPT_INVERT,			"Invert Pitch")					\
		item (GPAD_OPT_SWAP_MOVELOOK,	"Look Stick")					\
		item (SPACER,					"")								\
		item (GPAD_OPT_EXPONENT_LOOK,	"Look Accel")					\
		item (GPAD_OPT_EXPONENT_MOVE,	"Move Accel")					\
		item (SPACER,					"")								\
		item (GPAD_OPT_DEADZONE_LOOK,	"Look Deadzone")				\
		item (GPAD_OPT_DEADZONE_MOVE,	"Move Deadzone")				\
		item (GPAD_OPT_DEADZONE_TRIG,	"Trigger Thresh")				\
		item (SPACER,					"")								\
		item (GPAD_OPT_RUMBLE,			"Vibration")					\
		item (SPACER,					"")								\
		item (GPAD_OPT_GYROENABLE,		"Gyro")							\
		item (GPAD_OPT_FLICKSTICK,		"Flick Stick")					\
		item (GPAD_OPT_GYROMODE,		"Gyro Button")					\
		item (GPAD_OPT_GYROAXIS,		"Gyro Axis")					\
		item (GPAD_OPT_GYROSENSX,		"Gyro Yaw Speed")				\
		item (GPAD_OPT_GYROSENSY,		"Gyro Pitch Speed")				\
		item (GPAD_OPT_GYRONOISE,		"Gyro Noise Thresh")			\
		item (GPAD_OPT_CALIBRATE,		"Calibrate")					\
	end_menu ()															\
	begin_menu (GAME_OPTIONS, m_game, TITLE("Game"))					\
		item (OPT_FOV,					"Field Of View")				\
		item (OPT_FOVDISTORT,			"Gun Distortion")				\
		item (OPT_FLASHALPHA,			"Screen Flashes")				\
		item (OPT_RECOIL,				"Recoil")						\
		item (OPT_VIEWBOB,				"View Bob")						\
		item (OPT_ANGLELIMITS,			"Angle Limits")					\
		item (OPT_ALWAYSRUN,			"Always Run")					\
		item (SPACER,					"")								\
		item (OPT_ALWAYSMLOOK,			"Mouse Look")					\
		item (OPT_LOOKSTRAFE,			"Look Strafe")					\
		item (OPT_LOOKSPRING,			"Look Spring")					\
		item (SPACER,					"")								\
		item (OPT_LANGUAGE,				"Language")						\
		item (SPACER,					"")								\
		item (OPT_MUSICEXT,				"External Music")				\
		item (OPT_WATERSNDFX,			"Water Muffling")				\
	end_menu ()															\
////////////////////////////////////////////////////////////////////////

#define PP_IGNORE_ARGS(...)

enum 
{
	// Add option id's and BEGIN values
	#define BEGIN_MENU_OPT(prefix, state, desc)		prefix##_BEGIN, _##prefix##_REWIND = prefix##_BEGIN - 1,
	#define ADD_OPTION_ENUM(id, name)				id,
	OPTIONS_LIST (BEGIN_MENU_OPT, ADD_OPTION_ENUM, PP_IGNORE_ARGS)
	#undef ADD_OPTION_ENUM
	#undef BEGIN_MENU_OPT

	// Add END values
	#define ADD_END_VALUE(prefix, state, desc)		prefix##_END = prefix##_BEGIN
	#define COUNT_OPTION(id, name)					+1
	#define ADD_COMMA()								,
	OPTIONS_LIST(ADD_END_VALUE, COUNT_OPTION, ADD_COMMA)
	#undef ADD_END_VALUE
	#undef COUNT_OPTION
	#undef ADD_COMMA

	// Add NUMITEMS values
	#define ADD_NUMITEMS(prefix, state, desc)		prefix##_NUMITEMS = prefix##_END - prefix##_BEGIN,
	OPTIONS_LIST(ADD_NUMITEMS, PP_IGNORE_ARGS, PP_IGNORE_ARGS)
	#undef ADD_NUMITEMS

	// Gyro sub-range
	GYRO_OPTIONS_BEGIN			= GPAD_OPT_GYROENABLE,
	GYRO_OPTIONS_END			= GPAD_OPT_CALIBRATE + 1,
	GYRO_OPTIONS_NUMITEMS		= GYRO_OPTIONS_END - GYRO_OPTIONS_BEGIN,
};

static const char *const options_names[] =
{
	#define ADD_OPTION_NAME(id, name) name,
	OPTIONS_LIST (PP_IGNORE_ARGS, ADD_OPTION_NAME, PP_IGNORE_ARGS)
	#undef ADD_OPTION_NAME
};

typedef struct
{
	const char*		description;
	enum m_state_e	state;
	int				first_item;
	int				num_items;
} optsubmenu_t;

static const optsubmenu_t options_submenus[] =
{
	#define ADD_SUBMENU(prefix, state, desc)			{desc, state, prefix##_BEGIN, prefix##_NUMITEMS},
	OPTIONS_LIST (ADD_SUBMENU, PP_IGNORE_ARGS, PP_IGNORE_ARGS)
	#undef ADD_SUBMENU
};

static int M_Options_FindSubmenu (enum m_state_e state)
{
	int i;
	for (i = 0; i < (int) Q_COUNTOF (options_submenus); i++)
		if (options_submenus[i].state == state)
			return i;
	Sys_Error ("M_Options_FindSubmenu: invalid index %d", state);
	return -1;
}

static qboolean M_Options_IsGyroId (int id)
{
	return (unsigned int)(id - GYRO_OPTIONS_BEGIN) < GYRO_OPTIONS_NUMITEMS;
}

enum
{
	ALWAYSRUN_OFF = 0,
	ALWAYSRUN_VANILLA,
	ALWAYSRUN_QUAKESPASM,
	ALWAYSRUN_ITEMS
};

#define	SLIDER_RANGE		10

#define OPTIONS_LISTOFS		36
#define OPTIONS_MIDPOS		204

#define FOV_MIN				50.f
#define FOV_MAX				130.f

struct
{
	menulist_t		list;
	int				y;
	int				yofs;
	const char		*subtitle;
	int				first_item;

	int				submenu_cursors[Q_COUNTOF (options_submenus)];
	int				*last_cursor;

	struct
	{
		int			id;
		float		frac;
		float		frac_target;
		float		hold_time;
	}				preview;
} optionsmenu;

qboolean slider_grab;
float target_scale_frac;

void M_Options_SelectMods (void)
{
	SDL_assert (options_submenus[0].state == m_options);
	if (m_state == m_options)
		optionsmenu.list.cursor = OPT_MODS;
	else
		optionsmenu.submenu_cursors[0] = OPT_MODS;
}

static void M_Options_UpdateLayout (void)
{
	int height;

	M_UpdateBounds ();

	if (optionsmenu.subtitle && optionsmenu.subtitle[0])
		optionsmenu.yofs = 12;
	else
		optionsmenu.yofs = 0;

	height = OPTIONS_LISTOFS + optionsmenu.yofs + optionsmenu.list.numitems * 8;

	// Shift items down a bit if the total height is really small
	optionsmenu.yofs += (q_max (0, 144 - height) / 3) & ~7;

	// Enforce a minimum height, so that if the number of items is relatively small
	// the title pic doesn't get drawn below its usual position
	height = q_max (height, 192);

	if (height <= m_height)
	{
		optionsmenu.y = (m_top + (m_height - height) / 2) & ~7;
		optionsmenu.list.viewsize = optionsmenu.list.numitems;
	}
	else
	{
		optionsmenu.y = m_top;
		// Note: -8 for the bottom ellipsis bar
		optionsmenu.list.viewsize = (m_height - OPTIONS_LISTOFS - optionsmenu.yofs - 8) / 8;
	}

	M_List_Rescroll (&optionsmenu.list);
}

static int M_Options_GetSelected (void)
{
	return optionsmenu.list.cursor + optionsmenu.first_item;
}

static qboolean M_Options_IsEnabled (int index)
{
	if ((unsigned int) index >= (unsigned int)optionsmenu.list.numitems)
		return false;
	index += optionsmenu.first_item;
	if ((unsigned int) index >= countof (options_names))
		return false;
	if (index == OPT_TEXFILTER || index == OPT_ANISO)
		return !TexMgr_UsesFilterOverride ();
	if (index > GAMEPAD_OPTIONS_BEGIN && index < GAMEPAD_OPTIONS_END && !IN_HasGamepad ())
		return false;
	if (index == GPAD_OPT_RUMBLE && !IN_HasRumble ())
		return false;
	if (index == OPT_ALPHAMODE && map_checks.value)
		return false;
	if (M_Options_IsGyroId (index))
	{
		if (!IN_HasGyro ())
			return false;
		if (!gyro_enable.value && index > GYRO_OPTIONS_BEGIN)
			return false;
	}
	return true;
}

static qboolean M_Options_IsSelectable (int index)
{
	if (!M_Options_IsEnabled (index))
		return false;
	index += optionsmenu.first_item;
	return options_names[index][0] != '\0';
}

static qboolean M_Options_Match (int index)
{
	const char *name;
	index += optionsmenu.first_item;
	if ((unsigned int) index >= countof (options_names))
		return false;
	name = options_names[index];
	if (!*name)
		return false;
	return q_strcasestr (name, optionsmenu.list.search.text) != NULL;
}

static qboolean M_Options_WantsConsole (void)
{
	return optionsmenu.preview.id == OPT_CONALPHA || optionsmenu.preview.id == OPT_CONBRIGHTNESS;
}

static qboolean M_Options_ForcedCenterPrint (void)
{
	return optionsmenu.preview.id == OPT_CENTERPRINTBG && !cl.intermission;
}

static qboolean M_Options_ForcedUnderwater (void)
{
	return optionsmenu.preview.id == OPT_WATERWARP;
}

static float M_Options_PreviewAlpha (void)
{
	return optionsmenu.preview.frac;
}

static void M_Options_Preview (int id)
{
	if (cls.state == ca_connected && cls.signon == SIGNONS)
	{
		if (id < GRAPHICS_OPTIONS_BEGIN || id >= GRAPHICS_OPTIONS_END)
		{
			switch (id)
			{
			case OPT_GAMMA:
			case OPT_CONTRAST:
			case OPT_UISCALE:
			case OPT_HUDSTYLE:
			case OPT_SBALPHA:
			case OPT_HUDLEVEL:
			case OPT_CONALPHA:
			case OPT_CONBRIGHTNESS:
			case OPT_FOV:
			case OPT_FOVDISTORT:
				break;

			case OPT_CONFIRMQUIT:
				M_ChooseQuitMessage ();
				break;

			case OPT_CENTERPRINTBG:
				if (cl.intermission)
				{
					id = -1;
					break;
				}
				SCR_CenterPrint (
					"Certain messages appear inconveniently\n"
					"in the middle of your view. These are\n"
					"always important, and you do not want\n"
					"to ignore them!"
				);
				break;

			default:
				id = -1;
				break;
			}
		}
	}
	else
	{
		if (id != OPT_CONBRIGHTNESS)
			id = -1;
	}

	if (!ui_live_preview.value)
		id = -1;

	if (id < 0)
	{
		optionsmenu.preview.frac_target = 0.f;
		optionsmenu.preview.hold_time = 0.f;
		return;
	}

	optionsmenu.preview.id = id;
	optionsmenu.preview.frac_target = 1.f;
	optionsmenu.preview.hold_time = (id == OPT_WATERWARP) ? PREVIEW_LONG_HOLD_TIME : PREVIEW_HOLD_TIME;
}

static void M_Options_ResetPreview (void)
{
	if (M_Options_ForcedCenterPrint ())
		SCR_CenterPrint ("");

	optionsmenu.preview.frac = 0.f;
	optionsmenu.preview.id = -1;
	optionsmenu.preview.frac_target = 0.f;
	optionsmenu.preview.hold_time = 0.f;
}

void M_Options_Init (enum m_state_e state)
{
	int submenu = M_Options_FindSubmenu (state);

	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = state;
	m_entersound = true;
	slider_grab = false;

	if (state == m_video)
	{
		//set all the cvars to match the current mode when entering the menu
		VID_SyncCvars ();

		//set up rate list based on current cvars
		VID_Menu_RebuildRateList ();
	}

	optionsmenu.first_item = options_submenus[submenu].first_item;
	optionsmenu.list.numitems = options_submenus[submenu].num_items;
	optionsmenu.last_cursor = &optionsmenu.submenu_cursors[submenu];
	optionsmenu.subtitle = options_submenus[submenu].description;

	optionsmenu.list.cursor = *optionsmenu.last_cursor;
	optionsmenu.list.isactive_fn = M_Options_IsSelectable;
	optionsmenu.list.search.match_fn = M_Options_Match;

	// reset fading
	M_Options_ResetPreview ();

	M_List_ClearSearch (&optionsmenu.list);

	// If the cursor is on an inactive item, move it to the next active one
	if (!M_Options_IsSelectable (optionsmenu.list.cursor))
		M_List_SelectNextActive (&optionsmenu.list, optionsmenu.list.cursor, 1, true);

	M_Options_UpdateLayout ();
}

void M_Menu_Options_f (void)
{
	M_Options_Init (m_options);
}

typedef enum
{
	UI_MOUSE_OFF,
	UI_MOUSE_QUIET,
	UI_MOUSE_NOISY,

	UI_MOUSE_NUMSETTINGS,
} uimouse_t;

static uimouse_t M_Options_GetUIMouse (void)
{
	if (!ui_mouse.value)
		return UI_MOUSE_OFF;
	return ui_mouse_sound.value ? UI_MOUSE_NOISY : UI_MOUSE_QUIET;
}

static void M_Options_SetUIMouse (uimouse_t opt)
{
	switch (opt)
	{
	case UI_MOUSE_OFF:
		Cvar_SetValueQuick (&ui_mouse, 0.f);
		break;
	case UI_MOUSE_QUIET:
		Cvar_SetValueQuick (&ui_mouse, 1.f);
		Cvar_SetValueQuick (&ui_mouse_sound, 0.f);
		break;
	case UI_MOUSE_NOISY:
		Cvar_SetValueQuick (&ui_mouse, 1.f);
		Cvar_SetValueQuick (&ui_mouse_sound, 1.f);
		break;
	default:
		break;
	}
}

static void M_CycleCvar (cvar_t *cvar, int minval, int maxval, int dir)
{
	int range = maxval + 1 - minval;
	int value = (int) cvar->value + dir;
	value -= minval;
	value %= range;
	if (value < 0)
		value += range;
	value += minval;
	Cvar_SetValueQuick (cvar, (float) value);
}

void M_AdjustSliders (int dir)
{
	int	curr_alwaysrun, target_alwaysrun;
	float	f, l;

	M_ThrottledSound ("misc/menu3.wav");
	M_List_ClearSearch (&optionsmenu.list);

	switch (M_Options_GetSelected ())
	{
	case OPT_UISCALE:	// console and menu scale
		l = ((vid.width + 31) / 32) / 10.0;
		f = scr_conscale.value + dir * .1;
		if (f < 1)	f = 1;
		else if(f > l)	f = l;
		Cvar_SetValue ("scr_conscale", f);
		Cvar_SetValue ("scr_menuscale", f);
		Cvar_SetValue ("scr_sbarscale", f);
		Cvar_SetValue ("scr_crosshairscale", f);
		break;
	case OPT_HUDLEVEL:	// hud detail
		f = scr_viewsize.value - dir * 10;
		if (f > 130)		f = 130;
		else if(f < 100)	f = 100;
		Cvar_SetValue ("viewsize", f);
		break;
	case OPT_PIXELASPECT:	// 2D pixel aspect ratio
		Cvar_Set ("scr_pixelaspect", vid.guipixelaspect == 1.f ? "5:6" : "1");
		break;
	case OPT_CROSSHAIR:		// crosshair
		Cvar_SetValueQuick (&crosshair, ((int) q_max (crosshair.value, 0.f) + 3 + dir) % 3);
		break;
	case OPT_UIMOUSE:	// UI mouse support
		M_Options_SetUIMouse ((M_Options_GetUIMouse () + UI_MOUSE_NUMSETTINGS + dir) % UI_MOUSE_NUMSETTINGS);
		break;
	case OPT_GAMMA:	// gamma
		f = vid_gamma.value - dir * 0.05;
		if (f < 0.5)	f = 0.5;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("gamma", f);
		break;
	case OPT_CONTRAST:	// contrast
		f = vid_contrast.value + dir * 0.1;
		if (f < 1)	f = 1;
		else if (f > 2)	f = 2;
		Cvar_SetValue ("contrast", f);
		break;
	case OPT_MOUSESPEED:	// mouse speed
		f = sensitivity.value + dir * 0.5;
		if (f > 11)	f = 11;
		else if (f < 1)	f = 1;
		Cvar_SetValue ("sensitivity", f);
		break;
	case OPT_SBALPHA:	// statusbar alpha
		f = scr_sbaralpha.value - dir * 0.05;
		if (f < 0)	f = 0;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("scr_sbaralpha", f);
		break;
	case OPT_MUSICVOL:	// music volume
		f = bgmvolume.value + dir * 0.05f;
		if (f < 0)	f = 0;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("bgmvolume", f);
		break;
	case OPT_MUSICEXT:	// enable external music vs cdaudio
		Cvar_Set ("bgm_extmusic", bgm_extmusic.value ? "0" : "1");
		break;
	case OPT_SNDVOL:	// sfx volume
		f = sfxvolume.value + dir * 0.05f;
		if (f < 0)	f = 0;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("volume", f);
		break;
	case OPT_WATERSNDFX:
		Cbuf_AddText ("toggle snd_waterfx\n");
		break;

	case OPT_HUDSTYLE:	// hud style
		Cvar_SetValueQuick (&scr_hudstyle, ((int) q_max (scr_hudstyle.value, 0.f) + (int) HUD_COUNT + dir) % (int) HUD_COUNT);
		break;

	case OPT_PREVIEW:
		Cbuf_AddText ("toggle ui_live_preview\n");
		break;

	case OPT_ALWAYSRUN:	// always run
		if (cl_alwaysrun.value)
			curr_alwaysrun = ALWAYSRUN_QUAKESPASM;
		else if (cl_forwardspeed.value > 200)
			curr_alwaysrun = ALWAYSRUN_VANILLA;
		else
			curr_alwaysrun = ALWAYSRUN_OFF;
			
		target_alwaysrun = (ALWAYSRUN_ITEMS + curr_alwaysrun + dir) % ALWAYSRUN_ITEMS;
			
		if (target_alwaysrun == ALWAYSRUN_VANILLA)
		{
			Cvar_SetValue ("cl_alwaysrun", 0);
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
		}
		else if (target_alwaysrun == ALWAYSRUN_QUAKESPASM)
		{
			Cvar_SetValue ("cl_alwaysrun", 1);
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		else // ALWAYSRUN_OFF
		{
			Cvar_SetValue ("cl_alwaysrun", 0);
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		break;

	case OPT_VIEWBOB:	// view bob+roll
		if (cl_bob.value)
		{
			Cvar_SetValueQuick (&cl_bob, 0.f);
			Cvar_SetValueQuick (&cl_rollangle, 0.f);
		}
		else
		{
			Cvar_SetQuick (&cl_bob, cl_bob.default_string);
			Cvar_SetQuick (&cl_rollangle, cl_rollangle.default_string);
		}
		break;

	case OPT_ANGLELIMITS:	// pitch limits
		if (cl_maxpitch.value < 90.f)
		{
			Cvar_SetValueQuick (&cl_maxpitch, 90.f);
			Cvar_SetValueQuick (&cl_minpitch, -90.f);
		}
		else
		{
			Cvar_SetValueQuick (&cl_maxpitch, 80.f);
			Cvar_SetValueQuick (&cl_minpitch, -70.f);
		}
		break;

	case OPT_RECOIL:	// gun kick
		Cvar_SetValueQuick (&v_gunkick, ((int) q_max (v_gunkick.value, 0.f) + 3 + dir) % 3);
		break;

	case OPT_FLASHALPHA:	// flash intensity
		Cvar_SetValueQuick (&gl_cshiftpercent, CLAMP (0.f, gl_cshiftpercent.value + dir * 10.f, 100.f));
		break;

	case OPT_FOV:	// field of view
		Cvar_SetValueQuick (&scr_fov, CLAMP (FOV_MIN, scr_fov.value + dir * 5.f, FOV_MAX));
		break;

	case OPT_FOVDISTORT:	// FOV distortion
		Cvar_SetValueQuick (&cl_gun_fovscale, CLAMP (0.0, cl_gun_fovscale.value - dir * 0.1, 1.0));
		break;

	case OPT_INVMOUSE:	// invert mouse
		Cvar_SetValue ("m_pitch", -m_pitch.value);
		break;

	case OPT_ALWAYSMLOOK:
		if ((in_mlook.state & 1) || freelook.value)
		{
			Cvar_SetValueQuick (&freelook, 0.f);
			Cbuf_AddText("-mlook");
		}
		else
		{
			Cvar_SetValueQuick (&freelook, 1.f);
		}
		break;

	case OPT_LOOKSTRAFE:
		Cvar_SetValueQuick (&lookstrafe, !lookstrafe.value);
		break;

	case OPT_LOOKSPRING:
		Cvar_SetValueQuick (&lookspring, !lookspring.value);
		break;

	case OPT_LANGUAGE:
		Cbuf_AddText (va ("cycle%s language auto english french german italian spanish\n", dir > 0 ? "" : "back"));
		break;

	case OPT_CONFIRMQUIT:
		M_CycleCvar (&cl_confirmquit, 0, 2, dir);
		break;

	case OPT_STARTDEMOS:
		Cbuf_AddText ("toggle cl_startdemos\n");
		break;

	//
	// Video options
	//
	case OPT_RESOLUTION:
		VID_Menu_ChooseNextResolution (-dir);
		break;
	case OPT_REFRESHRATE:
		VID_Menu_ChooseNextRate (-dir);
		break;
	case OPT_DISPLAYMODE:
		VID_Menu_ChooseNextDisplayMode (-dir);
		break;
	case OPT_VSYNC:
		Cbuf_AddText ("toggle vid_vsync\n"); // kristian
		break;
	case OPT_FSAA:
		VID_Menu_ChooseNextAA (-dir);
		break;
	case OPT_FSAA_MODE:
		Cbuf_AddText ("toggle vid_fsaamode\n");
		break;
	case OPT_RENDERSCALE:
		Cvar_SetValueQuick (&r_scale, CLAMP (1, r_refdef.scale + dir, vid.maxscale));
		break;
	case OPT_ANISO:
		VID_Menu_ChooseNextAnisotropy (-dir);
		break;
	case OPT_TEXFILTER:
		Cbuf_AddText ("cycle gl_texturemode GL_NEAREST_MIPMAP_LINEAR GL_LINEAR_MIPMAP_LINEAR\n");
		break;
	case OPT_ENHANCEDMODELS:
		Cbuf_AddText ("toggle r_enhancedmodels\n");
		break;
	case OPT_ANIMLERP:
		Cvar_SetValueQuick (&r_lerpmodels, !r_lerpmove.value);
		Cvar_SetValueQuick (&r_lerpmove, !r_lerpmove.value);
		break;
	case OPT_PARTICLES:
		Cvar_SetValueQuick (&r_particles, (int)(q_max (r_particles.value, 0.f) + 3 + dir) % 3);
		break;
	case OPT_WATERWARP:
		Cvar_SetValueQuick (&r_waterwarp, (int)(q_max (r_waterwarp.value, 0.f) + 3 + dir) % 3);
		break;
	case OPT_ALPHAMODE:
		VID_Menu_ChooseNextAlphaMode (dir);
		break;
	case OPT_DLIGHTS:
		Cbuf_AddText ("toggle r_dynamic\n");
		break;
	case OPT_SOFTEMU:
		Cvar_SetValueQuick (&r_softemu, (int)(q_max (r_softemu.value, 0.f) + 4 + dir) % 4);
		break;
	case OPT_SOFTEMU_MDL:
		M_CycleCvar (&r_softemu_mdl_warp, -1, 1, dir);
		break;
	case OPT_FPSLIMIT:
		VID_Menu_ChooseNextFPSLimit (-dir);
		break;
	case OPT_SHOWFPS:
		Cbuf_AddText ("toggle scr_showfps\n");
		break;
	case OPT_SHOWSPEED:
		Cbuf_AddText ("toggle scr_showspeed\n");
		break;
	case OPT_SHOWTIME:
		Cbuf_AddText ("toggle scr_clock\n");
		break;
	case OPT_MENUBGSTYLE:
		M_CycleCvar (&scr_menubgstyle, -1, 3, dir);
		break;
	case OPT_MENUBGALPHA:
		Cvar_SetValueQuick (&scr_menubgalpha, CLAMP (0.0f, scr_menubgalpha.value - dir * 0.1f, 1.f));
		break;
	case OPT_CONALPHA:
		Cvar_SetValueQuick (&scr_conalpha, CLAMP (0.0f, scr_conalpha.value - dir * 0.1f, 1.f));
		break;
	case OPT_CONBRIGHTNESS:
		Cvar_SetValueQuick (&scr_conbrightness, CLAMP (0.0f, scr_conbrightness.value - dir * 0.1f, 1.f));
		break;
	case OPT_CENTERPRINTBG:
		M_CycleCvar (&scr_centerprintbg, 0, 3, dir);
		break;

	//
	// Gamepad Options
	//
	case GPAD_OPT_DEVICE:
		// Skip "off" option when using a gamepad to avoid getting into an awkward state
		// on systems where the gamepad is the primary input method, such as the Steam Deck.
		IN_UseNextGamepad (dir, m_lastkey < (int)K_GAMEPAD_BEGIN || m_lastkey >= (int)K_GAMEPAD_END);
		break;
	case GPAD_OPT_SENSX:
		Cvar_SetValueQuick (&joy_sensitivity_yaw, CLAMP (MIN_JOY_SENS, joy_sensitivity_yaw.value + dir * 10.f, MAX_JOY_SENS));
		break;
	case GPAD_OPT_SENSY:
		Cvar_SetValueQuick (&joy_sensitivity_pitch, CLAMP (MIN_JOY_SENS, joy_sensitivity_pitch.value + dir * 10.f, MAX_JOY_SENS));
		break;
	case GPAD_OPT_INVERT:
		Cvar_SetValueQuick (&joy_invert, !joy_invert.value);
		break;
	case GPAD_OPT_SWAP_MOVELOOK:
		Cvar_SetValueQuick (&joy_swapmovelook, !joy_swapmovelook.value);
		break;
	case GPAD_OPT_EXPONENT_LOOK:
		Cvar_SetValueQuick (&joy_exponent, CLAMP (MIN_JOY_EXPONENT, joy_exponent.value + dir * 0.5f, MAX_JOY_EXPONENT));
		break;
	case GPAD_OPT_EXPONENT_MOVE:
		Cvar_SetValueQuick (&joy_exponent_move, CLAMP (MIN_JOY_EXPONENT, joy_exponent_move.value + dir * 0.5f, MAX_JOY_EXPONENT));
		break;
	case GPAD_OPT_DEADZONE_LOOK:
		Cvar_SetValueQuick (&joy_deadzone_look, CLAMP (MIN_STICK_DEADZONE, joy_deadzone_look.value + dir * 0.05f, MAX_STICK_DEADZONE));
		break;
	case GPAD_OPT_DEADZONE_MOVE:
		Cvar_SetValueQuick (&joy_deadzone_move, CLAMP (MIN_STICK_DEADZONE, joy_deadzone_move.value + dir * 0.05f, MAX_STICK_DEADZONE));
		break;
	case GPAD_OPT_DEADZONE_TRIG:
		Cvar_SetValueQuick (&joy_deadzone_trigger, CLAMP (MIN_TRIGGER_DEADZONE, joy_deadzone_trigger.value + dir * 0.05f, MAX_TRIGGER_DEADZONE));
		break;
	case GPAD_OPT_RUMBLE:
		Cvar_SetValueQuick (&joy_rumble, CLAMP (0.f, joy_rumble.value + dir * 0.1f, 1.f));
		break;
	case GPAD_OPT_GYROENABLE:
		Cvar_SetValueQuick (&gyro_enable, !gyro_enable.value);
		break;
	case GPAD_OPT_FLICKSTICK:
		Cvar_SetValueQuick (&joy_flick, !joy_flick.value);
		break;
	case GPAD_OPT_GYROMODE:
		Cvar_SetValueQuick (&gyro_mode, (int)(q_max (gyro_mode.value, 0.f) + GYRO_MODE_COUNT + dir) % GYRO_MODE_COUNT);
		break;
	case GPAD_OPT_GYROAXIS:
		Cvar_SetValueQuick (&gyro_turning_axis, !gyro_turning_axis.value);
		break;
	case GPAD_OPT_GYROSENSX:
		Cvar_SetValueQuick (&gyro_yawsensitivity, CLAMP (MIN_GYRO_SENS, gyro_yawsensitivity.value + dir * .1f, MAX_GYRO_SENS));
		break;
	case GPAD_OPT_GYROSENSY:
		Cvar_SetValueQuick (&gyro_pitchsensitivity, CLAMP (MIN_GYRO_SENS, gyro_pitchsensitivity.value + dir * .1f, MAX_GYRO_SENS));
		break;
	case GPAD_OPT_GYRONOISE:
		Cvar_SetValueQuick (&gyro_noise_thresh, CLAMP (MIN_GYRO_NOISE_THRESH, gyro_noise_thresh.value + dir * .5f, MAX_GYRO_NOISE_THRESH));
		break;
	case GPAD_OPT_CALIBRATE:
		M_Menu_Calibration_f ();
		break;

	default:
		break;
	}

	M_Options_Preview (M_Options_GetSelected ());
}

typedef struct
{
	float		frac;
	char		glyph;
	char		yofs;
} slidermarker_t;

void M_DrawSliderWithMarkers (int x, int y, float range, const slidermarker_t *markers, int nummarkers, const char *desc)
{
	int i;

	range = CLAMP (0.f, range, 1.f);

	M_DrawCharacter (x-8, y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);

	for (i = 0; i < nummarkers; i++)
	{
		const slidermarker_t *marker = &markers[i];
		Draw_CharacterEx (x + (SLIDER_RANGE-1)*8 * CLAMP (0.f, marker->frac, 1.f), y + marker->yofs, CHARSIZE, CHARSIZE, marker->glyph);
	}

	Draw_CharacterEx (x + (SLIDER_RANGE-1)*8 * range, y, CHARSIZE, CHARSIZE, 131);

	i = x + (SLIDER_RANGE+2)*8;
	if (i + 5*8 < glcanvas.right)
		M_Print (i, y, desc);
}

void M_DrawSlider (int x, int y, float range, const char *desc)
{
	M_DrawSliderWithMarkers (x, y, range, NULL, 0, desc);
}

void M_DrawThresholdSlider (int x, int y, float range, qboolean enabled, float live, const char *desc)
{
	slidermarker_t	marker;
	char			buf[6];

	marker.frac = live;
	marker.yofs = 0;
	marker.glyph = 0x8E;

	if (enabled && marker.frac > range)
	{
		marker.glyph ^= 0x80;
		desc = COM_TintString (desc, buf, sizeof (buf));
	}

	M_DrawSliderWithMarkers (x, y, range, &marker, enabled ? 1 : 0, desc);
}

void M_DrawCheckbox (int x, int y, float value)
{
	M_Print (x, y, value ? "On" : "Off");
}

qboolean M_SetSliderValue (int option, float f)
{
	float l;
	f = CLAMP (0.f, f, 1.f);

	M_Options_Preview (option);

	switch (option)
	{
	case OPT_UISCALE:	// console and menu scale
		target_scale_frac = f;
		l = (vid.width / 320.0) - 1;
		f = l > 0 ? f * l + 1 : 0;
		Cvar_SetValue ("scr_conscale", f);
		Cvar_SetValue ("scr_sbarscale", f);
		Cvar_SetValue ("scr_crosshairscale", f);
		// Delay the actual update until we release the mouse button
		// to keep the UI layout stable while adjusting the scale
		if (!slider_grab)
		{
			Cvar_SetValue ("scr_menuscale", f);
			M_Options_UpdateLayout ();
		}
		return true;
	case OPT_HUDLEVEL:	// hud detail
		f = 1 - f;
		f = f * (130 - 100) + 100;
		if (f >= 100)
			f = floor (f / 10 + 0.5) * 10;
		Cvar_SetValue ("viewsize", f);
		return true;
	case OPT_GAMMA:	// gamma
		f = 1.f - f * 0.5f;
		Cvar_SetValue ("gamma", f);
		return true;
	case OPT_CONTRAST:	// contrast
		f += 1.f;
		Cvar_SetValue ("contrast", f);
		return true;
	case OPT_RENDERSCALE:
		f = floor (1 + f * (vid.maxscale - 1) + 0.5);
		Cvar_SetValueQuick (&r_scale, f);
		return true;
	case OPT_ANISO:
		f = Exp2f (floor (f * Log2f (gl_max_anisotropy) + 0.5f));
		Cvar_SetValueQuick (&gl_texture_anisotropy, CLAMP (1, (int)f, (int)gl_max_anisotropy));
		return true;
	case OPT_FSAA:
		f = Exp2f (floor (f * Log2f (framebufs.max_samples) + 0.5f));
		Cvar_SetValueQuick (&vid_fsaa, CLAMP (1, (int)f, framebufs.max_samples));
		return true;
	case OPT_MOUSESPEED:	// mouse speed
		f = f * 10.f + 1.f;
		Cvar_SetValue ("sensitivity", f);
		return true;
	case OPT_SBALPHA:	// statusbar alpha
		f = 1.f - f;
		Cvar_SetValue ("scr_sbaralpha", f);
		return true;
	case OPT_MENUBGALPHA:
		Cvar_SetValueQuick (&scr_menubgalpha, 1.f - f);
		return true;
	case OPT_CONALPHA:
		Cvar_SetValueQuick (&scr_conalpha, 1.f - f);
		return true;
	case OPT_CONBRIGHTNESS:
		Cvar_SetValueQuick (&scr_conbrightness, 1.f - f);
		return true;
	case OPT_MUSICVOL:	// music volume
		Cvar_SetValue ("bgmvolume", f);
		return true;
	case OPT_SNDVOL:	// sfx volume
		Cvar_SetValue ("volume", f);
		return true;
	case OPT_FLASHALPHA:
		Cvar_SetValueQuick (&gl_cshiftpercent, f * 100.f);
		return true;
	case OPT_FOV:	// field of view
		f = LERP (FOV_MIN, FOV_MAX, f);
		if (fabs (f - 90.f) < 5.f)
			f = 90.f;
		Cvar_SetValue ("fov", f);
		return true;
	case OPT_FOVDISTORT:	// FOV distortion
		Cvar_SetValue ("cl_gun_fovscale", 1.f - f);
		return true;
	case GPAD_OPT_SENSX:
		f = LERP (MIN_JOY_SENS, MAX_JOY_SENS, f);
		Cvar_SetValueQuick (&joy_sensitivity_yaw, f);
		return true;
	case GPAD_OPT_SENSY:
		f = LERP (MIN_JOY_SENS, MAX_JOY_SENS, f);
		Cvar_SetValueQuick (&joy_sensitivity_pitch, f);
		return true;
	case GPAD_OPT_EXPONENT_LOOK:
		f = LERP (MIN_JOY_EXPONENT, MAX_JOY_EXPONENT, f);
		Cvar_SetValueQuick (&joy_exponent, f);
		return true;
	case GPAD_OPT_EXPONENT_MOVE:
		f = LERP (MIN_JOY_EXPONENT, MAX_JOY_EXPONENT, f);
		Cvar_SetValueQuick (&joy_exponent_move, f);
		return true;
	case GPAD_OPT_DEADZONE_LOOK:
		f = LERP (MIN_STICK_DEADZONE, MAX_STICK_DEADZONE, f);
		Cvar_SetValueQuick (&joy_deadzone_look, f);
		return true;
	case GPAD_OPT_DEADZONE_MOVE:
		f = LERP (MIN_STICK_DEADZONE, MAX_STICK_DEADZONE, f);
		Cvar_SetValueQuick (&joy_deadzone_move, f);
		return true;
	case GPAD_OPT_DEADZONE_TRIG:
		f = LERP (MIN_TRIGGER_DEADZONE, MAX_TRIGGER_DEADZONE, f);
		Cvar_SetValueQuick (&joy_deadzone_trigger, f);
		return true;
	case GPAD_OPT_RUMBLE:
		Cvar_SetValueQuick (&joy_rumble, f);
		return true;
	case GPAD_OPT_GYROSENSX:
		f = LERP (MIN_GYRO_SENS, MAX_GYRO_SENS, f);
		Cvar_SetValueQuick (&gyro_yawsensitivity, f);
		return true;
	case GPAD_OPT_GYROSENSY:
		f = LERP (MIN_GYRO_SENS, MAX_GYRO_SENS, f);
		Cvar_SetValueQuick (&gyro_pitchsensitivity, f);
		return true;
	case GPAD_OPT_GYRONOISE:
		f = LERP (MIN_GYRO_NOISE_THRESH, MAX_GYRO_NOISE_THRESH, f);
		Cvar_SetValueQuick (&gyro_noise_thresh, f);
		return true;
	default:
		return false;
	}
}

float M_MouseToRawSliderFraction (float cx)
{
	return (cx - 4) / (float)((SLIDER_RANGE - 1) * 8);
}

float M_MouseToSliderFraction (float cx)
{
	float f = M_MouseToRawSliderFraction (cx);
	return CLAMP (0.f, f, 1.f);
}

void M_ReleaseSliderGrab (void)
{
	if (!slider_grab)
		return;
	slider_grab = false;
	M_ThrottledSound ("misc/menu1.wav");
	if (M_Options_GetSelected () == OPT_UISCALE)
		M_SetSliderValue (OPT_UISCALE, target_scale_frac);
}

qboolean M_SliderClick (float cx, float cy)
{
	int item;
	cx -= OPTIONS_MIDPOS;
	if (cx < -12 || cx > SLIDER_RANGE*8+4)
		return false;
	// HACK: we set the flag to true before updating the slider
	// to avoid changing the UI scale and implicitly the layout
	item = M_Options_GetSelected ();
	if (item == OPT_UISCALE)
		slider_grab = true;
	if (!M_SetSliderValue (item, M_MouseToSliderFraction (cx)))
		return false;
	slider_grab = true;
	M_ThrottledSound ("misc/menu3.wav");
	return true;
}

static void M_Options_DrawItem (int y, int item)
{
	char		buf[256];
	const char	*str;
	int			x = OPTIONS_MIDPOS;
	float		r, l;
	qboolean	selected;

	if ((unsigned int) item >= countof (options_names))
		return;

	COM_TintSubstring (options_names[item], optionsmenu.list.search.text, buf, sizeof (buf));
	M_PrintAligned (x - 28, y, ALIGN_RIGHT, buf);
	selected = M_Options_GetSelected () == item;

	switch (item)
	{
	case OPT_VIDEO:
	case OPT_GRAPHICS:
	case OPT_INTERFACE:
	case OPT_GAME:
	case OPT_CUSTOMIZE:
	case OPT_GAMEPAD:
	case OPT_MODS:
	case GPAD_OPT_CALIBRATE:
		M_Print (x - 4, y, "...");
		break;

	case OPT_UISCALE:
		l = (vid.width / 320.0) - 1;
		r = l > 0 ? (scr_conscale.value - 1) / l : 0;
		if (slider_grab && selected)
			r = target_scale_frac;
		M_DrawSlider (x, y, r, va ("%.1f", scr_conscale.value));
		break;

	case OPT_HUDLEVEL:
		if (scr_viewsize.value >= 130.f)
			str = "Photo";
		else if (scr_viewsize.value >= 120.f)
			str = "Off";
		else if (scr_viewsize.value >= 110.f)
			str = "Mini";
		else
			str = "Full";
		r = (scr_viewsize.value - 100) / (130 - 100);
		r = 1 - r;
		M_DrawSlider (x, y, r, str);
		break;

	case OPT_PIXELASPECT:
		M_Print (x, y, vid.guipixelaspect == 1.f ? "Square" : "Stretched");
		break;

	case OPT_CROSSHAIR:
		if (!crosshair.value)
			M_Print (x, y, "Off");
		else
			M_PrintWhite (x, y, va ("%c", crosshair_char));
		break;

	case OPT_UIMOUSE:
		switch (M_Options_GetUIMouse ())
		{
		case UI_MOUSE_OFF:		M_Print (x, y, "Off"); break;
		case UI_MOUSE_QUIET:	M_Print (x, y, "Quiet"); break;
		case UI_MOUSE_NOISY:	M_Print (x, y, "Noisy"); break;
		default:
			break;
		}
		break;

	case OPT_PREVIEW:
		M_DrawCheckbox (x, y, ui_live_preview.value);
		break;

	case OPT_CENTERPRINTBG:
		switch ((int)scr_centerprintbg.value)
		{
		case 1:		str = "Classic Box"; break;
		case 2:		str = "Menu Box"; break;
		case 3:		str = "Menu Strip"; break;
		default:	str = "Off"; break;
		}
		M_Print (x, y, str);
		break;

	case OPT_CONFIRMQUIT:
		if (!cl_confirmquit.value)
			str = "Off";
		else if (cl_confirmquit.value >= 2.f)
			str = "Classic";
		else
			str = "Simple";
		M_Print (x, y, str);
		break;

	case OPT_STARTDEMOS:
		M_DrawCheckbox (x, y, cl_startdemos.value);
		break;

	case OPT_LANGUAGE:
		M_Print (x, y, language.string);
		break;

	case OPT_GAMMA:
		r = (1.0 - vid_gamma.value) / 0.5;
		M_DrawSlider (x, y, r, va ("%.0f", 10.f * r));
		break;

	case OPT_CONTRAST:
		r = vid_contrast.value - 1.0;
		M_DrawSlider (x, y, r, va ("%.0f", 10.f * r));
		break;
	
	case OPT_MOUSESPEED:
		r = (sensitivity.value - 1)/10;
		M_DrawSlider (x, y, r, va ("%.1f", sensitivity.value));
		break;

	case OPT_SBALPHA:
		r = (1.0 - scr_sbaralpha.value) ; // scr_sbaralpha range is 1.0 to 0.0
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.0f * r));
		break;

	case OPT_HUDSTYLE:
		switch (hudstyle)
		{
		case HUD_MODERN_CENTERAMMO:		M_Print (x, y, "Modern 1"); break;
		case HUD_MODERN_SIDEAMMO:		M_Print (x, y, "Modern 2"); break;
		case HUD_QUAKEWORLD:			M_Print (x, y, "QuakeWorld"); break;
		default:
		case HUD_CLASSIC:				M_Print (x, y, "Classic"); break;
		}
		break;

	case OPT_SNDVOL:
		r = sfxvolume.value;
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.f * sfxvolume.value));
		break;

	case OPT_MUSICVOL:
		r = bgmvolume.value;
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.f * bgmvolume.value));
		break;

	case OPT_MUSICEXT:
		M_DrawCheckbox (x, y, bgm_extmusic.value);
		break;

	case OPT_WATERSNDFX:
		M_DrawCheckbox (x, y, snd_waterfx.value);
		break;

	case OPT_ALWAYSRUN:
		if (cl_alwaysrun.value)
			M_Print (x, y, "QuakeSpasm");
		else if (cl_forwardspeed.value > 200.0)
			M_Print (x, y, "Vanilla");
		else
			M_Print (x, y, "Off");
		break;

	case OPT_VIEWBOB:
		M_DrawCheckbox (x, y, cl_bob.value);
		break;

	case OPT_ANGLELIMITS:
		M_Print (x, y, cl_maxpitch.value < 90.f ? "Classic" : "Full");
		break;

	case OPT_RECOIL:
		if ((int)v_gunkick.value == 2)
			M_Print (x, y, "Smooth");
		else if ((int)v_gunkick.value == 1)
			M_Print (x, y, "Classic");
		else
			M_Print (x, y, "Off");
		break;

	case OPT_INVMOUSE:
		M_DrawCheckbox (x, y, m_pitch.value < 0);
		break;

	case OPT_ALWAYSMLOOK:
		M_DrawCheckbox (x, y, (in_mlook.state & 1) || freelook.value);
		break;

	case OPT_LOOKSTRAFE:
		M_DrawCheckbox (x, y, lookstrafe.value);
		break;

	case OPT_LOOKSPRING:
		M_DrawCheckbox (x, y, lookspring.value);
		break;

	case OPT_FLASHALPHA:
		r = gl_cshiftpercent.value / 100.f;
		M_DrawSlider (x, y, r, va ("%.0f%%", gl_cshiftpercent.value));
		break;

	case OPT_FOV:
		r = (scr_fov.value - FOV_MIN) / (FOV_MAX - FOV_MIN);
		M_DrawSlider (x, y, r, va ("%.0f", scr_fov.value));
		break;

	case OPT_FOVDISTORT:
		r = 1.f - cl_gun_fovscale.value;
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.f * r));
		break;

	case OPT_MENUBGALPHA:
		r = 1.f - scr_menubgalpha.value;
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.f * r));
		break;

	case OPT_CONALPHA:
		r = 1.f - scr_conalpha.value;
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.f * r));
		break;

	case OPT_CONBRIGHTNESS:
		r = 1.f - scr_conbrightness.value;
		M_DrawSlider (x, y, r, va ("%.0f%%", 100.f * r));
		break;

	//
	// Video Options
	//
	case OPT_RESOLUTION:
		M_Print (x, y, va("%i x %i", (int)vid_width.value, (int)vid_height.value));
		break;
	case OPT_REFRESHRATE:
		M_Print (x, y, va("%i Hz", (int)vid_refreshrate.value));
		break;
	case OPT_DISPLAYMODE:
		switch (VID_Menu_GetDisplayMode ())
		{
		case DISPLAYMODE_FULLSCREEN:	M_Print (x, y, "Fullscreen"); break;
		case DISPLAYMODE_WINDOWED:		M_Print (x, y, "Windowed"); break;
		case DISPLAYMODE_BORDERLESS:	M_Print (x, y, "Borderless"); break;
		default:						M_Print (x, y, "Other"); break;
		}
		break;
	case OPT_VSYNC:
		M_DrawCheckbox (x, y, (int)vid_vsync.value);
		break;
	case OPT_FSAA:
		r = GetLogFraction (framebufs.scene.samples, 1.f, framebufs.max_samples);
		M_DrawSlider (x, y, r, framebufs.scene.samples >= 2 ? va("%ix", framebufs.scene.samples) : "Off");
		break;
	case OPT_FSAA_MODE:
		M_Print (x, y, vid_fsaamode.value ? "Full" : "Edges only");
		break;
	case OPT_RENDERSCALE:
		r = (r_refdef.scale - 1) / (float) (vid.maxscale - 1);
		M_DrawSlider (x, y, r, r_refdef.scale >= 2 ? va("1/%i", r_refdef.scale) : "Off");
		break;
	case OPT_ANISO:
		r = GetLogFraction (gl_texfilter.anisotropy, 1.f, gl_max_anisotropy);
		M_DrawSlider (x, y, r, gl_texfilter.anisotropy >= 2.f ? va("%ix", (int)gl_texfilter.anisotropy) : "Off");
		break;
	case OPT_TEXFILTER:
		M_Print (x, y, VID_Menu_GetTexFilterDesc ());
		break;
	case OPT_ENHANCEDMODELS:
		M_Print (x, y, r_enhancedmodels.value ? "Enhanced" : "Classic");
		break;
	case OPT_ANIMLERP:
		M_Print (x, y, r_lerpmodels.value ? "Smooth" : "Classic");
		break;
	case OPT_PARTICLES:
		M_Print (x, y, VID_Menu_GetParticlesDesc ());
		break;
	case OPT_WATERWARP:
		M_Print (x, y, VID_Menu_GetWaterWarpDesc ());
		break;
	case OPT_ALPHAMODE:
		M_Print (x, y, VID_Menu_GetAlphaModeDesc ());
		break;
	case OPT_DLIGHTS:
		M_DrawCheckbox (x, y, r_dynamic.value);
		break;
	case OPT_SOFTEMU:
		M_Print (x, y, VID_Menu_GetSoftEmuDesc ());
		break;
	case OPT_SOFTEMU_MDL:
		if (r_softemu_mdl_warp.value < 0.f)
			str = "Auto";
		else
			str = r_softemu_mdl_warp.value ? "On" : "Off";
		M_Print (x, y, str);
		break;
	case OPT_FPSLIMIT:
		M_Print (x, y, host_maxfps.value ? va("%i", (int)host_maxfps.value): "Off");
		break;
	case OPT_SHOWFPS:
		M_DrawCheckbox (x, y, scr_showfps.value);
		break;
	case OPT_SHOWSPEED:
		M_DrawCheckbox (x, y, scr_showspeed.value);
		break;
	case OPT_SHOWTIME:
		M_DrawCheckbox (x, y, scr_clock.value);
		break;
	case OPT_MENUBGSTYLE:
		if (scr_menubgstyle.value < 0.f)
			str = "Auto";
		else if ((int)scr_menubgstyle.value == 0)
			str = "GLQuake";
		else if ((int)scr_menubgstyle.value == 1)
			str = "WinQuake 1";
		else if ((int)scr_menubgstyle.value == 2)
			str = "WinQuake 2";
		else
			str = "DOSQuake";
		M_Print (x, y, str);
		break;

	//
	// Gamepad Options
	//
	case GPAD_OPT_DEVICE:
		str = IN_GetGamepadName ();
		if (!str)
			str = "None";
		M_PrintWordWrap (x, y, str, 320 - x, 16, true); // note: hijacking the empty line below this option
		break;
	case GPAD_OPT_SENSX:
		r = (joy_sensitivity_yaw.value - MIN_JOY_SENS) / (MAX_JOY_SENS - MIN_JOY_SENS);
		M_DrawSlider (x, y, r, va ("%.0f", joy_sensitivity_yaw.value));
		break;
	case GPAD_OPT_SENSY:
		r = (joy_sensitivity_pitch.value - MIN_JOY_SENS) / (MAX_JOY_SENS - MIN_JOY_SENS);
		M_DrawSlider (x, y, r, va ("%.0f", joy_sensitivity_pitch.value));
		break;
	case GPAD_OPT_INVERT:
		M_DrawCheckbox (x, y, joy_invert.value);
		break;
	case GPAD_OPT_SWAP_MOVELOOK:
		M_Print (x, y, joy_swapmovelook.value ? "Left" : "Right");
		break;
	case GPAD_OPT_EXPONENT_LOOK:
		r = (joy_exponent.value - MIN_JOY_EXPONENT) / (MAX_JOY_EXPONENT - MIN_JOY_EXPONENT);
		M_DrawSlider (x, y, r, va ("%.1f", joy_exponent.value));
		break;
	case GPAD_OPT_EXPONENT_MOVE:
		r = (joy_exponent_move.value - MIN_JOY_EXPONENT) / (MAX_JOY_EXPONENT - MIN_JOY_EXPONENT);
		M_DrawSlider (x, y, r, va ("%.1f", joy_exponent_move.value));
		break;
	case GPAD_OPT_DEADZONE_LOOK:
		r = (joy_deadzone_look.value - MIN_STICK_DEADZONE) / (MAX_STICK_DEADZONE - MIN_STICK_DEADZONE);
		l = (IN_GetRawLookMagnitude () - MIN_STICK_DEADZONE) / (MAX_STICK_DEADZONE - MIN_STICK_DEADZONE);
		M_DrawThresholdSlider (x, y, r, selected && IN_HasGamepad (), l, va ("%.0f%%", joy_deadzone_look.value * 100.f));
		break;
	case GPAD_OPT_DEADZONE_MOVE:
		r = (joy_deadzone_move.value - MIN_STICK_DEADZONE) / (MAX_STICK_DEADZONE - MIN_STICK_DEADZONE);
		l = (IN_GetRawMoveMagnitude () - MIN_STICK_DEADZONE) / (MAX_STICK_DEADZONE - MIN_STICK_DEADZONE);
		M_DrawThresholdSlider (x, y, r, selected && IN_HasGamepad (), l, va ("%.0f%%", joy_deadzone_move.value * 100.f));
		break;
	case GPAD_OPT_DEADZONE_TRIG:
		r = (joy_deadzone_trigger.value - MIN_TRIGGER_DEADZONE) / (MAX_TRIGGER_DEADZONE - MIN_TRIGGER_DEADZONE);
		l = (IN_GetRawTriggerMagnitude () - MIN_TRIGGER_DEADZONE) / (MAX_TRIGGER_DEADZONE - MIN_TRIGGER_DEADZONE);
		M_DrawThresholdSlider (x, y, r, selected && IN_HasGamepad (), l, va ("%.0f%%", joy_deadzone_trigger.value * 100.f));
		break;
	case GPAD_OPT_RUMBLE:
		r = joy_rumble.value;
		if (!IN_HasRumble ())
			M_Print (x, y, "Unavailable");
		else
			M_DrawSlider (x, y, r, va ("%.0f%%", joy_rumble.value * 100.f));
		break;
	case GPAD_OPT_GYROENABLE:
		if (!IN_HasGyro ())
			M_Print (x, y, "Unavailable");
		else
			M_DrawCheckbox (x, y, gyro_enable.value);
		break;
	case GPAD_OPT_FLICKSTICK:
		M_DrawCheckbox (x, y, joy_flick.value);
		break;
	case GPAD_OPT_GYROMODE:
		switch ((int)gyro_mode.value)
		{
		case GYRO_BUTTON_ENABLES:		M_Print (x, y, "Enable gyro"); break;
		case GYRO_BUTTON_DISABLES:		M_Print (x, y, "Disable gyro"); break;
		case GYRO_BUTTON_INVERTS_DIR:	M_Print (x, y, "Invert dir"); break;
		default:						M_Print (x, y, "Ignore"); break;
		}
		break;
	case GPAD_OPT_GYROAXIS:
		M_Print(x, y, gyro_turning_axis.value ? "roll (lean)" : "yaw (turn)");
		break;
	case GPAD_OPT_GYROSENSX:
		r = (gyro_yawsensitivity.value - MIN_GYRO_SENS) / (MAX_GYRO_SENS - MIN_GYRO_SENS);
		M_DrawSlider (x, y, r, va ("%.1f", gyro_yawsensitivity.value));
		break;
	case GPAD_OPT_GYROSENSY:
		r = (gyro_pitchsensitivity.value - MIN_GYRO_SENS) / (MAX_GYRO_SENS - MIN_GYRO_SENS);
		M_DrawSlider (x, y, r, va ("%.1f", gyro_pitchsensitivity.value));
		break;
	case GPAD_OPT_GYRONOISE:
		r = (gyro_noise_thresh.value - MIN_GYRO_NOISE_THRESH) / (MAX_GYRO_NOISE_THRESH - MIN_GYRO_NOISE_THRESH);
		l = (IN_GetRawGyroMagnitude () - MIN_GYRO_NOISE_THRESH) / (MAX_GYRO_NOISE_THRESH - MIN_GYRO_NOISE_THRESH);
		M_DrawThresholdSlider (x, y, r, selected && IN_HasGyro () && gyro_enable.value, l, va ("%.1f", gyro_noise_thresh.value));
		break;

	default:
		break;
	}
}

/*
================
M_Options_UpdatePreview
================
*/
static void M_Options_UpdatePreview (void)
{
	if (optionsmenu.preview.frac != optionsmenu.preview.frac_target)
	{
		if (optionsmenu.preview.frac < optionsmenu.preview.frac_target)
		{
			optionsmenu.preview.frac += host_rawframetime / PREVIEW_FADEOUT_TIME;
			optionsmenu.preview.frac = q_min (optionsmenu.preview.frac, optionsmenu.preview.frac_target);
		}
		else
		{
			optionsmenu.preview.frac -= host_rawframetime / PREVIEW_FADEIN_TIME;
			optionsmenu.preview.frac = q_max (optionsmenu.preview.frac, optionsmenu.preview.frac_target);
			if (optionsmenu.preview.frac == optionsmenu.preview.frac_target)
			{
				if (M_Options_ForcedCenterPrint ())
					SCR_CenterPrint ("");
				optionsmenu.preview.id = -1;
			}
		}
	}
	else if (optionsmenu.preview.hold_time > 0.f && !slider_grab)
	{
		optionsmenu.preview.hold_time -= host_rawframetime;
		if (optionsmenu.preview.hold_time <= 0.f)
		{
			optionsmenu.preview.hold_time = 0.f;
			optionsmenu.preview.frac_target = 0.f;
		}
	}
}

/*
================
M_Options_DrawFadeScreen
================
*/
static void M_Options_DrawFadeScreen (void)
{
	int y, firstvis, numvis;
	float frac, y0, y1;

	GL_SetCanvas (CANVAS_MENU);
	y0 = glcanvas.top;
	y1 = glcanvas.bottom;

	M_List_GetVisibleRange (&optionsmenu.list, &firstvis, &numvis);
	firstvis += optionsmenu.first_item;
	if ((unsigned int)(optionsmenu.preview.id - firstvis) < (unsigned int)numvis)
	{
		y = optionsmenu.y + OPTIONS_LISTOFS + optionsmenu.yofs;
		y += (optionsmenu.preview.id - firstvis) * CHARSIZE + CHARSIZE / 2;
		frac = EaseInOut (optionsmenu.preview.frac);
		y0 = LERP (y0, y - CHARSIZE, frac);
		y1 = LERP (y1, y + CHARSIZE, frac);
	}

	Draw_PartialFadeScreen (glcanvas.left, glcanvas.right, y0, y1, 1.f);
}

void M_Options_Draw (void)
{
	qboolean enabled, wasenabled, selected, isolated;
	int firstvis, numvis;
	int i, x, y, cols;
	int option;
	float alpha;
	qpic_t	*p;

	if (slider_grab && !keydown[K_MOUSE1])
		M_ReleaseSliderGrab ();

	M_Options_UpdateLayout ();
	M_List_Update (&optionsmenu.list);
	
	if (*optionsmenu.last_cursor != optionsmenu.list.cursor)
	{
		*optionsmenu.last_cursor = optionsmenu.list.cursor;
		M_Options_Preview (-1);
	}

	M_Options_UpdatePreview ();

	x = 56;
	y = optionsmenu.y;
	cols = 32;

	alpha = 1.f - optionsmenu.preview.frac;
	alpha *= alpha;
	GL_PushCanvasColor (1.f, 1.f, 1.f, alpha);

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, y + 4, p);

	if (optionsmenu.subtitle && optionsmenu.subtitle[0])
		M_PrintWhite ((320-8*strlen(optionsmenu.subtitle))/2, y + 32, optionsmenu.subtitle);

	y += OPTIONS_LISTOFS + optionsmenu.yofs;

	if (M_List_GetOverflow (&optionsmenu.list) > 0)
	{
		if (optionsmenu.list.scroll > 0)
			M_DrawEllipsisBar (x, y - 8, cols);
		if (optionsmenu.list.scroll + optionsmenu.list.viewsize < optionsmenu.list.numitems)
			M_DrawEllipsisBar (x, y + optionsmenu.list.viewsize*8, cols);
	}

	wasenabled = true;
	M_List_GetVisibleRange (&optionsmenu.list, &firstvis, &numvis);
	while (numvis-- > 0)
	{
		i = firstvis++;
		option = optionsmenu.first_item + i;
		enabled = M_Options_IsEnabled (i);
		selected = (i == optionsmenu.list.cursor);
		isolated = (option == optionsmenu.preview.id && enabled);

		if (enabled != wasenabled)
		{
			float val = enabled ? 1.f : 0.375f;
			GL_SetCanvasColor (val, val, val, alpha);
			wasenabled = enabled;
		}

		if (isolated)
			GL_PushCanvasColor (1.f, 1.f, 1.f, 1.f);

		M_Options_DrawItem (y, option);

		// cursor
		if (selected)
			M_DrawArrowCursor (OPTIONS_MIDPOS - 20, y);

		if (isolated)
			GL_PopCanvasColor ();

		y += 8;
	}

	GL_PopCanvasColor ();

	if (M_ForcedQuitMessage (&alpha))
	{
		GL_PushCanvasColor (1.f, 1.f, 1.f, alpha);
		M_DrawQuitMessage ();
		GL_PopCanvasColor ();
	}
}

void M_Options_Key (int k)
{
	if (!keydown[K_MOUSE1])
		M_ReleaseSliderGrab ();

	if (slider_grab)
	{
		switch (k)
		{
		case K_ESCAPE:
		case K_BBUTTON:
		case K_MOUSE4:
		case K_MOUSE2:
			M_ReleaseSliderGrab ();
			break;
		}
		return;
	}

	if (M_List_Key (&optionsmenu.list, k))
		return;

	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		if (optionsmenu.preview.frac != 0.f)
		{
			M_Options_ResetPreview ();
			return;
		}
		if (m_state == m_video)
			VID_SyncCvars (); //sync cvars before leaving menu. FIXME: there are other ways to leave menu
		if (m_state == m_options)
			M_Menu_Main_f ();
		else
			M_Menu_Options_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	enter:
		m_entersound = true;
		M_List_ClearSearch (&optionsmenu.list);
		switch (M_Options_GetSelected ())
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case OPT_DEFAULTS:
			if (SCR_ModalMessage("This will reset all controls\n"
					"and stored cvars. Continue? (y/n)\n", 15.0f))
			{
				Cbuf_AddText ("resetcfg\n");
				Cbuf_AddText ("exec default.cfg\n");
			}
			break;
		case OPT_MODS:
			M_Menu_Mods_f ();
			break;
		case OPT_VIDEO:
			M_Menu_Video_f ();
			break;
		case OPT_GAMEPAD:
			M_Menu_Gamepad_f ();
			break;
		case OPT_GAME:
			M_Options_Init (m_game);
			break;
		case OPT_GRAPHICS:
			M_Options_Init (m_graphics);
			break;
		case OPT_INTERFACE:
			M_Options_Init (m_interface);
			break;

		case OPT_TEST_VIDEO:
			Cbuf_AddText ("vid_test\n");
			break;
		case OPT_APPLY_VIDEO:
			Cbuf_AddText ("vid_restart\n");
			key_dest = key_game;
			m_state = m_none;
			IN_Activate();
			break;

		default:
			M_AdjustSliders (1);
			break;
		}
		return;

	case K_LEFTARROW:
	//case K_MOUSE2:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;

	case K_MOUSE1:
		M_List_ClearSearch (&optionsmenu.list);
		if (!M_SliderClick (m_mousex, m_mousey))
			goto enter;
		break;
	}
}

textmode_t M_Options_TextEntry (void)
{
	return slider_grab ? TEXTMODE_OFF : TEXTMODE_NOPOPUP;
}

void M_Options_Char (int key)
{
	M_List_Char (&optionsmenu.list, key);
}

void M_Options_Mousemove (float cx, float cy)
{
	if (slider_grab)
	{
		float frac;
		if (!keydown[K_MOUSE1])
		{
			M_ReleaseSliderGrab ();
			return;
		}
		frac = M_MouseToRawSliderFraction (cx - OPTIONS_MIDPOS);
		M_SetSliderValue (M_Options_GetSelected (), frac);
		if (frac >= 0.f && frac <= 1.f)
			M_MouseSound ("misc/menu1.wav");
		return;
	}

	M_List_Mousemove (&optionsmenu.list, cy - optionsmenu.y - OPTIONS_LISTOFS - optionsmenu.yofs);
}

//=============================================================================
/* KEYS MENU */

typedef struct
{
	const char			*command;
	const char			*description;
	keydevicemask_t		devicemask;
} menukeybind_t;

#define QUICKSAVE "echo Quicksaving...; wait; save quick"
#define QUICKLOAD "echo Quickloading...; wait; load quick"

static const menukeybind_t menubinds[] =
{
	{"+forward",		"Move forward",			KDM_KEYBOARD_AND_MOUSE},
	{"+back",			"Move backward",		KDM_KEYBOARD_AND_MOUSE},
	{"+moveleft",		"Move left",			KDM_KEYBOARD_AND_MOUSE},
	{"+moveright",		"Move right",			KDM_KEYBOARD_AND_MOUSE},
	{"+jump",			"Jump / swim up",		KDM_ANY},
	{"+moveup",			"Swim up",				KDM_ANY},
	{"+movedown",		"Swim down",			KDM_ANY},
	{"+speed",			"Run",					KDM_KEYBOARD_AND_MOUSE},
	{"+strafe",			"Sidestep",				KDM_KEYBOARD_AND_MOUSE},
	{"",				"",						KDM_ANY},
	{"+left",			"Turn left",			KDM_KEYBOARD_AND_MOUSE},
	{"+right",			"Turn right",			KDM_KEYBOARD_AND_MOUSE},
	{"+lookup",			"Look up",				KDM_KEYBOARD_AND_MOUSE},
	{"+lookdown",		"Look down",			KDM_KEYBOARD_AND_MOUSE},
	{"centerview",		"Center view",			KDM_ANY},
	{"zoom_in",			"Toggle zoom",			KDM_ANY},
	{"+zoom",			"Quick zoom",			KDM_ANY},
	{"+gyroaction",		"Gyro switch",			KDM_GAMEPAD},
	{"",				"",						KDM_ANY},
	{"+attack",			"Attack",				KDM_ANY},
	{"impulse 10",		"Next weapon",			KDM_ANY},
	{"impulse 12",		"Previous weapon",		KDM_ANY},
	{"impulse 1",		"Axe",					KDM_ANY},
	{"impulse 2",		"Shotgun",				KDM_ANY},
	{"impulse 3",		"Super Shotgun",		KDM_ANY},
	{"impulse 4",		"Nailgun",				KDM_ANY},
	{"impulse 5",		"Super Nailgun",		KDM_ANY},
	{"impulse 6",		"Grenade Launcher",		KDM_ANY},
	{"impulse 7",		"Rocket Launcher",		KDM_ANY},
	{"impulse 8",		"Thunderbolt",			KDM_ANY},
	{"impulse 225",		"Laser Cannon",			KDM_ANY},
	{"impulse 226",		"Mjolnir",				KDM_ANY},
	{"",				"",						KDM_ANY},
	{QUICKSAVE,			"Quick save",			KDM_ANY},
	{QUICKLOAD,			"Quick load",			KDM_ANY},
	{"menu_load",		"Load menu",			KDM_ANY},
	{"menu_save",		"Save menu",			KDM_ANY},
	{"menu_maps",		"Maps menu",			KDM_ANY},
	{"menu_options",	"Options menu",			KDM_ANY},
	{"screenshot",		"Screenshot",			KDM_ANY},
	{"+showscores",		"Show score",			KDM_ANY},
};

#define	NUMCOMMANDS		Q_COUNTOF(menubinds)
#define KEYLIST_TOP		56						// title plaque, tabs, scroll ellipsis bar
#define KEYLIST_BOTTOM	24						// scroll ellipsis bar, search box, key hint

static struct
{
	menulist_t			list;
	keydevicemask_t		devicemask;
	int					y;
	menukeybind_t		*items;
} keysmenu;

static qboolean	bind_grab;

static void M_Keys_UpdateLayout (void)
{
	int height;

	M_UpdateBounds ();

	// Note: we use NUMCOMMANDS instead of keysmenu.list.numitems to have a stable layout
	// when switching between keyboard+mouse/gamepad tabs (different number of items)
	height = NUMCOMMANDS * 8 + KEYLIST_TOP + KEYLIST_BOTTOM;
	height = q_min (height, m_height);
	keysmenu.y = m_top + (((m_height - height) / 2) & ~7);
	keysmenu.list.viewsize = (height - KEYLIST_TOP - KEYLIST_BOTTOM) / 8;
}

static qboolean M_Keys_IsSelectable (int index)
{
	return keysmenu.items[index].command[0] != '\0';
}

static qboolean M_Keys_Match (int index)
{
	const char *name = keysmenu.items[index].description;
	if (!*name)
		return false;
	return q_strcasestr (name, keysmenu.list.search.text) != NULL;
}

static void M_Keys_Populate (void)
{
	int i;

	VEC_CLEAR (keysmenu.items);

	for (i = 0; i < NUMCOMMANDS; i++)
	{
		// filter item by device type
		if (!(keysmenu.devicemask & menubinds[i].devicemask))
			continue;

		if (!hipnotic && (strcmp (menubinds[i].command, "impulse 225") == 0 || strcmp (menubinds[i].command, "impulse 226") == 0))
			continue;

		// if we have two separators in a row, overwrite the old one
		if (VEC_SIZE (keysmenu.items) > 0 && !menubinds[i].command[0] && !VEC_LAST(keysmenu.items).command[0])
			VEC_LAST(keysmenu.items) = menubinds[i];
		else // otherwise add a new item
			VEC_PUSH (keysmenu.items, menubinds[i]);
	}

	keysmenu.list.numitems = (int) VEC_SIZE (keysmenu.items);
	keysmenu.list.cursor = 0;
	keysmenu.list.scroll = 0;
}

void M_Menu_Keys_f (void)
{
	keydevice_t lastactive = IN_GetLastActiveDeviceType ();

	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
	keysmenu.list.isactive_fn = M_Keys_IsSelectable;
	keysmenu.list.search.match_fn = M_Keys_Match;
	keysmenu.devicemask = (lastactive == KD_GAMEPAD) ? KDM_GAMEPAD : KDM_KEYBOARD_AND_MOUSE;

	M_Keys_Populate ();
	M_List_ClearSearch (&keysmenu.list);

	M_Keys_UpdateLayout ();
}


void M_FindKeysForCommand (const char *command, int *threekeys)
{
	Key_GetKeysForCommand (command, threekeys, 3, keysmenu.devicemask);
}

void M_UnbindCommand (const char *command)
{
	int		j;
	char	*b;

	for (j = 0; j < MAX_KEYS; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strcmp (b, command) )
			Key_SetBinding (j, NULL);
	}
}

extern qpic_t	*pic_up, *pic_down;

void M_Keys_Draw (void)
{
	int			firstvis, numvis;
	int			i, j, x, y, cols;
	int			keys[3];
	const char	*name;
	const char	*hint;
	qpic_t		*p;

	M_Keys_UpdateLayout ();
	M_List_Update (&keysmenu.list);

	x = 0;
	y = keysmenu.y;
	cols = 40;

	// draw title plaque
	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPic ( (320-p->width)/2, y + 4, p);

	// draw tabs
	M_PrintAlignedEx (320*1/4, y + 32, ALIGN_CENTER, 8, keysmenu.devicemask == KDM_GAMEPAD, "Keyboard & Mouse");
	M_PrintAlignedEx (320*3/4, y + 32, ALIGN_CENTER, 8, keysmenu.devicemask != KDM_GAMEPAD, "Gamepad");
	i = keysmenu.devicemask == KDM_GAMEPAD ? 3 : 1;
	M_DrawQuakeBar (320*i/4 - 20*4, y + 40, 20);

	y += KEYLIST_TOP;

	if (M_List_GetOverflow (&keysmenu.list) > 0)
	{
		if (keysmenu.list.scroll > 0)
			M_DrawEllipsisBar (x, y - 8, cols);
		if (keysmenu.list.scroll + keysmenu.list.viewsize < keysmenu.list.numitems)
			M_DrawEllipsisBar (x, y + keysmenu.list.viewsize*8, cols);
	}

	// search for known bindings
	M_List_GetVisibleRange (&keysmenu.list, &firstvis, &numvis);
	while (numvis-- > 0)
	{
		i = firstvis++;

		if (keysmenu.items[i].command[0])
		{
			char buf[64];
			qboolean active = (i == keysmenu.list.cursor && bind_grab);
			void (*print_fn) (int cx, int cy, const char *text) =
				active ? M_PrintWhite : M_Print;

			COM_TintSubstring (keysmenu.items[i].description, keysmenu.list.search.text, buf, sizeof (buf));
			M_PrintDotFill (0, y, buf, 17, !active);

			M_FindKeysForCommand (keysmenu.items[i].command, keys);
			// If we already have 3 keys bound to this action
			// they will all be unbound when a new one is assigned.
			// We show this outcome to the user before it actually
			// occurs so they have a chance to abort the process
			// and keep the existing key bindings.
			if (i == keysmenu.list.cursor && bind_grab && keys[2] != -1)
				keys[0] = -1;

			for (j = 0, x = 136; j < 3 && keys[j] != -1; j++)
			{
				if (j)
				{
					GL_SetCanvasColor (1.f, 1.f, 1.f, 0.375f);
					print_fn (x, y, ",");
					GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);
					x += 16;
				}
				name = Key_KeynumToFriendlyString (keys[j]);
				print_fn (x, y, name);
				x += strlen (name) * 8;
			}

			if (j == 0)
			{
				if (!active)
					GL_SetCanvasColor (1.f, 1.f, 1.f, 0.375f);
				print_fn (x, y, "???");
				GL_SetCanvasColor (1.f, 1.f, 1.f, 1.f);
			}
		}

		if (i == keysmenu.list.cursor)
		{
			if (bind_grab)
				M_DrawCharacter (128, y, '=');
			else
				M_DrawArrowCursor (128, y);
		}

		y += 8;
	}

	// draw search box
	M_List_DrawSearch (&keysmenu.list, 0, y + 4, 16);

	// show hint at the bottom
	if (bind_grab)
		hint = keysmenu.devicemask == KDM_GAMEPAD ?
			va ("Press new button, or %s to cancel", Key_KeynumToFriendlyString (K_START)) :
			va ("Press new key, or %s to cancel", Key_KeynumToFriendlyString (K_ESCAPE)) ;
	else
		hint = IN_GetLastActiveDeviceType () == KD_GAMEPAD ?
			va ("%s = change, %s = clear", Key_KeynumToFriendlyString (K_ABUTTON), Key_KeynumToFriendlyString (K_YBUTTON)):
			va ("%s = change, %s = clear", Key_KeynumToFriendlyString (K_ENTER), Key_KeynumToFriendlyString (K_BACKSPACE));
	M_PrintAligned (160, y + 16, ALIGN_CENTER, hint);
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[3];

	if (bind_grab) // defining a key
	{
		M_ThrottledSound ("misc/menu1.wav");

		if ((k != K_ESCAPE) && (k != '`'))
		{
			const char *command;

			// wrong key type?
			if (!(Key_GetDeviceMaskForKeynum (k) & keysmenu.devicemask))
				return;

			command = keysmenu.items[keysmenu.list.cursor].command;
			M_FindKeysForCommand (command, keys);
			if (keys[2] != -1)
				M_UnbindCommand (command);
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), command);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		IN_DeactivateForMenu(); // deactivate because we're returning to the menu
		return;
	}

	if (M_List_Key (&keysmenu.list, k))
		return;

	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Options_f ();
		break;

	case K_TAB:
	case K_LSHOULDER:
	case K_RSHOULDER:
		M_ThrottledSound ("misc/menu1.wav");
		keysmenu.devicemask ^= KDM_GAMEPAD | KDM_KEYBOARD_AND_MOUSE;
		M_Keys_Populate ();
		break;

	case K_ENTER:		// go into bind mode
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		M_List_ClearSearch (&keysmenu.list);
		M_ThrottledSound ("misc/menu2.wav");
		bind_grab = true;
		M_List_AutoScroll (&keysmenu.list);
		IN_Activate(); // activate to allow mouse key binding
		break;

	case K_BACKSPACE:	// delete bindings
		if (keysmenu.list.search.backspacecooldown)
			break;
		/* fall-through */
	case K_DEL:
	case K_YBUTTON:
		M_ThrottledSound ("misc/menu2.wav");
		M_UnbindCommand (keysmenu.items[keysmenu.list.cursor].command);
		break;
	}
}


void M_Keys_Mousemove (float cx, float cy)
{
	M_List_Mousemove (&keysmenu.list, cy - keysmenu.y - KEYLIST_TOP);
}

textmode_t M_Keys_TextEntry (void)
{
	return bind_grab ? TEXTMODE_OFF : TEXTMODE_NOPOPUP;
}

void M_Keys_Char (int key)
{
	M_List_Char (&keysmenu.list, key);
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/help%i.lmp", help_page)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Main_f ();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
	case K_MWHEELDOWN:
	case K_MOUSE1:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
	case K_MWHEELUP:
	//case K_MOUSE2:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
enum m_state_e	m_quit_prevstate;
qboolean	wasInMenus;

const char*const quitMessage [] = 
{
/* .........1.........2.... */
  "  Are you gonna quit    ",
  "  this game just like   ",
  "   everything else?     ",
  "                        ",
 
  " Milord, methinks that  ",
  "   thou art a lowly     ",
  " quitter. Is this true? ",
  "                        ",

  " Do I need to bust your ",
  "  face open for trying  ",
  "        to quit?        ",
  "                        ",

  " Man, I oughta smack you",
  "   for trying to quit!  ",
  "     Press Y to get     ",
  "      smacked out.      ",
 
  " Press Y to quit like a ",
  "   big loser in life.   ",
  "  Press N to stay proud ",
  "    and successful!     ",
 
  "   If you press Y to    ",
  "  quit, I will summon   ",
  "  Satan all over your   ",
  "      hard drive!       ",
 
  "  Um, Asmodeus dislikes ",
  " his children trying to ",
  " quit. Press Y to return",
  "   to your Tinkertoys.  ",
 
  "  If you quit now, I'll ",
  "  throw a blanket-party ",
  "   for you next time!   ",
  "                        ",

  "  Leave QUAKE?  ",
  "",
  "",
  "\xD9\x65s   \xCE\x6F",
};

void M_ChooseQuitMessage (void)
{
	msgNumber = (cl_confirmquit.value >= 2.f) ? (int)(realtime*(5.0*1.61803399))&7 : 8;
}

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	M_ChooseQuitMessage ();
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			IN_Activate();
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case K_ABUTTON:
		IN_DeactivateForConsole();
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}
}


void M_Quit_Char (int key)
{
	switch (key)
	{
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			IN_Activate();
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case 'y':
	case 'Y':
		IN_DeactivateForConsole();
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}

}


textmode_t M_Quit_TextEntry (void)
{
	return TEXTMODE_NOPOPUP;
}

qboolean M_ForcedQuitMessage (float *alpha)
{
	qboolean forced = (key_dest == key_menu && M_GetBaseState (m_state) == m_options) ? optionsmenu.preview.id == OPT_CONFIRMQUIT : false;
	if (alpha)
		*alpha = forced ? M_Options_PreviewAlpha () : 0.f;
	return forced;
}

void M_DrawQuitMessage (void)
{
	const char*const *msg = quitMessage + msgNumber*4;
	int i, boxlen = 0;

	if (!cl_confirmquit.value)
		return;

	//okay, this is kind of fucked up.  M_DrawTextBox will always act as if
	//width is even. Also, the width and lines values are for the interior of the box,
	//but the x and y values include the border.
	for (i = 0; i < 4; i++)
	{
		int msglen = (int)strlen (msg[i]);
		boxlen = q_max (boxlen, msglen);
	}
	boxlen = (boxlen + 1) & ~1;
	M_DrawTextBox (160 - 4*(boxlen+2), 76, boxlen, 5);

	//now do the text
	for (i = 0; i < 4; i++)
		M_Print (160 - 8*((strlen(msg[i])+1)>>1), 88 + i*8, msg[i]);
}


void M_Quit_Draw (void) //johnfitz -- modified for new quit message
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	M_DrawQuitMessage ();
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {72, 92, 124};
#define NUM_LANCONFIG_CMDS	3

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_Menu_LanConfig_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor == 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	const char	*startJoin;
	const char	*protocol;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

	basex = 72; /* Arcane Dimensions has an oversized gfx/p_multi.lmp */

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (IPXConfig)
		protocol = "IPX";
	else
		protocol = "TCP/IP";
	M_Print (basex, 32, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 52, "Address:");
	if (IPXConfig)
		M_Print (basex+9*8, 52, my_ipx_address);
	else
		M_Print (basex+9*8, 52, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Search for local games...");
		M_Print (basex, 108, "Join game at:");
		M_DrawTextBox (basex+8, lanConfig_cursor_table[2]-8, 22, 1);
		M_Print (basex+16, lanConfig_cursor_table[2], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawArrowCursor (basex-8, lanConfig_cursor_table [lanConfig_cursor]);

	if (lanConfig_cursor == 0)
		M_DrawTextCursor (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0]);

	if (lanConfig_cursor == 2)
		M_DrawTextCursor (basex+16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [2]);

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem ();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 2)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			IN_Activate();
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 2)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	l =  Q_atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}


void M_LanConfig_Char (int key)
{
	int l;

	switch (lanConfig_cursor)
	{
	case 0:
		if (key < '0' || key > '9')
			return;
		l = strlen(lanConfig_portname);
		if (l < 5)
		{
			lanConfig_portname[l+1] = 0;
			lanConfig_portname[l] = key;
		}
		break;
	case 2:
		l = strlen(lanConfig_joinname);
		if (l < 21)
		{
			lanConfig_joinname[l+1] = 0;
			lanConfig_joinname[l] = key;
		}
		break;
	}
}


textmode_t M_LanConfig_TextEntry (void)
{
	return (lanConfig_cursor == 0 || lanConfig_cursor == 2) ? TEXTMODE_ON : TEXTMODE_OFF;
}


void M_LanConfig_Mousemove (float cx, float cy)
{
	int prev = lanConfig_cursor;
	M_UpdateCursorWithTable (cy, lanConfig_cursor_table, NUM_LANCONFIG_CMDS - StartingGame, &lanConfig_cursor);
	if (lanConfig_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	const char	*name;
	const char	*description;
} level_t;

level_t		levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
	{"start", "Command HQ"},	// 0

	{"hip1m1", "The Pumping Station"},			// 1
	{"hip1m2", "Storage Facility"},
	{"hip1m3", "The Lost Mine"},
	{"hip1m4", "Research Facility"},
	{"hip1m5", "Military Complex"},

	{"hip2m1", "Ancient Realms"},				// 6
	{"hip2m2", "The Black Cathedral"},
	{"hip2m3", "The Catacombs"},
	{"hip2m4", "The Crypt"},
	{"hip2m5", "Mortum's Keep"},
	{"hip2m6", "The Gremlin's Domain"},

	{"hip3m1", "Tur Torment"},				// 12
	{"hip3m2", "Pandemonium"},
	{"hip3m3", "Limbo"},
	{"hip3m4", "The Gauntlet"},

	{"hipend", "Armagon's Lair"},				// 16

	{"hipdm1", "The Edge of Oblivion"}			// 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

typedef struct
{
	const char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] =
{
	{"Scourge of Armagon", 0, 1},
	{"Fortress of the Dead", 1, 5},
	{"Dominion of Darkness", 6, 6},
	{"The Rift", 12, 4},
	{"Final Level", 16, 1},
	{"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

void M_Menu_GameOptions_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 112, 120};
#define	NUM_GAMEOPTIONS	9
int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	qpic_t	*p;
	int		x;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_DrawTextBox (152, 32, 10, 1);
	M_Print (160, 40, "begin game");

	M_Print (0, 56, "      Max players");
	M_Print (160, 56, va("%i", maxplayers) );

	M_Print (0, 64, "        Game Type");
	if (coop.value)
		M_Print (160, 64, "Cooperative");
	else
		M_Print (160, 64, "Deathmatch");

	M_Print (0, 72, "        Teamplay");
	if (rogue)
	{
		const char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, 72, msg);
	}
	else
	{
		const char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, 72, msg);
	}

	M_Print (0, 80, "            Skill");
	if (skill.value == 0)
		M_Print (160, 80, "Easy difficulty");
	else if (skill.value == 1)
		M_Print (160, 80, "Normal difficulty");
	else if (skill.value == 2)
		M_Print (160, 80, "Hard difficulty");
	else
		M_Print (160, 80, "Nightmare difficulty");

	M_Print (0, 88, "       Frag Limit");
	if (fraglimit.value == 0)
		M_Print (160, 88, "none");
	else
		M_Print (160, 88, va("%i frags", (int)fraglimit.value));

	M_Print (0, 96, "       Time Limit");
	if (timelimit.value == 0)
		M_Print (160, 96, "none");
	else
		M_Print (160, 96, va("%i minutes", (int)timelimit.value));

	M_Print (0, 112, "         Episode");
	// MED 01/06/97 added hipnotic episodes
	if (hipnotic)
		M_Print (160, 112, hipnoticepisodes[startepisode].description);
	// PGM 01/07/97 added rogue episodes
	else if (rogue)
		M_Print (160, 112, rogueepisodes[startepisode].description);
	else
		M_Print (160, 112, episodes[startepisode].description);

	M_Print (0, 120, "           Level");
	// MED 01/06/97 added hipnotic episodes
	if (hipnotic)
	{
		M_Print (160, 120, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, 128, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
	}
	// PGM 01/07/97 added rogue episodes
	else if (rogue)
	{
		M_Print (160, 120, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, 128, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
	}
	else
	{
		M_Print (160, 120, levels[episodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, 128, levels[episodes[startepisode].firstLevel + startlevel].name);
	}

// line cursor
	M_DrawArrowCursor (144, gameoptions_cursor_table[gameoptions_cursor]);

	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print (x, 146, "  More than 4 players   ");
			M_Print (x, 154, " requires using command ");
			M_Print (x, 162, "line parameters; please ");
			M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}
}


void M_NetStart_Change (int dir)
{
	int count;
	float	f;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_Set ("coop", coop.value ? "0" : "1");
		break;

	case 3:
		count = (rogue) ? 6 : 2;
		f = teamplay.value + dir;
		if (f > count)	f = 0;
		else if (f < 0)	f = count;
		Cvar_SetValue ("teamplay", f);
		break;

	case 4:
		f = skill.value + dir;
		if (f > 3)	f = 0;
		else if (f < 0)	f = 3;
		Cvar_SetValue ("skill", f);
		break;

	case 5:
		f = fraglimit.value + dir * 10;
		if (f > 100)	f = 0;
		else if (f < 0)	f = 100;
		Cvar_SetValue ("fraglimit", f);
		break;

	case 6:
		f = timelimit.value + dir * 5;
		if (f > 60)	f = 0;
		else if (f < 0)	f = 60;
		Cvar_SetValue ("timelimit", f);
		break;

	case 7:
		startepisode += dir;
	//MED 01/06/97 added hipnotic count
		if (hipnotic)
			count = 6;
	//PGM 01/07/97 added rogue count
	//PGM 03/02/97 added 1 for dmatch episode
		else if (rogue)
			count = 4;
		else if (registered.value)
			count = 7;
		else
			count = 2;

		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 8:
		startlevel += dir;
	//MED 01/06/97 added hipnotic episodes
		if (hipnotic)
			count = hipnoticepisodes[startepisode].levels;
	//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
			count = rogueepisodes[startepisode].levels;
		else
			count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		M_ThrottledSound ("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		M_ThrottledSound ("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
	//case K_MOUSE2:
		if (gameoptions_cursor == 0)
			break;
		M_ThrottledSound ("misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		M_ThrottledSound ("misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		M_ThrottledSound ("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			SCR_BeginLoadingPlaque ();

			if (hipnotic)
				Cbuf_AddText ( va ("map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name) );
			else if (rogue)
				Cbuf_AddText ( va ("map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name) );
			else
				Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}

		M_NetStart_Change (1);
		break;
	}
}


void M_GameOptions_Mousemove (float cx, float cy)
{
	int prev = gameoptions_cursor;
	M_UpdateCursorWithTable (cy, gameoptions_cursor_table, NUM_GAMEOPTIONS, &gameoptions_cursor);
	if (gameoptions_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	qpic_t	*p;
	int x;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12, 1);
	M_Print (x, 40, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int	n;
	qpic_t	*p;

	if (!slist_sorted)
	{
		slist_sorted = true;
		NET_SlistSort ();
	}

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	for (n = 0; n < hostCacheCount; n++)
		M_Print (16, 32 + 8*n, NET_SlistPrintServer (n));
	M_DrawArrowCursor (0, 32 + slist_cursor*8);

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		M_ThrottledSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		M_ThrottledSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	case K_MOUSE1:
		M_ThrottledSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		IN_Activate();
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", NET_SlistPrintServerName(slist_cursor)) );
		break;

	default:
		break;
	}

}


void M_ServerList_Mousemove (float cx, float cy)
{
	int prev = slist_cursor;
	M_UpdateCursor (cy, 32, 8, hostCacheCount, &slist_cursor);
	if (slist_cursor != prev)
		M_MouseSound ("misc/menu1.wav");
}

//=============================================================================
/* Mods menu */

#define MODLIST_OFS				32
#define MODLIST_MAXCOLS			56
#define DOWNLOAD_FLASH_TIME		1.0

typedef struct
{
	const char				*name;
	const filelist_item_t	*source;
	int						modidx;
	qboolean				active;
} moditem_t;

static struct
{
	menulist_t			list;
	menuticker_t		ticker;
	int					x, y, cols;
	int					modcount;
	int					prev_cursor;
	double				download_flash_time;
	enum m_state_e		prev;
	qboolean			scrollbar_grab;
	moditem_t			*items;
} modsmenu;

static qboolean M_Mods_IsActive (const char *game)
{
	extern char com_gamenames[];
	const char *list, *end, *p;

	if (!q_strcasecmp (game, GAMENAME))
		return !*com_gamenames;

	list = com_gamenames;
	while (*list)
	{
		end = list;
		while (*end && *end != ';')
			end++;

		p = game;
		while (*p && list != end)
			if (q_tolower (*p) == q_tolower (*list))
				p++, list++;
			else
				break;

		if (!*p && list == end)
			return true;

		list = end;
		if (*list)
			list++;
	}

	return false;
}

static void M_Mods_AddDecoration (const char *text)
{
	moditem_t item;
	memset (&item, 0, sizeof (item));
	item.name = text;
	VEC_PUSH (modsmenu.items, item);
	modsmenu.list.numitems++;
}

static void M_Mods_AddSeparator (qboolean installed)
{
	#define QBAR "\35\36\37"

	if (installed && !modsmenu.list.numitems)
		return;

	if (modsmenu.list.numitems)
		M_Mods_AddDecoration ("");

	if (installed)
		M_Mods_AddDecoration (QBAR " Installed " QBAR);
	else
		M_Mods_AddDecoration (QBAR " Available for download " QBAR);

	M_Mods_AddDecoration ("");

	#undef QBAR
}

static void M_Mods_Add (const filelist_item_t *item)
{
	moditem_t mod;
	memset (&mod, 0, sizeof (mod));
	mod.name = item->name;
	mod.source = item;
	mod.active = M_Mods_IsActive (item->name);
	mod.modidx = modsmenu.modcount++;
	if (mod.active && modsmenu.list.cursor == -1)
		modsmenu.list.cursor = modsmenu.list.numitems;
	VEC_PUSH (modsmenu.items, mod);
	modsmenu.list.numitems++;
}

static qboolean M_Mods_Match (int index)
{
	const filelist_item_t *source = modsmenu.items[index].source;
	const char *fullname;
	if (!source)
		return false;

	if (q_strcasestr (source->name, modsmenu.list.search.text))
		return true;

	fullname = Modlist_GetFullName (source);
	return fullname && q_strcasestr (fullname, modsmenu.list.search.text);
}

static qboolean M_Mods_IsSelectable (int index)
{
	return modsmenu.items[index].source != NULL;
}

static void M_Mods_UpdateLayout (void)
{
	int height;

	M_UpdateBounds ();

	height = modsmenu.list.numitems * 8 + MODLIST_OFS + 16;
	height = q_min (height, m_height);
	modsmenu.cols = m_width / 8 - 2;
	modsmenu.cols = q_min (modsmenu.cols, MODLIST_MAXCOLS);
	modsmenu.x = m_left + (m_width - modsmenu.cols * 8) / 2;
	modsmenu.y = m_top + (((m_height - height) / 2) & ~7);
	modsmenu.list.viewsize = (height - MODLIST_OFS - 16) / 8;
}

static void M_Mods_Init (void)
{
	int pass, count;
	filelist_item_t *item;

	modsmenu.scrollbar_grab = false;
	memset (&modsmenu.list.search, 0, sizeof (modsmenu.list.search));
	modsmenu.list.search.match_fn = M_Mods_Match;
	modsmenu.list.isactive_fn = M_Mods_IsSelectable;
	modsmenu.list.cursor = -1;
	modsmenu.list.scroll = 0;
	modsmenu.list.numitems = 0;
	modsmenu.modcount = 0;
	VEC_CLEAR (modsmenu.items);

	M_Ticker_Init (&modsmenu.ticker);

	for (pass = 0; pass < 2; pass++)
	{
		count = 0;
		for (item = modlist; item; item = item->next)
		{
			modstatus_t status = Modlist_GetStatus (item);
			qboolean installed = status == MODSTATUS_INSTALLED;
			if (installed != pass)
				continue;
			if (!count++)
				M_Mods_AddSeparator (pass);
			M_Mods_Add (item);

			// always focus the add-on being downloaded, if any,
			// since it will be activated automatically afterwards
			if (status == MODSTATUS_INSTALLING)
				modsmenu.list.cursor = modsmenu.list.numitems - 1;
		}
	}

	if (modsmenu.list.cursor == -1)
		M_List_SelectNextActive (&modsmenu.list, 0, 1, false);

	M_Mods_UpdateLayout ();

	M_List_CenterCursor (&modsmenu.list);
}

void M_Menu_Mods_f (void)
{
	IN_DeactivateForMenu();
	key_dest = key_menu;
	modsmenu.prev = m_state;
	m_state = m_mods;
	m_entersound = true;
	M_Mods_Init ();
}

void M_Mods_Draw (void)
{
	const char *str;
	int x, y, i, j, cols;
	int firstvis, numvis;
	int firstvismod, numvismods;
	int namecols, desccols;
	int flash;

	if (!keydown[K_MOUSE1])
		modsmenu.scrollbar_grab = false;

	M_Mods_UpdateLayout ();
	M_List_Update (&modsmenu.list);

	namecols = (int) CLAMP (16, modsmenu.cols * 0.4375f, 24) & ~1;
	desccols = modsmenu.cols - 1 - namecols;

	if (modsmenu.prev_cursor != modsmenu.list.cursor)
	{
		modsmenu.prev_cursor = modsmenu.list.cursor;
		M_Ticker_Init (&modsmenu.ticker);
	}
	else
		M_Ticker_Update (&modsmenu.ticker);

	modsmenu.download_flash_time = q_max (0.0, modsmenu.download_flash_time - host_rawframetime);
	flash = (int)(modsmenu.download_flash_time * 8.0) & 1;

	x = modsmenu.x;
	y = modsmenu.y;
	cols = modsmenu.cols;

	Draw_StringEx (x, y + 4, 12, "Mods");
	M_DrawQuakeBar (x - 8, y + 16, namecols + 1);
	M_DrawQuakeBar (x + namecols * 8, y + 16, cols + 1 - namecols);

	y += MODLIST_OFS;

	firstvismod = -1;
	numvismods = 0;
	M_List_GetVisibleRange (&modsmenu.list, &firstvis, &numvis);
	for (i = 0; i < numvis; i++)
	{
		int idx = i + firstvis;
		const moditem_t *item = &modsmenu.items[idx];
		int mask = item->active ? 128 : 0;
		const char *message = item->source ? Modlist_GetFullName (item->source) : NULL;
		qboolean selected = (idx == modsmenu.list.cursor);

		if (!item->source)
		{
			M_PrintWhite (x + (cols - strlen (item->name))/2*8, y + i*8, item->name);
		}
		else
		{
			char tinted[256], buf[256];
			if (modsmenu.list.search.len > 0)
				COM_TintSubstring (item->name, modsmenu.list.search.text, tinted, sizeof (tinted));
			else
				q_strlcpy (tinted, item->name, sizeof (tinted));
			if (Modlist_GetStatus (item->source) == MODSTATUS_INSTALLING)
			{
				double progress = Modlist_GetDownloadProgress (item->source);
				q_snprintf (buf, sizeof (buf), "\20%3.0f%%\21 %s", 100.0 * progress, item->name);
				if (flash)
					mask ^= 128;
			}
			else
				q_strlcpy (buf, tinted, sizeof (buf));

			if (firstvismod == -1)
				firstvismod = item->modidx;
			numvismods++;

			for (j = 0; j < namecols - 2 && buf[j]; j++)
				M_DrawCharacter (x + j*8, y + i*8, buf[j] ^ mask);

			if (message && message[0])
			{
				if (modsmenu.list.search.len > 0)
					COM_TintSubstring (message, modsmenu.list.search.text, buf, sizeof (buf));
				else
					q_strlcpy (buf, message, sizeof (buf));

				GL_SetCanvasColor (1, 1, 1, 0.375f);
				for (/**/; j < namecols; j++)
					M_DrawCharacter (x + j*8, y + i*8, '.' | mask);
				if (message)
					GL_SetCanvasColor (1, 1, 1, 1);

				M_PrintScroll (x + namecols*8, y + i*8, desccols*8, buf,
					selected ? modsmenu.ticker.scroll_time : 0.0, true);

				if (!message)
					GL_SetCanvasColor (1, 1, 1, 1);
			}
		}

		if (idx == modsmenu.list.cursor)
			M_DrawArrowCursor (x - 8, y + i*8);
	}

	str = va("%d-%d of %d", firstvismod + 1, firstvismod + numvismods, modsmenu.modcount);
	M_Print (x + (cols - strlen (str))*8, y - 24, str);

	if (M_List_GetOverflow (&modsmenu.list) > 0)
	{
		M_List_DrawScrollbar (&modsmenu.list, x + cols*8 - 8, y);

		if (modsmenu.list.scroll > 0)
			M_DrawEllipsisBar (x, y - 8, cols);
		if (modsmenu.list.scroll + modsmenu.list.viewsize < modsmenu.list.numitems)
			M_DrawEllipsisBar (x, y + modsmenu.list.viewsize*8, cols);
	}

	i = q_min (modsmenu.list.viewsize, modsmenu.list.numitems);
	M_List_DrawSearch (&modsmenu.list, x, y + i*8 + 4, namecols);
}

void M_Mods_Char (int key)
{
	M_List_Char (&modsmenu.list, key);
}

textmode_t M_Mods_TextEntry (void)
{
	return modsmenu.scrollbar_grab ? TEXTMODE_OFF : TEXTMODE_NOPOPUP;
}

void M_Mods_Key (int key)
{
	const filelist_item_t *item;
	float x, y;

	if (modsmenu.scrollbar_grab)
	{
		switch (key)
		{
		case K_ESCAPE:
		case K_BBUTTON:
		case K_MOUSE4:
		case K_MOUSE2:
			modsmenu.scrollbar_grab = false;
			break;
		}
		return;
	}

	if (M_List_Key (&modsmenu.list, key))
		return;

	if (M_Ticker_Key (&modsmenu.ticker, key))
	{
		M_List_KeepSearchVisible (&modsmenu.list, 1.0);
		return;
	}

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		M_List_ClearSearch (&modsmenu.list);
		m_state = modsmenu.prev;
		if (m_state == m_none)
		{
			IN_Activate ();
			key_dest = key_game;
		}
		M_ThrottledSound ("misc/menu2.wav");
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
	enter:
		M_List_ClearSearch (&modsmenu.list);
		item = modsmenu.items[modsmenu.list.cursor].source;
		if (Modlist_GetStatus (item) == MODSTATUS_INSTALLED)
		{
			Cbuf_AddText (va ("game %s\n", item->name));
			M_Menu_Main_f ();
		}
		else
		{
			M_Menu_ModInfo_f (item);
		}
		break;

	case K_MOUSE1:
		x = m_mousex - modsmenu.x - (modsmenu.cols - 1) * 8;
		y = m_mousey - modsmenu.y - MODLIST_OFS;
		if (x < -8 || !M_List_UseScrollbar (&modsmenu.list, y))
			goto enter;
		modsmenu.scrollbar_grab = true;
		M_Mods_Mousemove (m_mousex, m_mousey);
		break;

	default:
		break;
	}
}


void M_Mods_Mousemove (float cx, float cy)
{
	cy -= modsmenu.y + MODLIST_OFS;

	if (modsmenu.scrollbar_grab)
	{
		if (!keydown[K_MOUSE1])
		{
			modsmenu.scrollbar_grab = false;
			return;
		}
		M_List_UseScrollbar (&modsmenu.list, cy);
		// Note: no return, we also update the cursor
	}

	M_List_Mousemove (&modsmenu.list, cy);
}

void M_OnModInstall (const char *name)
{
	if (key_dest != key_menu || (m_state != m_mods && m_state != m_modinfo))
		return;
	Cbuf_AddText (va ("game \"%s\"\n", name));
	M_Menu_Main_f ();
}

void M_RefreshMods (void)
{
	if (key_dest != key_menu || m_state != m_mods)
		return;
	M_Mods_Init ();
}

//=============================================================================
/* Mod info menu */

#define MODINFO_HEADERCOLS			16
#define MODINFO_INFOCOLS			(modinfomenu.cols - 2 - MODINFO_HEADERCOLS)
#define MODINFO_MAXAUTHORLINES		11
#define MODINFO_BOXMARGIN			1	// lines, not pixels

static struct
{
	const filelist_item_t	*item;
	int						x, y, cols, lines;
	char					title[36];
	char					author[256];
} modinfomenu;

static void M_ModInfo_UpdateLayout (void)
{
	int			width = strlen (modinfomenu.title) * 12 + 16;
	int			height = 0;
	const char	*str;
	
	str = modinfomenu.author;
	while (*str && height < MODINFO_MAXAUTHORLINES)
	{
		M_LineWrap (&str, MODINFO_INFOCOLS);
		height++;
	}

	if (height)
		height = height * 8 + 8;
	if (Modlist_GetDate (modinfomenu.item))
		height += 16;
	if (Modlist_GetDownloadSize (modinfomenu.item))
		height += 16;

	height += 12 + 8 + 8; // title + bar + spacing
	height += 32; // download button
	height += MODINFO_BOXMARGIN * 2;

	modinfomenu.cols = CLAMP (320 - 32, width, m_width - 32) / 8;
	modinfomenu.cols = (modinfomenu.cols + 1) & ~1;
	modinfomenu.lines = (height + 7) / 8;
	modinfomenu.x = m_left + (((m_width - modinfomenu.cols * 8) / 2) & ~7);
	modinfomenu.y = m_top + (((m_height - height) / 2) & ~7) + MODINFO_BOXMARGIN * 8;
}

void M_Menu_ModInfo_f (const filelist_item_t *item)
{
	const char *str;
	size_t i, j;

	m_entersound = true;
	if (Modlist_IsInstalling () || Modlist_GetStatus (item) != MODSTATUS_DOWNLOADABLE)
	{
		modsmenu.download_flash_time = DOWNLOAD_FLASH_TIME;
		return;
	}

	IN_DeactivateForMenu();
	key_dest = key_menu;
	m_state = m_modinfo;
	m_entersound = true;
	modinfomenu.item = item;

	str = Modlist_GetFullName (item);
	if (!str)
		str = item->name;
	COM_TintString (str, modinfomenu.title, sizeof (modinfomenu.title));

	str = Modlist_GetAuthor (item);
	if (!str || !*str)
		modinfomenu.author[0] = '\0';
	else
	{
		UTF8_ToQuake (modinfomenu.author, sizeof (modinfomenu.author), str);

		// clean up a bit
		for (i = j = 0; modinfomenu.author[i]; i++)
		{
			char c = modinfomenu.author[i];
			// replace newlines with spaces (since we're doing manual word wrapping)
			if (c == '\n')
				c = ' ';
			// remove leading spaces, replace consecutive spaces with single one
			if (c != ' ' || (j > 0 && modinfomenu.author[j - 1] != ' '))
				modinfomenu.author[j++] = c;
		}

		// remove trailing spaces
		while (j > 0 && modinfomenu.author[j] == ' ')
			--j;

		modinfomenu.author[j] = '\0';
	}

	M_ModInfo_UpdateLayout ();
}

void M_ModInfo_Draw (void)
{
	const char	*str;
	double		size;
	int			x, x2, y, namecols, len;

	M_ModInfo_UpdateLayout ();

	namecols = MODINFO_HEADERCOLS;
	x = modinfomenu.x + 8;
	x2 = x + namecols * 8;
	y = modinfomenu.y;

	M_DrawTextBox (x - 24, y - 12, modinfomenu.cols + 2, modinfomenu.lines - 4);

	len = strlen (modinfomenu.title);
	if (len * 10 + 16 > modinfomenu.cols * 8)
		M_PrintSubstring (x, y + 2, modinfomenu.title, modinfomenu.cols - 2, false);
	else if (len * 12 + 16 > modinfomenu.cols * 8)
		Draw_StringEx (x, y, 10, modinfomenu.title);
	else
		Draw_StringEx (x, y, 12, modinfomenu.title);
	y += 12;

	M_DrawQuakeBar (x - 8, y, modinfomenu.cols);
	y += 16;

	str = modinfomenu.author;
	if (*str)
	{
		int maxlines = MODINFO_MAXAUTHORLINES;
		M_PrintDotFill (x, y, "Created by", namecols, false);
		y += M_PrintWordWrap (x2, y, str, MODINFO_INFOCOLS * 8, maxlines * 8, false) * 8 + 8;
	}

	str = Modlist_GetDate (modinfomenu.item);
	if (str)
	{
		M_PrintDotFill (x, y, "Release date", namecols, false);
		M_PrintWhite (x2, y, str);
		y += 16;
	}

	size = Modlist_GetDownloadSize (modinfomenu.item);
	if (size)
	{
		M_PrintDotFill (x, y, "Download size", namecols, false);
		M_PrintWhite (x2, y, va ("%.1f MB", size / (double) 0x100000));
		y += 16;
	}

	str = "Download";
	len = strlen (str);
	x = 160 - (len / 2) * 8;
	y = modinfomenu.y + modinfomenu.lines * 8 - 8;
	M_DrawTextBox (x - 8, y - 8, len, 1);

	M_Print (x, y, str);
	M_DrawArrowCursor (x - 16, y);
}

void M_ModInfo_Key (int key)
{
	float dx, dy;

	switch (key)
	{
	case K_MOUSE1:
		dx = fabs (m_mousex - 160) / 8;
		dy = fabs (m_mousey - modinfomenu.y - (modinfomenu.lines - 1) * 8 - 4) / 8;
		if (dy > 0 || dx > 4)
		{
			m_entersound = true;
			break;
		}
		/* fall-through */

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		Modlist_StartInstalling (modinfomenu.item);
		/* fall-through */

	case K_ESCAPE:
	case K_BBUTTON:
	case K_MOUSE4:
	case K_MOUSE2:
		m_state = m_mods;
		m_entersound = true;
		break;
	}
}


//=============================================================================
/* Credits menu -- used by the 2021 re-release */

void M_Menu_Credits_f (void)
{
}

//=============================================================================
/* Menu Subsystem */

static void UI_Mouse_f (cvar_t *cvar)
{
	// Ignore first mouse move message after we re-enable the option.
	// This makes it possible to cycle through the UI Mouse options
	// in full-screen mode using the keyboard without having the
	// selected item change due to the automatic mouse move message
	// sent when the cursor is shown again.
	if (modestate == MS_FULLSCREEN)
		m_ignoremouseframe = true;

	switch (key_dest)
	{
	case key_menu:
		IN_DeactivateForMenu ();
		break;
	case key_console:
		IN_DeactivateForConsole ();
		break;
	case key_game:
	case key_message:
		IN_Activate ();
		break;
	default:
		break;
	}
}

void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_gamepad", M_Menu_Gamepad_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_credits", M_Menu_Credits_f); // needed by the 2021 re-release
	Cmd_AddCommand ("menu_mods", M_Menu_Mods_f);
	Cmd_AddCommand ("menu_maps", M_Menu_Maps_f);

	Cvar_RegisterVariable (&ui_mouse);
	Cvar_SetCallback (&ui_mouse, UI_Mouse_f);
	Cvar_RegisterVariable (&ui_live_preview);
	Cvar_RegisterVariable (&ui_mouse_sound);
	Cvar_RegisterVariable (&ui_sound_throttle);
	Cvar_RegisterVariable (&ui_search_timeout);
}

static void M_UpdateBounds (void)
{
	drawtransform_t transform;
	float left, top, right, bottom;
	float width, height;
	float xfrac = 0.5f;
	float yfrac = 0.5f;

	Draw_GetCanvasTransform (CANVAS_MENU, &transform);
	Draw_GetTransformBounds (&transform, &left, &top, &right, &bottom);
	width = right - left;
	height = bottom - top;

	m_height = q_min (height, 280);
	m_height += (height - m_height) * yfrac;
	m_height &= ~7;
	m_width = 320 + q_max (0, width - 320) * xfrac;
	m_width = q_min (m_width, m_height * 2);
	m_width = q_min (m_width, 960);
	m_width &= ~15;
	m_left = left + (width - m_width) / 2;
	m_top = top + (height - m_height) / 2;
}

void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	M_UpdateBounds ();

	if (!m_recursiveDraw)
	{
		//johnfitz -- fade even if console fills screen
		if (M_GetBaseState (m_state) == m_options)
			M_Options_DrawFadeScreen ();
		else
			Draw_FadeScreen (1.f);
	}
	else
	{
		m_recursiveDraw = false;
	}

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	switch (M_GetBaseState (m_state))
	{
	default:
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_maps:
		M_Maps_Draw ();
		break;

	case m_skill:
		M_Skill_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_calibration:
		M_Calibration_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_mods:
		M_Mods_Draw ();
		break;

	case m_modinfo:
		M_ModInfo_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		if (!fitzmode && !cl_confirmquit.value)
		{ /* QuakeSpasm customization: */
			/* Quit now! S.A. */
			key_dest = key_console;
			Host_Quit_f ();
		}
		M_Quit_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	}

	if (m_entersound)
	{
		M_ThrottledSound ("misc/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_Keydown (int key)
{
	if (!bind_grab && !ui_mouse.value && M_IsMouseKey (key))
		return;

	m_lastkey = key;
	if (!bind_grab)
	{
		switch (key)
		{
			case K_DPAD_UP:		key = K_UPARROW; break;
			case K_DPAD_DOWN:	key = K_DOWNARROW; break;
			case K_DPAD_LEFT:	key = K_LEFTARROW; break;
			case K_DPAD_RIGHT:	key = K_RIGHTARROW; break;
			default:
				break;
		}
	}

	switch (M_GetBaseState (m_state))
	{
	default:
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_maps:
		M_Maps_Key (key);
		return;

	case m_skill:
		M_Skill_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_calibration:
		M_Calibration_Key (key);
		break;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_mods:
		M_Mods_Key (key);
		return;

	case m_modinfo:
		M_ModInfo_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
	}
}


void M_Mousemove (int screenx, int screeny)
{
	drawtransform_t transform;
	float x, y;

	if (bind_grab || !ui_mouse.value)
		return;

	Draw_GetCanvasTransform (CANVAS_MENU, &transform);
	x = (screenx - glx) * 2.f / (float) glwidth - 1.f;
	y = (screeny - gly) * 2.f / (float) glheight - 1.f;
	y = -y;
	x = (x - transform.offset[0]) / transform.scale[0];
	y = (y - transform.offset[1]) / transform.scale[1];
	m_mousex = x;
	m_mousey = y;

	if (m_ignoremouseframe)
	{
		m_ignoremouseframe = false;
		return;
	}

	switch (M_GetBaseState (m_state))
	{
	default:
		return;

	case m_main:
		M_Main_Mousemove (x, y);
		return;

	case m_singleplayer:
		M_SinglePlayer_Mousemove (x, y);
		return;

	case m_load:
		M_Load_Mousemove (x, y);
		return;

	case m_save:
		M_Save_Mousemove (x, y);
		return;

	case m_maps:
		M_Maps_Mousemove (x, y);
		return;

	case m_skill:
		M_Skill_Mousemove (x, y);
		return;

	case m_multiplayer:
		M_MultiPlayer_Mousemove (x, y);
		return;

	case m_setup:
		M_Setup_Mousemove (x, y);
		return;

	case m_net:
		M_Net_Mousemove (x, y);
		return;

	case m_options:
		M_Options_Mousemove (x, y);
		return;

	case m_keys:
		M_Keys_Mousemove (x, y);
		return;

	case m_mods:
		M_Mods_Mousemove (x, y);
		return;

	//case m_help:
	//	M_Help_Mousemove (x, y);
	//	return;

	//case m_quit:
	//	M_Quit_Mousemove (x, y);
	//	return;

	case m_lanconfig:
		M_LanConfig_Mousemove (x, y);
		return;

	case m_gameoptions:
		M_GameOptions_Mousemove (x, y);
		return;

	//case m_search:
	//	M_Search_Mousemove (x, y);
	//	break;

	case m_slist:
		M_ServerList_Mousemove (x, y);
		return;
	}
}


void M_Charinput (int key)
{
	switch (M_GetBaseState (m_state))
	{
	case m_setup:
		M_Setup_Char (key);
		return;
	case m_quit:
		M_Quit_Char (key);
		return;
	case m_lanconfig:
		M_LanConfig_Char (key);
		return;
	case m_maps:
		M_Maps_Char (key);
		return;
	case m_mods:
		M_Mods_Char (key);
		return;
	case m_options:
		M_Options_Char (key);
		return;
	case m_keys:
		M_Keys_Char (key);
		return;
	default:
		return;
	}
}


textmode_t M_TextEntry (void)
{
	switch (M_GetBaseState (m_state))
	{
	case m_setup:
		return M_Setup_TextEntry ();
	case m_quit:
		return M_Quit_TextEntry ();
	case m_lanconfig:
		return M_LanConfig_TextEntry ();
	case m_maps:
		return M_Maps_TextEntry ();
	case m_mods:
		return M_Mods_TextEntry ();
	case m_options:
		return M_Options_TextEntry ();
	case m_keys:
		return M_Keys_TextEntry ();
	default:
		return TEXTMODE_OFF;
	}
}


qboolean M_WaitingForKeyBinding (void)
{
	return key_dest == key_menu && m_state == m_keys && bind_grab;
}

qboolean M_WantsConsole (float *alpha)
{
	qboolean forced = (key_dest == key_menu && M_GetBaseState (m_state) == m_options && M_Options_WantsConsole ());
	if (alpha)
		*alpha = forced ? M_Options_PreviewAlpha () : 0.f;
	return forced;
}

qboolean M_ForcedCenterPrint (float *alpha)
{
	qboolean forced = (key_dest == key_menu && M_GetBaseState (m_state) == m_options) ? M_Options_ForcedCenterPrint () : false;
	if (alpha)
		*alpha = forced ? M_Options_PreviewAlpha () : 0.f;
	return forced;
}

qboolean M_ForcedUnderwater (void)
{
	return key_dest == key_menu && M_GetBaseState (m_state) == m_options && M_Options_ForcedUnderwater ();
}

void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config
	Cbuf_AddText ("stopdemo\n");

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}

//=============================================================================

static qboolean M_CheckCustomGfx (const char *custompath, const char *basepath, int knownlength, const unsigned int *hashes, int numhashes)
{
	unsigned int id_custom, id_base;
	int h, length;
	qboolean ret = false;

	if (!COM_FileExists (custompath, &id_custom))
		return false;

	length = COM_OpenFile (basepath, &h, &id_base);
	if (id_custom >= id_base)
		ret = true;
	else if (length == knownlength)
	{
		int mark = Hunk_LowMark ();
		byte* data = (byte*) Hunk_Alloc (length);
		if (length == Sys_FileRead (h, data, length))
		{
			unsigned int hash = COM_HashBlock (data, length);
			while (numhashes-- > 0 && !ret)
				if (hash == *hashes++)
					ret = true;
		}
		Hunk_FreeToLowMark (mark);
	}

	COM_CloseFile (h);

	return ret;
}


void M_CheckMods (void)
{
	const unsigned int
		main_hashes[] = {0x136bc7fd, 0x90555cb4},
		sp_hashes[] = {0x86a6f086},
		sgl_hashes[] = {0x7bba813d}
	;

	m_main_mods = M_CheckCustomGfx ("gfx/menumods.lmp",
		"gfx/mainmenu.lmp", 26888, main_hashes, countof (main_hashes));

	m_singleplayer_showlevels = M_CheckCustomGfx ("gfx/sp_maps.lmp",
		"gfx/sp_menu.lmp", 14856, sp_hashes, countof (sp_hashes));

	m_skill_usegfx = M_CheckCustomGfx ("gfx/skillmenu.lmp",
		"gfx/sp_menu.lmp", 14856, sp_hashes, countof (sp_hashes));

	m_skill_usecustomtitle = M_CheckCustomGfx ("gfx/p_skill.lmp",
		"gfx/ttl_sgl.lmp", 6728, sgl_hashes, countof (sgl_hashes));
}

