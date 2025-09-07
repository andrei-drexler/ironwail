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

#ifndef QUAKE_SERVER_H
#define QUAKE_SERVER_H

// server.h

typedef struct
{
	int			maxclients;
	int			maxclientslimit;
	struct client_s	*clients;		// [maxclients]
	int			serverflags;		// episode completion information
	qboolean	changelevel_issued;	// cleared when at SV_SpawnServer
} server_static_t;

//=============================================================================

#define MAX_SIGNON_BUFFERS 256

typedef enum {ss_loading, ss_active} server_state_t;

typedef struct
{
	qboolean	active;				// false if only a net client

	qboolean	paused;
	qboolean	loadgame;			// handle connections specially
	qboolean	nomonsters;			// server started with 'nomonsters' cvar active

	int			lastcheck;			// used by PF_checkclient
	double		lastchecktime;

	qcvm_t		qcvm;				// Spike: entire qcvm state

	char		name[64];			// map name
	char		modelname[64];		// maps/<name>.bsp, for model_precache[0]
	struct qmodel_s	*worldmodel;
	const char	*model_precache[MAX_MODELS];	// NULL terminated
	struct qmodel_s	*models[MAX_MODELS];
	const char	*sound_precache[MAX_SOUNDS];	// NULL terminated
	const char	*lightstyles[MAX_LIGHTSTYLES];
	server_state_t	state;			// some actions are only valid during load

	sizebuf_t	datagram;
	byte		datagram_buf[MAX_DATAGRAM];

	sizebuf_t	reliable_datagram;	// copied to all clients at end of frame
	byte		reliable_datagram_buf[MAX_DATAGRAM];

	sizebuf_t	*signon;
	int			num_signon_buffers;
	sizebuf_t	*signon_buffers[MAX_SIGNON_BUFFERS];

	unsigned	protocol; //johnfitz
	unsigned	protocolflags;

	struct svcustomstat_s
	{
		int idx;
		int type;
		int fld;
		eval_t *ptr;
	} customstats[MAX_CL_STATS*2];	//strings or numeric...
	size_t		numcustomstats;

	char		lastsave[MAX_OSPATH];
	qboolean	autoloading;

	struct
	{
		float	secret_boost;
		float	teleport_boost;
		float	prev_health;
		int		prev_secrets;
		double	time;					// last autosave time
		double	hurt_time;				// last time the player was hurt
		double	shoot_time;				// last time the player attacked
		double	cheat;					// time spent with cheats active since last autosave
	}			autosave;

	struct
	{
		qboolean	active;
		int			numwarnings;

		const char	*changelevel;
		int			trigger_changelevel;
		int			valid_changelevel;
		int			intermission;
		int			skill_triggers;
		int			coop_spawns;
		int			dm_spawns;
		int			skill_ents[3];
	}			mapchecks;				// additional map checks (for level designers)
} server_t;


#define	NUM_PING_TIMES		16

enum sendsignon_e
{
	PRESPAWN_DONE,
	PRESPAWN_FLUSH=1,
	PRESPAWN_SIGNONBUFS,
	PRESPAWN_SIGNONMSG,
};

typedef struct client_s
{
	qboolean		active;				// false = client is free
	qboolean		spawned;			// false = don't send datagrams
	qboolean		dropasap;			// has been told to go to another level
	enum sendsignon_e	sendsignon;			// only valid before spawned
	int				signonidx;

	double			last_message;		// reliable messages must be sent
										// periodically

	struct qsocket_s *netconnection;	// communications handle

	usercmd_t		cmd;				// movement
	vec3_t			wishdir;			// intended motion calced from cmd

	sizebuf_t		message;			// can be added to at any time,
										// copied and clear once per frame
	byte			msgbuf[MAX_MSGLEN];
	edict_t			*edict;				// EDICT_NUM(clientnum+1)
	char			name[32];			// for printing to other people
	int				colors;

	float			ping_times[NUM_PING_TIMES];
	int				num_pings;			// ping_times[num_pings%NUM_PING_TIMES]

// spawn parms are carried from level to level
	float			spawn_parms[NUM_SPAWN_PARMS];

// client known data for deltas
	int				old_frags;

	int				oldstats_i[MAX_CL_STATS];		//previous values of stats. if these differ from the current values, reflag resendstats.
	float			oldstats_f[MAX_CL_STATS];		//previous values of stats. if these differ from the current values, reflag resendstats.
	char			*oldstats_s[MAX_CL_STATS];
} client_t;


//=============================================================================

