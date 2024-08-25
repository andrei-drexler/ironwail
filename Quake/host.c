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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"
#include "bgmusic.h"
#include "steam.h"
#include <setjmp.h>

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t *host_parms;

qboolean	host_initialized;		// true if into command execution

double		host_frametime;
double		host_rawframetime;
double		realtime;				// without any filtering or bounding
double		oldrealtime;			// last frame run

int		host_framecount;

int		host_hunklevel;

int		minimum_memory;

client_t	*host_client;			// current client

jmp_buf 	host_abortserver;

byte		*host_colormap;
float	host_netinterval;
cvar_t	host_framerate = {"host_framerate","0",CVAR_NONE};	// set for slow motion
cvar_t	host_speeds = {"host_speeds","0",CVAR_NONE};			// set for running times
cvar_t	host_maxfps = {"host_maxfps", "250", CVAR_ARCHIVE}; //johnfitz
cvar_t	host_timescale = {"host_timescale", "0", CVAR_NONE}; //johnfitz
cvar_t	max_edicts = {"max_edicts", "16384", CVAR_NONE}; //johnfitz //ericw -- changed from 2048 to 8192, removed CVAR_ARCHIVE
cvar_t	cl_nocsqc = {"cl_nocsqc", "0", CVAR_NONE};	//spike -- blocks the loading of any csqc modules

cvar_t	sys_ticrate = {"sys_ticrate","0.05",CVAR_NONE}; // dedicated server
cvar_t	serverprofile = {"serverprofile","0",CVAR_NONE};

cvar_t	fraglimit = {"fraglimit","0",CVAR_NOTIFY|CVAR_SERVERINFO};
cvar_t	timelimit = {"timelimit","0",CVAR_NOTIFY|CVAR_SERVERINFO};
cvar_t	teamplay = {"teamplay","0",CVAR_NOTIFY|CVAR_SERVERINFO};
cvar_t	samelevel = {"samelevel","0",CVAR_NONE};
cvar_t	noexit = {"noexit","0",CVAR_NOTIFY|CVAR_SERVERINFO};
cvar_t	skill = {"skill","1",CVAR_NONE};			// 0 - 3
cvar_t	deathmatch = {"deathmatch","0",CVAR_NONE};	// 0, 1, or 2
cvar_t	coop = {"coop","0",CVAR_NONE};			// 0 or 1

cvar_t	pausable = {"pausable","1",CVAR_NONE};

cvar_t	developer = {"developer","0",CVAR_NONE};
cvar_t	map_checks = {"map_checks","0",CVAR_NONE};

cvar_t	temp1 = {"temp1","0",CVAR_NONE};

cvar_t devstats = {"devstats","0",CVAR_NONE}; //johnfitz -- track developer statistics that vary every frame
cvar_t cl_titlestats = {"cl_titlestats","1",CVAR_ARCHIVE};

cvar_t	campaign = {"campaign","0",CVAR_NONE}; // for the 2021 rerelease
cvar_t	horde = {"horde","0",CVAR_NONE}; // for the 2021 rerelease
cvar_t	sv_cheats = {"sv_cheats","0",CVAR_NONE}; // for the 2021 rerelease

cvar_t	sv_autosave = {"sv_autosave", "1", CVAR_ARCHIVE};
cvar_t	sv_autosave_interval = {"sv_autosave_interval", "30", CVAR_ARCHIVE};

devstats_t dev_stats, dev_peakstats;
overflowtimes_t dev_overflows; //this stores the last time overflow messages were displayed, not the last time overflows occured

/*
================
Max_Edicts_f -- johnfitz
================
*/
static void Max_Edicts_f (cvar_t *var)
{
	//TODO: clamp it here?
	if (cls.state == ca_connected || sv.active)
		Con_Printf ("Changes to max_edicts will not take effect until the next time a map is loaded.\n");
}

/*
================
Max_Fps_f -- ericw
================
*/
static void Max_Fps_f (cvar_t *var)
{
	if (var->value > 72 || var->value <= 0)
	{
		if (!host_netinterval)
			Con_Printf ("Using renderer/network isolation.\n");
		host_netinterval = 1.0/72;
	}
	else
	{
		if (host_netinterval)
			Con_Printf ("Disabling renderer/network isolation.\n");
		host_netinterval = 0;

		if (var->value > 72)
			Con_Warning ("host_maxfps above 72 breaks physics.\n");
	}
}

/*
================
Map_Checks_f -- called when map_checks cvar changes
================
*/
static void Map_Checks_f (cvar_t *var)
{
	static qboolean showed_message = false;
	if (!var->value || showed_message)
		return;
	showed_message = true;
	Con_SafePrintf ("\x02Note: ");
	Con_SafePrintf ("%s overrides gl_zfix, r_oit, and r_alphasort\n", var->name);
}

