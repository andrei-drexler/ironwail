/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2019 QuakeSpasm developers

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

#ifndef QUAKEDEFS_H
#define QUAKEDEFS_H

// quakedef.h -- primary header for client

#define	VERSION			1.09
#define	GLQUAKE_VERSION		1.00
#define	D3DQUAKE_VERSION	0.01
#define	WINQUAKE_VERSION	0.996
#define	LINUX_VERSION		1.30
#define	X11_VERSION		1.10

#define	FITZQUAKE_VERSION	0.85	//johnfitz
#define	QUAKESPASM_VERSION	0.96
#define	QUAKESPASM_VER_PATCH	3	// helper to print a string like 0.94.7
#ifndef	QUAKESPASM_VER_SUFFIX
#define	QUAKESPASM_VER_SUFFIX		// optional version suffix string literal like "-beta1"
#endif

#define IRONWAIL_VER_MAJOR		0
#define IRONWAIL_VER_MINOR		8
#define IRONWAIL_VER_PATCH		2
#ifndef IRONWAIL_VER_SUFFIX
#define IRONWAIL_VER_SUFFIX		"-dev"	// optional version suffix string literal like "-beta1"
#endif

#define	QS_STRINGIFY_(x)	#x
#define	QS_STRINGIFY(x)	QS_STRINGIFY_(x)

// combined version string like "0.92.1-beta1"
#define	QUAKESPASM_VER_STRING	QS_STRINGIFY(QUAKESPASM_VERSION) "." QS_STRINGIFY(QUAKESPASM_VER_PATCH) QUAKESPASM_VER_SUFFIX
#define	IRONWAIL_VER_STRING		QS_STRINGIFY(IRONWAIL_VER_MAJOR) "." QS_STRINGIFY(IRONWAIL_VER_MINOR) "." QS_STRINGIFY(IRONWAIL_VER_PATCH) IRONWAIL_VER_SUFFIX

// SDL version the code was compiled with
#define Q_SDL_COMPILED_VERSION_STRING	QS_STRINGIFY(SDL_MAJOR_VERSION) "." QS_STRINGIFY(SDL_MINOR_VERSION) "." QS_STRINGIFY(SDL_PATCHLEVEL)

#define CONSOLE_TITLE_STRING	"Ironwail " IRONWAIL_VER_STRING
#define WINDOW_TITLE_STRING		"Quake/Ironwail " IRONWAIL_VER_STRING
#define CONFIG_NAME				"ironwail.cfg"
#define SCREENSHOT_PREFIX		"ironwail"
#define ENGINE_PAK				"ironwail.pak"
#define ENGINE_USERDIR_WIN		"Ironwail"
#define ENGINE_USERDIR_OSX		"Ironwail"
#define ENGINE_USERDIR_UNIX		".ironwail"

//define	PARANOID			// speed sapping error checking

#define	GAMENAME	"id1"		// directory to look in by default

#ifndef RC_INVOKED // skip the rest of the file when compiling resources
#include "q_stdinc.h"

#define	MINIMUM_MEMORY	0x550000
#define	MINIMUM_MEMORY_LEVELPAK	(MINIMUM_MEMORY + 0x100000)

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH		0

// left / right
#define	YAW		1

// fall over
#define	ROLL		2


#define	MAX_QPATH	64		// max length of a quake game pathname

#define	ON_EPSILON	0.1		// point on plane side epsilon

#define	DIST_EPSILON	(0.03125)	// 1/32 epsilon to keep floating point happy (moved from world.c)

#define	MAX_MSGLEN	64000		// max length of a reliable message //ericw -- was 32000
#define	MAX_DATAGRAM	64000		// max length of unreliable message //johnfitz -- was 1024

#define	DATAGRAM_MTU	1400		// johnfitz -- actual limit for unreliable messages to nonlocal clients

//
// per-level limits
//
#define	MIN_EDICTS	256		// johnfitz -- lowest allowed value for max_edicts cvar
#define	MAX_EDICTS	32000		// johnfitz -- highest allowed value for max_edicts cvar
						// ents past 8192 can't play sounds in the standard protocol
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS	4096		// johnfitz -- was 256
#define	MAX_SOUNDS	2048		// johnfitz -- was 256

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING		64

//
// stats are integers communicated to the client by the server
//
typedef enum
{
	MAX_CL_BASE_STATS	= 32,
	MAX_CL_STATS		= 256,

	STAT_HEALTH			= 0,
	STAT_FRAGS			= 1,
	STAT_WEAPON			= 2,
	STAT_AMMO			= 3,
	STAT_ARMOR			= 4,
	STAT_WEAPONFRAME	= 5,
	STAT_SHELLS			= 6,
	STAT_NAILS			= 7,
	STAT_ROCKETS		= 8,
	STAT_CELLS			= 9,
	STAT_ACTIVEWEAPON	= 10,
	STAT_NONCLIENT		= 11,	// first stat not included in svc_clientdata
	STAT_TOTALSECRETS	= 11,
	STAT_TOTALMONSTERS	= 12,
	STAT_SECRETS		= 13,	// bumped on client side by svc_foundsecret
	STAT_MONSTERS		= 14,	// bumped by svc_killedmonster
	STAT_ITEMS			= 15,	//replaces clc_clientdata info
} stat_t;

