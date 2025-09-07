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

#ifndef _QUAKE_KEYS_H
#define _QUAKE_KEYS_H

//
// gamepad button definitions
//
#define GAMEPAD_KEY_LIST(def)																					\
	/*	Keycode			Enum value			XBox name			PlayStation name			Nintendo name	*/	\
	def (K_START,		= K_GAMEPAD_BEGIN,	"MENU",				"OPTIONS",					"+")				\
	def (K_BACK,		/*auto*/,			"VIEW",				"CREATE",					"-")				\
	def (K_LTHUMB,		/*auto*/,			"LS",				"L3",						"LSB")				\
	def (K_RTHUMB,		/*auto*/,			"RS",				"R3",						"RSB")				\
	def (K_LSHOULDER,	/*auto*/,			"LB",				"L1",						"L")				\
	def (K_RSHOULDER,	/*auto*/,			"RB",				"R1",						"R")				\
	def (K_DPAD_UP,		/*auto*/,			"DPAD UP",			"DPAD UP",					"DPAD UP")			\
	def (K_DPAD_DOWN,	/*auto*/,			"DPAD DOWN",		"DPAD DOWN",				"DPAD DOWN")		\
	def (K_DPAD_LEFT,	/*auto*/,			"DPAD LEFT",		"DPAD LEFT",				"DPAD LEFT")		\
	def (K_DPAD_RIGHT,	/*auto*/,			"DPAD RIGHT",		"DPAD RIGHT",				"DPAD RIGHT")		\
	def (K_ABUTTON,		/*auto*/,			"A",				"X",						"A")				\
	def (K_BBUTTON,		/*auto*/,			"B",				"CIRCLE",					"B")				\
	def (K_XBUTTON,		/*auto*/,			"X",				"SQUARE",					"X")				\
	def (K_YBUTTON,		/*auto*/,			"Y",				"TRIANGLE",					"Y")				\
	def (K_LTRIGGER,	/*auto*/,			"LT",				"L2",						"ZL")				\
	def (K_RTRIGGER,	/*auto*/,			"RT",				"R2",						"ZR")				\
	def (K_MISC1,		/*auto*/,			NULL,				"MUTE",						"CAPTURE")			\
	def (K_PADDLE1,		/*auto*/,			"P1 PADDLE",		NULL,						NULL)				\
	def (K_PADDLE2,		/*auto*/,			"P2 PADDLE",		NULL,						NULL)				\
	def (K_PADDLE3,		/*auto*/,			"P3 PADDLE",		NULL,						NULL)				\
	def (K_PADDLE4,		/*auto*/,			"P4 PADDLE",		NULL,						NULL)				\
	def (K_TOUCHPAD,	/*auto*/,			NULL,				"TOUCHPAD",					NULL)				\


//
// these are the key numbers that should be passed to Key_Event
//
typedef enum keycode_t
{
	K_TAB				= 9,
	K_ENTER				= 13,
	K_ESCAPE			= 27,
	K_SPACE				= 32,

// normal keys should be passed as lowercased ascii

	K_BACKSPACE			= 127,
	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_KP_NUMLOCK,
	K_KP_SLASH,
	K_KP_STAR,
	K_KP_MINUS,
	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_PLUS,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,

	K_COMMAND,

	K_CAPSLOCK,
	K_SCROLLLOCK,
	K_PRINTSCREEN,

//
// mouse buttons generate virtual keys
//
	K_MOUSE_BEGIN		= 200,
	K_MOUSE1			= K_MOUSE_BEGIN,
	K_MOUSE2,
	K_MOUSE3,

// thumb buttons
	K_MOUSE4,
	K_MOUSE5,

// JACK: Intellimouse(c) Mouse Wheel Support
	K_MWHEELUP,
	K_MWHEELDOWN,

	K_MOUSE_END,

// SDL2 game controller keys
// Note: start/back are never actually generated, they are always remapped to ESC/TAB
// The values below are only present to make it easier to name these keys in the menus
	K_GAMEPAD_BEGIN = K_MOUSE_END,
	#define GAMEPAD_KEYCODE_ENUM(keycode, value, xboxname, psname, nintendoname) keycode value,
	GAMEPAD_KEY_LIST (GAMEPAD_KEYCODE_ENUM)
	#undef GAMEPAD_KEYCODE_ENUM
	K_GAMEPAD_END,
	K_GAMEPAD_COUNT = K_GAMEPAD_END - K_GAMEPAD_BEGIN,

	K_PAUSE = K_GAMEPAD_END,

	NUM_KEYCODES,
} keycode_t;

#define	MAX_KEYS		256
COMPILE_TIME_ASSERT (too_many_keycodes, NUM_KEYCODES <= MAX_KEYS);

#define	MAXCMDLINE	256

typedef enum {key_game, key_console, key_message, key_menu} keydest_t;
typedef enum textmode_t
{
	TEXTMODE_OFF,		// no char events
	TEXTMODE_ON,		// char events, show on-screen keyboard
	TEXTMODE_NOPOPUP,	// char events, don't show on-screen keyboard
} textmode_t;

typedef enum keydevice_t
{
	KD_NONE = -1,
	KD_KEYBOARD,
	KD_MOUSE,
	KD_GAMEPAD,
} keydevice_t;

typedef enum
{
	KDM_NONE				= 0,
	KDM_KEYBOARD			= 1 << KD_KEYBOARD,
	KDM_MOUSE				= 1 << KD_MOUSE,
	KDM_GAMEPAD				= 1 << KD_GAMEPAD,
	KDM_KEYBOARD_AND_MOUSE	= KDM_KEYBOARD | KDM_MOUSE,
	KDM_ANY					= -1,
} keydevicemask_t;

extern keydest_t	key_dest;
extern	char	*keybindings[MAX_KEYS];

#define		CMDLINES 64

extern	char	key_lines[CMDLINES][MAXCMDLINE];
extern	char	key_tabhint[MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;
extern	int		key_insert;
extern	double		key_blinktime;

extern	qboolean	chat_team;

void Key_Init (void);
void Key_ClearStates (void);
void Key_UpdateForDest (void);

void Key_BeginInputGrab (void);
void Key_EndInputGrab (void);
void Key_GetGrabbedInput (int *lastkey, int *lastchar);

void Key_Event (int key, qboolean down);
void Key_EventWithKeycode (int key, qboolean down, int keycode);
void Char_Event (int key);
textmode_t Key_TextEntry (void);

void Key_SetBinding (int keynum, const char *binding);
keydevice_t Key_GetDeviceForKeynum (int keynum);
keydevicemask_t Key_GetDeviceMaskForKeynum (int keynum);
int Key_GetKeysForCommand (const char *command, int *keys, int maxkeys, keydevicemask_t devmask);
const char *Key_KeynumToString (int keynum);
const char *Key_KeynumToFriendlyString (int keynum);
void Key_WriteBindings (FILE *f);

void Key_EndChat (void);
const char *Key_GetChatBuffer (void);
int Key_GetChatMsgLen (void);

void History_Init (void);
void History_Shutdown (void);

#endif	/* _QUAKE_KEYS_H */