/*
================
Host_EndGame
================
*/
void Host_EndGame (const char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,message);
	q_vsnprintf (string, sizeof(string), message, argptr);
	va_end (argptr);
	Con_DPrintf ("Host_EndGame: %s\n",string);

	PR_SwitchQCVM(NULL);

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit

	if (cls.demonum != -1 && cls.demoloop)
		CL_NextDemo ();
	else
		CL_Disconnect ();

	longjmp (host_abortserver, 1);
}

/*
================
Host_ReportError

This shuts down both the client and server
================
*/
void Host_ReportError (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;

	PR_SwitchQCVM(NULL);

	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr,error);
	q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n",string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;
	cl.intermission = 0; //johnfitz -- for errors during intermissions (changelevel with no map found, etc.)

	inerror = false;

	longjmp (host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void	Host_FindMaxClients (void)
{
	int		i;

	svs.maxclients = 1;

	i = COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (com_argc - 1))
		{
			svs.maxclients = Q_atoi (com_argv[i+1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = COM_CheckParm ("-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");
		if (i != (com_argc - 1))
			svs.maxclients = Q_atoi (com_argv[i+1]);
		else
			svs.maxclients = 8;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = (struct client_s *) Hunk_AllocName (svs.maxclientslimit*sizeof(client_t), "clients");

	if (svs.maxclients > 1)
		Cvar_SetQuick (&deathmatch, "1");
	else
		Cvar_SetQuick (&deathmatch, "0");
}

void Host_Version_f (void)
{
	SDL_version sdl_linked;
	SDL_GetVersion (&sdl_linked);

	Con_Printf ("\n");
	Con_Printf ("Quake      %1.2f\n", VERSION);
	Con_Printf ("QuakeSpasm " QUAKESPASM_VER_STRING "\n");
	Con_Printf ("Ironwail   " IRONWAIL_VER_STRING "\n");
	Con_Printf ("Exe        " __TIME__ " " __DATE__ "\n");
	Con_Printf ("SDL        " Q_SDL_COMPILED_VERSION_STRING " (compiled)\n");
	Con_Printf ("           %d.%d.%d (linked)\n", sdl_linked.major, sdl_linked.minor, sdl_linked.patch);
	Con_Printf ("OS         %s %d-bit\n", SDL_GetPlatform (), (int)sizeof(void*)*8);
	Con_Printf ("\n");
}

/* cvar callback functions : */
void Host_Callback_Notify (cvar_t *var)
{
	if (sv.active)
		SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, var->string);
}

/*
===============
Host_WriteConfigurationToFile

Writes key bindings and archived cvars to specified file
===============
*/
void Host_WriteConfigurationToFile (const char *name)
{
	FILE	*f;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	if (host_initialized && !isDedicated && !host_parms->errstate)
	{
		char fullname[MAX_OSPATH];
		q_snprintf (fullname, sizeof (fullname), "%s/%s", com_gamedir, name);
		f = Sys_fopen (fullname, "w");
		if (!f)
		{
			Con_Printf ("Couldn't write %s.\n", name);
			return;
		}

		//VID_SyncCvars (); //johnfitz -- write actual current mode to config file, in case cvars were messed with

		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		//johnfitz -- extra commands to preserve state
		fprintf (f, "vid_restart\n");
		if (in_mlook.state & 1) fprintf (f, "+mlook\n");
		//johnfitz

		fclose (f);

		Con_SafePrintf ("Wrote ");
		Con_LinkPrintf (fullname, "%s", name);
		Con_SafePrintf (".\n");
	}
}

/*
=======================
Host_WriteConfig_f
======================
*/
void Host_WriteConfig_f (void)
{
	char filename[MAX_QPATH];
	q_strlcpy (filename, Cmd_Argc () >= 2 ? Cmd_Argv (1) : CONFIG_NAME, sizeof (filename));
	COM_AddExtension (filename, ".cfg", sizeof (filename));
	Host_WriteConfigurationToFile (filename);
}

/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal (void)
{
	Cmd_AddCommand ("version", Host_Version_f);
	Cmd_AddCommand ("writeconfig", Host_WriteConfig_f);

	Host_InitCommands ();

	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariable (&host_maxfps); //johnfitz
	Cvar_SetCallback (&host_maxfps, Max_Fps_f);
	Max_Fps_f (&host_maxfps);
	Cvar_RegisterVariable (&host_timescale); //johnfitz

	Cvar_RegisterVariable (&cl_nocsqc);	//spike
	Cvar_RegisterVariable (&max_edicts); //johnfitz
	Cvar_SetCallback (&max_edicts, Max_Edicts_f);
	Cvar_RegisterVariable (&devstats); //johnfitz

	Cvar_RegisterVariable (&cl_titlestats);

	Cvar_RegisterVariable (&sys_ticrate);
	Cvar_RegisterVariable (&serverprofile);

	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&timelimit);
	Cvar_RegisterVariable (&teamplay);
	Cvar_SetCallback (&fraglimit, Host_Callback_Notify);
	Cvar_SetCallback (&timelimit, Host_Callback_Notify);
	Cvar_SetCallback (&teamplay, Host_Callback_Notify);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&noexit);
	Cvar_SetCallback (&noexit, Host_Callback_Notify);
	Cvar_RegisterVariable (&skill);
	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&map_checks);
	Cvar_SetCallback (&map_checks, Map_Checks_f);
	Cvar_RegisterVariable (&coop);
	Cvar_RegisterVariable (&deathmatch);

	Cvar_RegisterVariable (&campaign);
	Cvar_RegisterVariable (&horde);
	Cvar_RegisterVariable (&sv_cheats);

	Cvar_RegisterVariable (&pausable);

	Cvar_RegisterVariable (&temp1);

	Host_FindMaxClients ();
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to engine config file
===============
*/
void Host_WriteConfiguration (void)
{
	Host_WriteConfigurationToFile (CONFIG_NAME);
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,fmt);
	q_vsnprintf (string, sizeof(string), fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			i;

	va_start (argptr,fmt);
	q_vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
	}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands (const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,fmt);
	q_vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient (qboolean crash)
{
	int		saveSelf;
	int		i;
	client_t *client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}

		if (host_client->edict && host_client->spawned)
		{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
			qcvm_t *oldvm = qcvm;
			PR_SwitchQCVM(NULL);
			PR_SwitchQCVM(&sv.qcvm);
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
			PR_SwitchQCVM(NULL);
			PR_SwitchQCVM(oldvm);
		}

		Sys_Printf ("Client %s removed\n",host_client->name);
	}

// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int		i;
	int		count;
	sizebuf_t	buf;
	byte		message[4];
	double	start;

	if (!sv.active)
		return;

	sv.active = false;

// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

// flush any pending messages - like the score!!!
	start = Sys_DoubleTime();
	do
	{
		count = 0;
		for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_DoubleTime() - start) > 3.0)
			break;
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.data = message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5.0);
	if (count)
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	PR_SwitchQCVM(&sv.qcvm);
	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		if (host_client->active)
			SV_DropClient(crash);

	PR_SwitchQCVM(NULL);

//
// clear structures
//
//	memset (&sv, 0, sizeof(sv)); // ServerSpawn already do this by Host_ClearMemory
	memset (svs.clients, 0, svs.maxclientslimit*sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (void)
{
	R_ClearBoundingBoxes ();

	if (cl.qcvm.extfuncs.CSQC_Shutdown)
	{
		PR_SwitchQCVM(&cl.qcvm);
		PR_ExecuteProgram(qcvm->extfuncs.CSQC_Shutdown);
		qcvm->extfuncs.CSQC_Shutdown = 0;
		PR_SwitchQCVM(NULL);
	}

	Con_DPrintf ("Clearing memory\n");
	Mod_ClearAll ();
	Sky_ClearAll();
	PR_ClearProgs(&sv.qcvm);
	PR_ClearProgs(&cl.qcvm);
/* host_hunklevel MUST be set at this point */
	Hunk_FreeToLowMark (host_hunklevel);
	cls.signon = 0; // not CL_ClearSignons()
	memset (&sv, 0, sizeof(sv));

	CL_FreeState ();
}


//==============================================================================
//
// Main thread APC queue
//
//==============================================================================

typedef struct asyncproc_s
{
	void				(*func) (void *param);
	void				*param;
} asyncproc_t;

typedef struct asyncqueue_s
{
	size_t				head;
	size_t				tail;
	size_t				capacity;
	qboolean			teardown;
	SDL_mutex			*mutex;
	SDL_cond			*notfull;
	asyncproc_t			*procs;
} asyncqueue_t;

static asyncqueue_t		async_queue;

static void AsyncQueue_Init (asyncqueue_t *queue, size_t capacity)
{
	memset (queue, 0, sizeof (*queue));

	if (!capacity)
		capacity = 1024;
	else
		capacity = Q_nextPow2 (capacity);
	queue->capacity = capacity;
	capacity *= sizeof (queue->procs[0]);
	queue->procs = (asyncproc_t *) malloc (capacity);
	if (!queue->procs)
		Sys_Error ("AsyncQueue_Init: malloc failed on %" SDL_PRIu64 " bytes", (uint64_t) capacity);

	queue->mutex = SDL_CreateMutex ();
	if (!queue->mutex)
		Sys_Error ("AsyncQueue_Init: could not create mutex");

	queue->notfull = SDL_CreateCond ();
	if (!queue->notfull)
		Sys_Error ("AsyncQueue_Init: could not create condition variable");
}

static void AsyncQueue_Push (asyncqueue_t *queue, void (*func) (void *param), void *param)
{
	asyncproc_t *proc;

	if (!queue->mutex)
		return;
	SDL_LockMutex (queue->mutex);
	while (!queue->teardown && queue->tail - queue->head >= queue->capacity)
		SDL_CondWait (queue->notfull, queue->mutex);

	if (!queue->teardown)
	{
		proc = &queue->procs[(queue->tail++) & (queue->capacity - 1)];
		proc->func = func;
		proc->param = param;
	}

	SDL_UnlockMutex (queue->mutex);
}

static void AsyncQueue_Drain (asyncqueue_t *queue)
{
	SDL_LockMutex (queue->mutex);
	while (queue->head != queue->tail)
	{
		asyncproc_t *proc = &queue->procs[(queue->head++) & (queue->capacity - 1)];
		proc->func (proc->param);
		SDL_CondSignal (queue->notfull);
	}
	SDL_UnlockMutex (queue->mutex);
}

static void AsyncQueue_Destroy (asyncqueue_t *queue)
{
	if (!queue->mutex)
		return;

	SDL_LockMutex (queue->mutex);
	queue->teardown = true;
	SDL_UnlockMutex (queue->mutex);
	SDL_CondBroadcast (queue->notfull);

	AsyncQueue_Drain (queue);

	SDL_DestroyCond (queue->notfull);
	SDL_DestroyMutex (queue->mutex);
	memset (queue, 0, sizeof (*queue));
}

void Host_InvokeOnMainThread (void (*func) (void *param), void *param)
{
	AsyncQueue_Push (&async_queue, func, param);
}

//==============================================================================
//
// Host Frame
//
//==============================================================================

/*
===================
Host_GetFrameInterval
===================
*/
double Host_GetFrameInterval (void)
{
	if ((host_maxfps.value || cls.state == ca_disconnected) && !cls.timedemo)
	{
		float maxfps;
		if (cls.state == ca_disconnected)
		{
			maxfps = vid.refreshrate ? vid.refreshrate : 60.f;
			if (host_maxfps.value)
				maxfps = q_min (maxfps, host_maxfps.value);
			maxfps = CLAMP (10.f, maxfps, 1000.f);
		}
		else
		{
			maxfps = CLAMP (10.f, host_maxfps.value, 1000.f);
		}

		return 1.0 / maxfps;
	}

	return 0.0;
}

/*
===================
Host_AdvanceTime
===================
*/
static void Host_AdvanceTime (double dt)
{
	realtime += dt;
	host_frametime = host_rawframetime = realtime - oldrealtime;
	oldrealtime = realtime;

	//johnfitz -- host_timescale is more intuitive than host_framerate
	if (host_timescale.value > 0)
		host_frametime *= host_timescale.value;
	//johnfitz
	else if (host_framerate.value > 0)
		host_frametime = host_framerate.value;
	else if (host_maxfps.value)// don't allow really long or short frames
		host_frametime = CLAMP (0.0001, host_frametime, 0.1); //johnfitz -- use CLAMP
}

/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands (void)
{
	const char	*cmd;

	if (!isDedicated)
		return;	// no stdin necessary in graphical mode

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}

/*
==================
Host_CheckAutosave
==================
*/
static void Host_CheckAutosave (void)
{
	float health_change, speed, elapsed, score;

	if (!sv_autosave.value || sv_autosave_interval.value <= 0.f || svs.maxclients != 1 || sv_player->v.health <= 0.f || cl.intermission)
		return;

	if (cls.signon == SIGNONS)
	{
		// Track new secrets
		if (pr_global_struct->found_secrets != sv.autosave.prev_secrets)
		{
			sv.autosave.prev_secrets = pr_global_struct->found_secrets;
			sv.autosave.secret_boost = 1.f;
		}
		else
			sv.autosave.secret_boost = q_max (0.f, sv.autosave.secret_boost - host_frametime / 1.5f);
	}

	// Track health changes
	if (!sv.autosave.prev_health)
		sv.autosave.prev_health = sv_player->v.health;
	health_change = sv_player->v.health - sv.autosave.prev_health;
	if (health_change < 0.f)
		if (health_change < -3.f || sv_player->v.health < 100.f || sv_player->v.watertype == CONTENTS_SLIME || sv_player->v.watertype == CONTENTS_LAVA)
			sv.autosave.hurt_time = qcvm->time;
	sv.autosave.prev_health = sv_player->v.health;

	// Track attacking
	if (sv_player->v.button0)
		sv.autosave.shoot_time = qcvm->time;

	// Time spent with cheats active doesn't count
	if (sv_player->v.movetype == MOVETYPE_NOCLIP || (int)sv_player->v.flags & (FL_GODMODE|FL_NOTARGET))
	{
		sv.autosave.cheat += host_frametime;
		return;
	}

	// Don't save if the player has been hurt recently
	if (qcvm->time - sv.autosave.hurt_time < 3.f)
		return;

	// Don't save if the player has fired recently
	if (qcvm->time - sv.autosave.shoot_time < 3.f)
		return;

	// Only save when the player slows down a bit
	speed = VectorLength (sv_player->v.velocity);
	if (speed > 100.f)
		return;

	// Copper's func_void holds the player at the bottom for a bit before inflicting damage,
	// so we can't assume it's safe to save just because we're no longer falling
	if ((int)sv_player->v.movetype == MOVETYPE_NONE)
		return;

	// Don't save too often
	elapsed = qcvm->time - sv.autosave.time - sv.autosave.cheat;
	if (elapsed < 3.f)
		return;

	// Compute a normalized autosave score

	// Base value is the fraction of the autosave interval already passed
	score = elapsed / sv_autosave_interval.value;
	// Scale down the score if health + armor is below 100 (save less often with lower health)
	score *= q_min (100.f, (sv_player->v.health + sv_player->v.armortype * sv_player->v.armorvalue)) / 100.f;
	// Boost the score right after picking up health
	score += q_max (0.f, health_change) / 100.f;
	// Lower score a bit based on speed (favor standing still/slowing down)
	score -= (speed / 100.f) * 0.25f;
	// Boost the score after finding a secret
	score += sv.autosave.secret_boost * 0.25f;
	// Boost the score after teleporting
	score += CLAMP (0.f, 1.f - (qcvm->time - sv_player->v.teleport_time) / 1.5f, 1.f) * 0.5f;

	// Only save if the score is high enough
	if (score < 1.f)
		return;

	sv.autosave.time = qcvm->time;
	sv.autosave.cheat = 0;
	Cbuf_AddText (va ("save \"autosave/%s\" 0\n", sv.name));
}

/*
==================
Host_ServerFrame
==================
*/
void Host_ServerFrame (void)
{
	int		i, active; //johnfitz
	edict_t	*ent; //johnfitz

// run the world state
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();

// check for new clients
	SV_CheckForNewClients ();

// read client messages
	SV_RunClients ();

// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();

//johnfitz -- devstats
	if (cls.signon == SIGNONS)
	{
		for (i=0, active=0; i<qcvm->num_edicts; i++)
		{
			ent = EDICT_NUM(i);
			if (!ent->free)
				active++;
		}
		if (active > 600 && dev_peakstats.edicts <= 600)
			Con_DWarning ("%i edicts exceeds standard limit of 600 (max = %d).\n", active, qcvm->max_edicts);
		dev_stats.edicts = active;
		dev_peakstats.edicts = q_max(active, dev_peakstats.edicts);
	}
//johnfitz

// send all messages to the clients
	SV_SendClientMessages ();

	Host_CheckAutosave ();
}

typedef struct summary_s {
	struct {
		int		players;
		int		max_players;
		int		skill;
		int		monsters;
		int		total_monsters;
		int		secrets;
		int		total_secrets;
	}			stats;
	char		map[countof (cl.mapname)];
} summary_t;

/*
==================
CountActiveClients
==================
*/
static int CountActiveClients (void)
{
	int i, active;
	for (i = active = 0; i < cl.maxclients; i++)
		if (cl.scores[i].name[0])
			active++;
	return active;
}

/*
==================
GetGameSummary
==================
*/
static void GetGameSummary (summary_t *s)
{
	if (!cl_titlestats.value || cls.state != ca_connected || cls.signon != SIGNONS)
	{
		s->map[0] = 0;
		memset (&s->stats, 0, sizeof (s->stats));
	}
	else
	{
		q_strlcpy (s->map, cl.mapname, countof (s->map));
		s->stats.players        = CountActiveClients ();
		s->stats.max_players    = cl.maxclients;
		s->stats.skill          = (int) skill.value;
		s->stats.monsters       = cl.stats[STAT_MONSTERS];
		s->stats.total_monsters = cl.stats[STAT_TOTALMONSTERS];
		s->stats.secrets        = cl.stats[STAT_SECRETS];
		s->stats.total_secrets  = cl.stats[STAT_TOTALSECRETS];
	}
}

/*
==================
UpdateWindowTitle
==================
*/
static void UpdateWindowTitle (void)
{
	static float timeleft = 0.f;
	static summary_t last = {{-1}}; // negative value to force initial update
	summary_t current;

	timeleft -= host_frametime;
	if (timeleft > 0.f)
		return;
	timeleft = 0.125f;

	GetGameSummary (&current);
	if (!strcmp (current.map, last.map) && !memcmp (&current.stats, &last.stats, sizeof (current.stats)))
		return;
	last = current;

	if (current.map[0])
	{
		char cleanname[sizeof (cl.levelname)];
		char utf8name[4 * sizeof (cl.levelname)];
		char title[1024];

		Mod_SanitizeMapDescription (cleanname, sizeof (cleanname), cl.levelname);

		UTF8_FromQuake (utf8name, sizeof (utf8name), cleanname);
		q_snprintf (title, sizeof (title),
			utf8name[0] ?
				"%s (%s)  |  skill %d  |  %d/%d kills  |  %d/%d secrets  -  " WINDOW_TITLE_STRING :
				"%s%s  |  skill %d  |  %d/%d kills  |  %d/%d secrets  -  " WINDOW_TITLE_STRING,
			utf8name, current.map,
			current.stats.skill,
			current.stats.monsters, current.stats.total_monsters,
			current.stats.secrets, current.stats.total_secrets
		);
		VID_SetWindowTitle (title);

		if (current.stats.max_players > 1)
			Steam_SetStatus_Multiplayer (current.stats.players, current.stats.max_players, utf8name[0] ? utf8name : current.map);
		else
			Steam_SetStatus_SinglePlayer (utf8name[0] ? utf8name : current.map);
	}
	else
	{
		VID_SetWindowTitle (WINDOW_TITLE_STRING);
		if (cls.state == ca_connected)
			Steam_ClearStatus ();
		else
			Steam_SetStatus_Menu ();
	}
}

static void CL_LoadCSProgs (void)
{
	PR_ClearProgs (&cl.qcvm);
	if (pr_checkextension.value && !cl_nocsqc.value)
	{ // only try to use csqc if qc extensions are enabled.
		PR_SwitchQCVM (&cl.qcvm);

		// try csprogs.dat first, then fall back on progs.dat in case someone tried merging the two.
		// we only care about it if it actually contains a CSQC_DrawHud, otherwise its either just a (misnamed) ssqc progs or a full csqc progs that would just
		// crash us on 3d stuff.
		if ((PR_LoadProgs ("csprogs.dat", false) && (qcvm->extfuncs.CSQC_DrawHud||qcvm->extfuncs.CSQC_DrawScores)) ||
		    (PR_LoadProgs ("progs.dat", false) && qcvm->extfuncs.CSQC_DrawHud))
		{
			qcvm->max_edicts = CLAMP (MIN_EDICTS, (int)max_edicts.value, MAX_EDICTS);
			qcvm->edicts = (edict_t *)malloc (qcvm->max_edicts * qcvm->edict_size);
			qcvm->num_edicts = qcvm->reserved_edicts = 1;
			memset (qcvm->edicts, 0, qcvm->num_edicts * qcvm->edict_size);

			if (!qcvm->extfuncs.CSQC_DrawHud)
			{ // no simplecsqc entry points... abort entirely!
				PR_ClearProgs (qcvm);
				PR_SwitchQCVM (NULL);
				return;
			}

			// set a few globals, if they exist
			if (qcvm->extglobals.maxclients)
				*qcvm->extglobals.maxclients = cl.maxclients;
			pr_global_struct->time = cl.time;
			pr_global_struct->mapname = PR_SetEngineString (cl.mapname);
			pr_global_struct->total_monsters = cl.stats[STAT_TOTALMONSTERS];
			pr_global_struct->total_secrets = cl.stats[STAT_TOTALSECRETS];
			pr_global_struct->deathmatch = cl.gametype;
			pr_global_struct->coop = (cl.gametype == GAME_COOP) && cl.maxclients != 1;
			if (qcvm->extglobals.player_localnum)
				*qcvm->extglobals.player_localnum = cl.viewentity - 1; // this is a guess, but is important for scoreboards.

			// set a few worldspawn fields too
			qcvm->edicts->v.solid = SOLID_BSP;
			qcvm->edicts->v.modelindex = 1;
			qcvm->edicts->v.model = PR_SetEngineString (cl.worldmodel->name);
			VectorCopy (cl.worldmodel->mins, qcvm->edicts->v.mins);
			VectorCopy (cl.worldmodel->maxs, qcvm->edicts->v.maxs);
			qcvm->edicts->v.message = PR_SetEngineString (cl.levelname);

			// and call the init function... if it exists.
			if (qcvm->extfuncs.CSQC_Init)
			{
				G_FLOAT (OFS_PARM0) = false;
				G_INT (OFS_PARM1) = PR_SetEngineString ("Ironwail");
				G_FLOAT (OFS_PARM2) = 10000 * IRONWAIL_VER_MAJOR + 100 * IRONWAIL_VER_MINOR + IRONWAIL_VER_PATCH;
				PR_ExecuteProgram (qcvm->extfuncs.CSQC_Init);
			}
		}
		else
			PR_ClearProgs (qcvm);
		PR_SwitchQCVM (NULL);
	}
}

/*
==================
Host_PrintTimes
==================
*/
static void Host_PrintTimes (const double times[], const char *names[], int count, qboolean showtotal)
{
	char line[1024];
	double total = 0.0;
	int i, worst;

	for (i = 0, worst = -1; i < count; i++)
	{
		if (worst == -1 || times[i] > times[worst])
			worst = i;
		total += times[i];
	}

	if (showtotal)
		q_snprintf (line, sizeof (line), "%5.2f tot | ", total * 1000.0);
	else
		line[0] = '\0';

	for (i = 0; i < count; i++)
	{
		char entry[256];
		q_snprintf (entry, sizeof (entry), "%5.2f %s", times[i] * 1000.0, names[i]);
		if (i == worst)
			COM_TintString (entry, entry, sizeof (entry));
		if (i != 0)
			q_strlcat (line, " | ", sizeof (line));
		q_strlcat (line, entry, sizeof (line));
	}

	Con_Printf ("%s\n", line);
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame (double time)
{
	static double	accumtime = 0;
	double time1, time2, time3;
	qboolean ranserver = false;

	time1 = Sys_DoubleTime ();

	if (setjmp (host_abortserver) )
		return;			// something bad happened, or the server disconnected

// keep the random time dependent
	rand ();

// decide the simulation time
	accumtime += host_netinterval?CLAMP(0.0, time, 0.2):0.0;	//for renderer/server isolation
	Host_AdvanceTime (time);

// run async procs
	AsyncQueue_Drain (&async_queue);

// get new key events
	Key_UpdateForDest ();
	IN_UpdateInputMode ();
	Sys_SendKeyEvents ();

// allow mice or other external controllers to add commands
	IN_Commands ();

//check the stdin for commands (dedicated servers)
	Host_GetConsoleCommands ();

// process console commands
	Cbuf_Execute ();

	NET_Poll();

	if (cl.sendprespawn)
	{
		CL_LoadCSProgs();

		cl.sendprespawn = false;
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");
		vid.recalc_refdef = true;
	}

	CL_AccumulateCmd ();

	//Run the server+networking (client->server->client), at a different rate from everyt
	if (accumtime >= host_netinterval)
	{
		float realframetime = host_frametime;
		if (host_netinterval)
		{
			host_frametime = q_max(accumtime, (double)host_netinterval);
			accumtime -= host_frametime;
			if (host_timescale.value > 0)
				host_frametime *= host_timescale.value;
			else if (host_framerate.value)
				host_frametime = host_framerate.value;
		}
		else
			accumtime -= host_netinterval;
		CL_SendCmd ();
		if (sv.active)
		{
			PR_SwitchQCVM(&sv.qcvm);
			Host_ServerFrame ();
			PR_SwitchQCVM(NULL);
		}
		host_frametime = realframetime;
		Cbuf_Waited();
		ranserver = true;
	}

// fetch results from server
	if (cls.state == ca_connected)
		CL_ReadFromServer ();

// update video
	if (host_speeds.value)
		time2 = Sys_DoubleTime ();

	SCR_UpdateScreen ();

	CL_RunParticles (); //johnfitz -- seperated from rendering

	if (host_speeds.value)
		time3 = Sys_DoubleTime ();

// update audio
	BGM_Update();	// adds music raw samples and/or advances midi driver
	if (cls.signon == SIGNONS)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update();
	UpdateWindowTitle();

	if (host_speeds.value)
	{
		static double pass[3] = {0.0, 0.0, 0.0};
		static double elapsed = 0.0;
		static int numframes = 0;
		static int numserverframes = 0;

		time1 = time2 - time1;
		time2 = time3 - time2;
		time3 = Sys_DoubleTime () - time3;

		if (ranserver || host_speeds.value < 0.f)
		{
			pass[0] += time1;
			numserverframes++;
		}
		numframes++;
		pass[1] += time2;
		pass[2] += time3;
		elapsed += time;

		if (elapsed >= host_speeds.value * 0.375)
		{
			const char *names[3] = {"server", "gfx", "snd"};
			pass[0] /= q_max (numserverframes, 1);
			pass[1] /= numframes;
			pass[2] /= numframes;

			Host_PrintTimes (pass, names, countof (pass), host_speeds.value < 0.f);

			pass[0] = pass[1] = pass[2] = elapsed = 0.0;
			numframes = numserverframes = 0;
		}
	}

	host_framecount++;
}

void Host_Frame (double time)
{
	double	time1, time2;
	static double	timetotal;
	static int		timecount;
	int		i, c, m;

	if (!serverprofile.value)
	{
		_Host_Frame (time);
		return;
	}

	time1 = Sys_DoubleTime ();
	_Host_Frame (time);
	time2 = Sys_DoubleTime ();

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
		return;

	m = timetotal*1000/timecount;
	timecount = 0;
	timetotal = 0;
	c = 0;
	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active)
			c++;
	}

	Con_Printf ("serverprofile: %2i clients %2i msec\n",  c,  m);
}