// stock defines
//
typedef enum
{
	IT_SHOTGUN			= 1,
	IT_SUPER_SHOTGUN	= 2,
	IT_NAILGUN			= 4,
	IT_SUPER_NAILGUN	= 8,
	IT_GRENADE_LAUNCHER	= 16,
	IT_ROCKET_LAUNCHER	= 32,
	IT_LIGHTNING		= 64,
	IT_SUPER_LIGHTNING	= 128,
	IT_SHELLS			= 256,
	IT_NAILS			= 512,
	IT_ROCKETS			= 1024,
	IT_CELLS			= 2048,
	IT_AXE				= 4096,
	IT_ARMOR1			= 8192,
	IT_ARMOR2			= 16384,
	IT_ARMOR3			= 32768,
	IT_SUPERHEALTH		= 65536,
	IT_KEY1				= 131072,
	IT_KEY2				= 262144,
	IT_INVISIBILITY		= 524288,
	IT_INVULNERABILITY	= 1048576,
	IT_SUIT				= 2097152,
	IT_QUAD				= 4194304,
	IT_SIGIL1			= (1<<28),
	IT_SIGIL2			= (1<<29),
	IT_SIGIL3			= (1<<30),
	IT_SIGIL4			= (1<<31),
} items_t;

//===========================================
//rogue changed and added defines

typedef enum
{
	RIT_SHELLS				= 128,
	RIT_NAILS				= 256,
	RIT_ROCKETS				= 512,
	RIT_CELLS				= 1024,
	RIT_AXE					= 2048,
	RIT_LAVA_NAILGUN		= 4096,
	RIT_LAVA_SUPER_NAILGUN	= 8192,
	RIT_MULTI_GRENADE		= 16384,
	RIT_MULTI_ROCKET		= 32768,
	RIT_PLASMA_GUN			= 65536,
	RIT_ARMOR1				= 8388608,
	RIT_ARMOR2				= 16777216,
	RIT_ARMOR3				= 33554432,
	RIT_LAVA_NAILS			= 67108864,
	RIT_PLASMA_AMMO			= 134217728,
	RIT_MULTI_ROCKETS		= 268435456,
	RIT_SHIELD				= 536870912,
	RIT_ANTIGRAV			= 1073741824,
	RIT_SUPERHEALTH			= 2147483648,
} rogueitems_t;

//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines
typedef enum
{
	HIT_PROXIMITY_GUN_BIT	= 16,
	HIT_MJOLNIR_BIT			= 7,
	HIT_LASER_CANNON_BIT	= 23,
	HIT_PROXIMITY_GUN		= (1<<HIT_PROXIMITY_GUN_BIT),
	HIT_MJOLNIR				= (1<<HIT_MJOLNIR_BIT),
	HIT_LASER_CANNON		= (1<<HIT_LASER_CANNON_BIT),
	HIT_WETSUIT				= (1<<(23+2)),
	HIT_EMPATHY_SHIELDS		= (1<<(23+3)),
} hipnoticitems_t;

//===========================================

#define	MAX_SCOREBOARD		16
#define	MAX_SCOREBOARDNAME	32

#define	SOUND_CHANNELS		8

#define	NUM_SPAWN_PARMS		16


typedef struct
{
	const char *basedir;
	const char *userdir;	// user's directory on UNIX platforms.
				// if user directories are enabled, basedir
				// and userdir will point to different
				// memory locations, otherwise to the same.
	const char *exedir;
	int	argc;
	char	**argv;
	void	*membase;
	int	memsize;
	int	numcpus;
	int	errstate;
} quakeparms_t;

#include "sys.h"
#include "common.h"
#include "filesys.h"
#include "bspfile.h"
#include "zone.h"
#include "mathlib.h"
#include "cvar.h"

#include "protocol.h"
#include "net.h"

#include "cmd.h"
#include "crc.h"

#include "platform.h"
#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include "SDL.h"
#include "SDL_opengl.h"
#endif
#ifndef APIENTRY
#define	APIENTRY
#endif

#include "progs.h"
#include "server.h"

#include "console.h"
#include "wad.h"
#include "vid.h"
#include "screen.h"
#include "draw.h"
#include "render.h"
#include "view.h"
#include "sbar.h"
#include "q_sound.h"
#include "client.h"

#include "gl_model.h"
#include "world.h"

#include "image.h"	//johnfitz
#include "gl_texmgr.h"	//johnfitz
#include "input.h"
#include "keys.h"
#include "menu.h"
#include "cdaudio.h"
#include "glquake.h"


//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

extern qboolean noclip_anglehack;

//
// host
//
extern	quakeparms_t *host_parms;

extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;
extern	cvar_t		map_checks;
extern	cvar_t		max_edicts; //johnfitz