// edict->movetype values
typedef enum
{
	MOVETYPE_NONE				= 0,	// never moves
	MOVETYPE_ANGLENOCLIP		= 1,
	MOVETYPE_ANGLECLIP			= 2,
	MOVETYPE_WALK				= 3,	// gravity
	MOVETYPE_STEP				= 4,	// gravity, special edge handling
	MOVETYPE_FLY				= 5,
	MOVETYPE_TOSS				= 6,	// gravity
	MOVETYPE_PUSH				= 7,	// no clip to world, push and crush
	MOVETYPE_NOCLIP				= 8,
	MOVETYPE_FLYMISSILE			= 9,	// extra size to monsters
	MOVETYPE_BOUNCE				= 10,
	MOVETYPE_GIB				= 11,	// 2021 rerelease gibs
} emovetype_t;

// edict->solid values
typedef enum
{
	SOLID_NOT					= 0,	// no interaction with other objects
	SOLID_TRIGGER				= 1,	// touch on edge, but not blocking
	SOLID_BBOX					= 2,	// touch on edge, block
	SOLID_SLIDEBOX				= 3,	// touch on edge, but not an onground
	SOLID_BSP					= 4,	// bsp clip, touch on edge, block
} esolid_t;

// edict->deadflag values
typedef enum
{
	DEAD_NO						= 0,
	DEAD_DYING					= 1,
	DEAD_DEAD					= 2,
	DEAD_RESPAWNABLE			= 3,
} edeadflag_t;

// edict->takedamage
typedef enum
{
	DAMAGE_NO					= 0,
	DAMAGE_YES					= 1,
	DAMAGE_AIM					= 2,
} etakedamage_t;

// edict->flags
typedef enum
{
	FL_FLY						= 1,
	FL_SWIM						= 2,
//	FL_GLIMPSE					= 4,
	FL_CONVEYOR					= 4,
	FL_CLIENT					= 8,
	FL_INWATER					= 16,
	FL_MONSTER					= 32,
	FL_GODMODE					= 64,
	FL_NOTARGET					= 128,
	FL_ITEM						= 256,
	FL_ONGROUND					= 512,
	FL_PARTIALGROUND			= 1024,	// not all corners are valid
	FL_WATERJUMP				= 2048,	// player jumping out of water
	FL_JUMPRELEASED				= 4096,	// for jump debouncing
} eflags_t;

// entity effects
typedef enum
{
	EF_BRIGHTFIELD				= 1,
	EF_MUZZLEFLASH 				= 2,
	EF_BRIGHTLIGHT 				= 4,
	EF_DIMLIGHT 				= 8,
	EF_QEX_QUADLIGHT			= 16,	// 2021 rerelease
	EF_QEX_PENTALIGHT			= 32,	// 2021 rerelease
	EF_QEX_CANDLELIGHT			= 64,	// 2021 rerelease
} efx_t;

// spawnflags
typedef enum
{
	SPAWNFLAG_NOT_EASY			= 256,
	SPAWNFLAG_NOT_MEDIUM		= 512,
	SPAWNFLAG_NOT_HARD			= 1024,
	SPAWNFLAG_NOT_DEATHMATCH	= 2048,
} spawnflags_t;

//============================================================================

extern	cvar_t	teamplay;
extern	cvar_t	skill;
extern	cvar_t	deathmatch;
extern	cvar_t	coop;
extern	cvar_t	fraglimit;
extern	cvar_t	timelimit;

extern	server_static_t	svs;				// persistant server info
extern	server_t		sv;					// local server

extern	client_t	*host_client;

extern	edict_t		*sv_player;

//===========================================================

void SV_Init (void);

void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound (edict_t *entity, int channel, const char *sample, int volume,
    float attenuation);
void SV_LocalSound (client_t *client, const char *sample); // for 2021 rerelease

void SV_DropClient (qboolean crash);

void SV_SendClientMessages (void);
void SV_ClearDatagram (void);
void SV_ReserveSignonSpace (int numbytes);

int SV_ModelIndex (const char *name);

void SV_SetIdealPitch (void);

void SV_AddUpdates (void);

void SV_ClientThink (void);
void SV_AddClientToServer (struct qsocket_s	*ret);

void SV_ClientPrintf (const char *fmt, ...) FUNC_PRINTF(1,2);
void SV_BroadcastPrintf (const char *fmt, ...) FUNC_PRINTF(1,2);

void SV_Physics (void);

qboolean SV_CheckBottom (edict_t *ent);
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink);

void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg);

void SV_MoveToGoal (void);

void SV_CheckForNewClients (void);
void SV_RunClients (void);
void SV_SaveSpawnparms (void);
void SV_SpawnServer (const char *server);

#endif	/* QUAKE_SERVER_H */