/*
====================
Host_Init
====================
*/
void Host_Init (void)
{
	if (standard_quake)
		minimum_memory = MINIMUM_MEMORY;
	else	minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (COM_CheckParm ("-minmemory"))
		host_parms->memsize = minimum_memory;

	if (host_parms->memsize < minimum_memory)
		Sys_Error ("Only %4.1f megs of memory available, can't execute game", host_parms->memsize / (float)0x100000);

	Memory_Init (host_parms->membase, host_parms->memsize);
	AsyncQueue_Init (&async_queue, 1024);
	Cbuf_Init ();
	Cmd_Init ();
	LOG_Init (host_parms);
	Cvar_Init (); //johnfitz
	COM_Init ();
	COM_InitFilesystem ();
	Host_InitLocal ();
	W_LoadWadFile (); //johnfitz -- filename is now hard-coded for honesty
	if (cls.state != ca_dedicated)
	{
		Key_Init ();
		Con_Init ();
	}
	PR_Init ();
	Mod_Init ();
	NET_Init ();
	SV_Init ();

	Con_Printf ("Exe: " __TIME__ " " __DATE__ " (%s %d-bit)\n", SDL_GetPlatform (), (int)sizeof(void*)*8);
	Con_Printf ("%4.1f megabyte heap\n", host_parms->memsize/ (1024*1024.0));

	if (cls.state != ca_dedicated)
	{
		host_colormap = (byte *)QFS_LoadHunkFile ("gfx/colormap.lmp", NULL, NULL);
		if (!host_colormap)
			Sys_Error ("Couldn't load gfx/colormap.lmp");

		V_Init ();
		Chase_Init ();
		M_Init ();
		VID_Init ();
		IN_Init ();
		TexMgr_Init (); //johnfitz
		Draw_Init ();
		SCR_Init ();
		R_Init ();
		S_Init ();
		CDAudio_Init ();
		BGM_Init();
		Sbar_Init ();
		CL_Init ();
		ExtraMaps_Init (); //johnfitz
		DemoList_Init (); //ericw
		SaveList_Init ();
		SkyList_Init ();
		M_CheckMods ();
	}

	LOC_Init (); // for 2021 rerelease support.

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	host_initialized = true;
	Con_Printf ("\n========= Quake Initialized =========\n\n");

	if (!COM_CheckParm ("-nomapchecks") && Sys_IsStartedFromMapEditor ())
	{
		Con_Printf (
			"Level editing environment detected, enabling map_checks\n"
			"(pass -nomapchecks or set map_checks to 0 to disable)\n"
		);
		Cvar_SetValueQuick (&map_checks, 1.f);
	}

	if (cls.state != ca_dedicated)
	{
		Cbuf_InsertText ("exec quake.rc\n");
	// johnfitz -- in case the vid mode was locked during vid_init, we can unlock it now.
		// note: two leading newlines because the command buffer swallows one of them.
		Cbuf_AddText ("\n\nvid_unlock\n");
	}

	if (cls.state == ca_dedicated)
	{
		Cbuf_AddText ("exec autoexec.cfg\n");
		Cbuf_AddText ("stuffcmds");
		Cbuf_Execute ();
		if (!sv.active)
			Cbuf_AddText ("map start\n");
	}
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Steam_Shutdown ();

	AsyncQueue_Destroy (&async_queue);

	Host_ShutdownSave ();
	Host_WriteConfiguration ();

// stop downloads before shutting down networking
	Modlist_ShutDown ();

	NET_Shutdown ();

	QFS_Shutdown();

	if (cls.state != ca_dedicated)
	{
		if (con_initialized)
			History_Shutdown ();
		ExtraMaps_ShutDown ();
		BGM_Shutdown();
		CDAudio_Shutdown ();
		S_Shutdown ();
		IN_Shutdown ();
		VID_Shutdown();
	}

	LOG_Close ();

	LOC_Shutdown ();
}