extern	qboolean	host_initialized;	// true if into command execution
extern	double		host_frametime;
extern	double		host_rawframetime;
extern	byte		*host_colormap;
extern	int		host_framecount;	// incremented every frame, never reset
extern	double		realtime;		// not bounded in any way, changed at
							// start of every frame, never reset

typedef struct filelist_item_s
{
	char			name[MAX_QPATH];
	struct filelist_item_s	*next;
} filelist_item_t;

typedef enum
{
	MAPTYPE_CUSTOM_MOD_START,
	MAPTYPE_CUSTOM_MOD_LEVEL,
	MAPTYPE_CUSTOM_MOD_END,
	MAPTYPE_CUSTOM_MOD_DM,

	MAPTYPE_MOD_START,
	MAPTYPE_MOD_LEVEL,
	MAPTYPE_MOD_END,
	MAPTYPE_MOD_DM,

	MAPTYPE_CUSTOM_ID_START,
	MAPTYPE_CUSTOM_ID_LEVEL,
	MAPTYPE_CUSTOM_ID_END,
	MAPTYPE_CUSTOM_ID_DM,

	MAPTYPE_ID_START,
	MAPTYPE_ID_EP1_LEVEL,
	MAPTYPE_ID_EP2_LEVEL,
	MAPTYPE_ID_EP3_LEVEL,
	MAPTYPE_ID_EP4_LEVEL,
	MAPTYPE_ID_END,
	MAPTYPE_ID_DM,
	MAPTYPE_ID_LEVEL,

	MAPTYPE_BMODEL,

	MAPTYPE_COUNT,
} maptype_t;

maptype_t			ExtraMaps_GetType (const filelist_item_t *item);
qboolean			ExtraMaps_IsStart (maptype_t type);
const char			*ExtraMaps_GetMessage (const filelist_item_t *item);

typedef enum
{
	MODSTATUS_DOWNLOADABLE,
	MODSTATUS_INSTALLING,
	MODSTATUS_INSTALLED,
} modstatus_t;

modstatus_t			Modlist_GetStatus (const filelist_item_t *item);
float				Modlist_GetDownloadProgress (const filelist_item_t *item);
double				Modlist_GetDownloadSize (const filelist_item_t *item);
const char			*Modlist_GetFullName (const filelist_item_t *item);
const char			*Modlist_GetDescription (const filelist_item_t *item);
const char			*Modlist_GetAuthor (const filelist_item_t *item);
const char			*Modlist_GetDate (const filelist_item_t *item);
qboolean			Modlist_StartInstalling (const filelist_item_t *item);
qboolean			Modlist_IsInstalling (void);

extern filelist_item_t **extralevels_sorted;
extern filelist_item_t	*modlist;
extern filelist_item_t	*extralevels;
extern filelist_item_t	*demolist;
extern filelist_item_t	*savelist;
extern filelist_item_t	*skylist;

void Host_ClearMemory (void);
void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (void);
void Host_Shutdown(void);
void Host_Callback_Notify (cvar_t *var);	/* callback function for CVAR_NOTIFY */

FUNC_NORETURN void Host_ReportError (const char *error, ...) FUNC_PRINTF(1,2);
FUNC_NORETURN void Host_EndGame (const char *message, ...) FUNC_PRINTF(1,2);
#ifdef __WATCOMC__
#pragma aux Host_Error aborts;
#pragma aux Host_EndGame aborts;
#endif

#define Host_Error(...)									\
	do													\
	{													\
		if (Sys_IsDebuggerPresent ())					\
			SDL_TriggerBreakpoint ();					\
		Host_ReportError (__VA_ARGS__);					\
	} while (0)											\

double Host_GetFrameInterval (void);
void Host_Frame (double time);
void Host_Quit_f (void);
void Host_ClientCommands (const char *fmt, ...) FUNC_PRINTF(1,2);
void Host_ShutdownServer (qboolean crash);
void Host_WriteConfiguration (void);
void Host_Resetdemos (void);

void Host_SavegameComment (char text[SAVEGAME_COMMENT_LENGTH + 1]);
void Host_WaitForSaveThread (void);
void Host_ShutdownSave (void);
qboolean Host_IsSaving (void);

void ExtraMaps_Init (void);
void Modlist_Init (void);
void DemoList_Init (void);
void SaveList_Init (void);
void SkyList_Init (void);

void ExtraMaps_Clear (void);
void ExtraMaps_ShutDown (void);

void DemoList_Rebuild (void);
void SaveList_Rebuild (void);
void SkyList_Rebuild (void);

void Modlist_ShutDown (void);


void M_CheckMods (void);

extern int		current_skill;	// skill level for currently loaded level (in case
					//  the user changes the cvar while the level is
					//  running, this reflects the level actually in use)

extern qboolean		isDedicated;

extern int		minimum_memory;

void Host_InvokeOnMainThread (void (*func) (void *param), void *param);

#endif /* RC_INVOKED */

#endif	/* QUAKEDEFS_H */

