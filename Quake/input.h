/*
Copyright (C) 1996-2001 Id Software, Inc.
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

#ifndef QUAKE_INPUT_H
#define QUAKE_INPUT_H

// input.h -- external (non-keyboard) input devices

void IN_Init (void);

void IN_Shutdown (void);

void IN_Commands (void);
// oportunity for devices to stick commands on the script buffer

// mouse moved by dx and dy pixels
void IN_MouseMotion(int dx, int dy);

//
// controller gyro
//
typedef enum gyromode_t
{
	GYRO_BUTTON_IGNORED,
	GYRO_BUTTON_ENABLES,
	GYRO_BUTTON_DISABLES,
	GYRO_BUTTON_INVERTS_DIR,

	GYRO_MODE_COUNT,
} gyromode_t;

qboolean IN_HasGyro (void);
void IN_StartGyroCalibration (void);
qboolean IN_IsCalibratingGyro (void);

qboolean IN_HasLED (void);

typedef enum ds_trigger_mode_t
{
	// Official supported modes
	DS_TRIGGER_OFF,
	DS_TRIGGER_WEAPON,
	DS_TRIGGER_FEEDBACK,
	DS_TRIGGER_SLOPE,
	DS_TRIGGER_VIBRATION,
	// Unofficial supported modes
	DS_TRIGGER_BOW,
	DS_TRIGGER_GALLOPING,
	DS_TRIGGER_MACHINE,

	DS_TRIGGER_COUNT,

	// Unofficial bugged modes, broken
	DS_TRIGGER_SIMPLE_FEEDBACK,
	DS_TRIGGER_SIMPLE_WEAPON,
	DS_TRIGGER_SIMPLE_VIBRATION,
	DS_TRIGGER_LIMITED_FEEDBACK,
	DS_TRIGGER_LIMITED_WEAPON,
} ds_trigger_mode_t;

qboolean IN_HasAdaptiveTriggers (void);
const char* IN_GetDSTriggerModeName (int mode);

qboolean IN_HasGamepad (void);
const char *IN_GetGamepadName (void);
void IN_UseNextGamepad (int dir, qboolean allow_disable);

void IN_SendKeyEvents (void);
// used as a callback for Sys_SendKeyEvents() by some drivers

void IN_UpdateInputMode (void);
// do stuff if input mode (text/non-text) changes matter to the keyboard driver

enum textmode_t IN_GetTextMode (void);

void IN_Move (usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd

void IN_ClearStates (void);
// restores all button and position states to defaults

// called when the app becomes active
void IN_Activate (void);

// called when the app becomes inactive
void IN_Deactivate (qboolean free_cursor);
void IN_DeactivateForConsole (void);
void IN_DeactivateForMenu (void);

#endif
