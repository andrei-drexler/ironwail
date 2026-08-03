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

#ifndef _QUAKE_MENU_H
#define _QUAKE_MENU_H

enum m_state_e {
	m_none,
	m_main,
	m_singleplayer,
	m_load,
	m_save,
	m_maps,
	m_skill,
	m_multiplayer,
	m_setup,
	m_net,
	m_options,
	m_video,
	m_graphics,
	m_interface,
	m_game,
	m_keys,
	m_calibration,
	m_gamepad,
	m_mods,
	m_modinfo,
	m_help,
	m_quit,
	m_lanconfig,
	m_gameoptions,
	m_search,
	m_slist
};

extern enum m_state_e m_state;
extern enum m_state_e m_return_state;

extern qboolean m_entersound;

//
// menus
//
void M_Init (void);
void M_Keydown (int key, qboolean repeat);
void M_Charinput (int key);
void M_Mousemove (int x, int y);
enum textmode_t M_TextEntry (void);
qboolean M_WaitingForKeyBinding (void);
qboolean M_WantsConsole (float *alpha);
qboolean M_ForcedCenterPrint (float *alpha);
qboolean M_ForcedUnderwater (void);
void M_ToggleMenu_f (void);

void M_RefreshMods (void);
void M_OnModInstall (const char *name);

void M_Menu_Main_f (void);
void M_Menu_Options_f (void);
void M_Menu_Quit_f (void);

void M_Print (int cx, int cy, const char *str);
void M_PrintWhite (int cx, int cy, const char *str);

void M_Draw (void);
void M_DrawCharacter (int cx, int line, int num);

void M_DrawPic (int x, int y, qpic_t *pic);
void M_DrawSubpic (int x, int y, qpic_t *pic, int left, int top, int width, int height);
void M_DrawTransPic (int x, int y, qpic_t *pic);
void M_DrawCheckbox (int x, int y, float value);
void M_DrawTextBox (int x, int y, int width, int lines);

#endif	/* _QUAKE_MENU_H */

