/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

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

// stubs_pluq_frontend.c -- Stub implementations for excluded backend functions
// These are minimal no-op implementations for functions that may be called
// by client code but aren't needed in the PluQ frontend

#include "quakedef.h"

// Server global stubs
server_t sv = {0};
server_static_t svs = {0};
client_t *host_client = NULL;
edict_t *sv_player = NULL;
THREAD_LOCAL globalvars_t *pr_global_struct = NULL;

// Server stubs
void SV_Init (void) {}
void SV_Shutdown (void) {}
void SV_Frame (void) {}
qboolean SV_FilterPacket (void) { return false; }
void SV_ReadClientMessage (void) {}
void SV_RunClients (void) {}
void SV_SaveSpawnparms (void) {}
void SV_SpawnServer (const char *server) {}  // Frontend doesn't use startspot param
void SV_ClearDatagram (void) {}
void SV_SendClientMessages (void) {}
void SV_ClientPrintf (const char *fmt, ...)
{
	// In PluQ frontend mode, print to console instead
	va_list argptr;
	char text[1024];

	va_start (argptr, fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	Con_Printf ("%s", text);
}
void SV_LinkEdict (edict_t *ent, qboolean touch_triggers) {}
void SV_DropClient (qboolean crash) {}
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg) {}
void SV_BroadcastPrintf (const char *fmt, ...)
{
	// In PluQ frontend mode, just print to console
	va_list argptr;
	char text[1024];
	va_start (argptr, fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);
	Con_Printf ("%s", text);
}

// Network stubs (PluQ frontend uses PluQ only)
void NET_Init (void) {}
void NET_Shutdown (void) {}
void NET_Poll (void) {}
int NET_SendMessage (struct qsocket_s *sock, sizebuf_t *data) { return 0; }
int NET_SendUnreliableMessage (struct qsocket_s *sock, sizebuf_t *data) { return 0; }
qboolean NET_CanSendMessage (struct qsocket_s *sock) { return false; }
int NET_GetMessage (struct qsocket_s *sock) { return 0; }
struct qsocket_s *NET_Connect (const char *host) { return NULL; }
void NET_Close (struct qsocket_s *sock) {}
double NET_QSocketGetTime (const struct qsocket_s *sock) { return 0.0; }
const char *NET_QSocketGetAddressString (const struct qsocket_s *sock) { return ""; }
void NET_Slist_f (void) {}
const char *NET_SlistPrintServer (int idx) { return ""; }
const char *NET_SlistPrintServerName (int idx) { return ""; }
void NET_SlistSort (void) {}

// QuakeC/Progs stubs (no game logic in PluQ frontend)
void PR_Init (void) {}
void PR_Shutdown (void) {}
qboolean PR_LoadProgs (const char *filename, qboolean fatal) { return false; }
void PR_SwitchQCVM (qcvm_t *vm) {}
void PR_PushQCVM (qcvm_t *newvm, qcvm_t **oldvm) { if (oldvm) *oldvm = NULL; }
void PR_PopQCVM (qcvm_t *oldvm) {}
string_t PR_SetEngineString (const char *s) { return 0; }
const char *PR_GetString (string_t num) { return ""; }
void PR_ClearProgs (qcvm_t *vm) {}
void PR_ExecuteProgram (func_t fnum) {}
void PR_ReloadPics (qboolean no_overrides) {}
void PR_AutoCvarChanged (cvar_t *var) {}  // No QuakeC autocvar handling in frontend
const char *ED_FieldValueString (edict_t *ed, ddef_t *d) { return ""; }
qboolean ED_IsRelevantField (edict_t *ed, ddef_t *d) { return false; }

// World physics stubs
void SV_Physics (void) {}
void SV_CheckVelocity (edict_t *ent) {}
void SV_AddGravity (edict_t *ent) {}
void SV_Impact (edict_t *e1, edict_t *e2) {}
void SV_ClipMoveToEntity (edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end) {}
qboolean SV_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	if (trace) {
		trace->fraction = 1.0f;
		trace->allsolid = false;
	}
	return true;
}
qboolean SV_EdictInPVS (edict_t *ent) { return false; }
qboolean SV_BoxInPVS (vec3_t mins, vec3_t maxs) { return false; }
byte *SV_FatPVS (vec3_t org) { return NULL; }

// Host command stubs (some may call server functions)
void Host_ServerFrame (void) {}
void Host_ClearMemory (void) {}
void Host_InitLocal (void) {}  // Frontend doesn't need host command registration
void Host_Shutdown (void) {}  // Frontend has its own shutdown
void Host_ShutdownServer (qboolean crash) {}
void Host_WriteConfiguration (void) {}  // Frontend doesn't save configuration
void Host_InvokeOnMainThread (void (*func)(void *), void *data) {}  // No threading in frontend
void Host_EndGame (const char *message, ...)
{
	// In frontend, print error and exit
	va_list argptr;
	char text[1024];
	va_start (argptr, message);
	q_vsnprintf (text, sizeof(text), message, argptr);
	va_end (argptr);
	Con_Printf ("Game Ended: %s\n", text);
	exit(1);
}
void Host_ReportError (const char *fmt, ...)
{
	// In frontend, print errors and exit
	va_list argptr;
	char text[1024];
	va_start (argptr, fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);
	Con_Printf ("FATAL ERROR: %s\n", text);
	exit(1);
}
double Host_GetFrameInterval (void) { return 1.0 / 72.0; }  // Default 72 FPS

// Console command stubs that reference server
void Host_Map_f (void) { Con_Printf ("Map command not available in PluQ frontend mode\n"); }
void Host_Changelevel_f (void) { Con_Printf ("Changelevel command not available in PluQ frontend mode\n"); }
void Host_Restart_f (void) { Con_Printf ("Restart command not available in PluQ frontend mode\n"); }
void Host_Reconnect_f (void) { Con_Printf ("Reconnect command not available in PluQ frontend mode\n"); }
void Host_God_f (void) { Con_Printf ("God mode not available in PluQ frontend mode\n"); }
void Host_Notarget_f (void) { Con_Printf ("Notarget not available in PluQ frontend mode\n"); }
void Host_Noclip_f (void) { Con_Printf ("Noclip not available in PluQ frontend mode\n"); }
void Host_Fly_f (void) { Con_Printf ("Fly mode not available in PluQ frontend mode\n"); }
void Host_Ping_f (void) { Con_Printf ("Ping not available in PluQ frontend mode\n"); }
void Host_Kick_f (void) { Con_Printf ("Kick not available in PluQ frontend mode\n"); }
void Host_Give_f (void) { Con_Printf ("Give not available in PluQ frontend mode\n"); }

// Edict/World stubs
edict_t *EDICT_NUM(int n) { return NULL; }
int NUM_FOR_EDICT(edict_t *e) { return 0; }
void ED_Write (savedata_t *save, edict_t *ed) {}
void ED_ClearEdict (edict_t *e) {}
const char *ED_ParseEdict (const char *data, edict_t *ent) { return data; }
const char *ED_ParseGlobals (const char *data) { return data; }
eval_t *GetEdictFieldValueByName(edict_t *ed, const char *name) { return NULL; }

// Save/Load stubs
void SaveData_Clear (savedata_t *save) {}  // Frontend doesn't save
void SaveData_Init (savedata_t *save) {}
void SaveData_WriteHeader (savedata_t *save) {}
void SaveData_Fill (savedata_t *save) {}

// ============================================================================
// Global Variables (stubbed for frontend)
// ============================================================================

// Time variables
double realtime = 0;
double host_frametime = 0;
double host_rawframetime = 0;
double host_timescale = 1.0;
int host_framecount = 0;
double host_netinterval = 0.013; // ~72 FPS

// Host variables
quakeparms_t *host_parms = NULL;
qboolean host_initialized = false;
byte *host_colormap = NULL;
int minimum_memory = 0x1000000; // 16 MB minimum

// Development/Debug variables
cvar_t developer = {"developer", "0", CVAR_NONE};
cvar_t map_checks = {"map_checks", "0", CVAR_NONE};
cvar_t devstats = {"devstats", "0", CVAR_NONE};
devstats_t dev_stats = {0};
devstats_t dev_peakstats = {0};
overflowtimes_t dev_overflows = {0};

// Network variables
sizebuf_t net_message = {0};
cvar_t max_edicts = {"max_edicts", "8192", CVAR_NONE};

// Server cvars
cvar_t sv_gravity = {"sv_gravity", "800", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t sv_maxspeed = {"sv_maxspeed", "320", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t hostname = {"hostname", "UNNAMED", CVAR_SERVERINFO};
cvar_t teamplay = {"teamplay", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t pausable = {"pausable", "1", CVAR_NONE};
cvar_t nomonsters = {"nomonsters", "0", CVAR_NONE};
cvar_t skill = {"skill", "1", CVAR_NONE};
cvar_t coop = {"coop", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t fraglimit = {"fraglimit", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t timelimit = {"timelimit", "0", CVAR_NOTIFY | CVAR_SERVERINFO};
cvar_t host_maxfps = {"host_maxfps", "72", CVAR_ARCHIVE};

// Network globals
qboolean tcpipAvailable = false;
qboolean ipxAvailable = false;
char my_tcpip_address[64] = "";
char my_ipx_address[64] = "";
int net_activeconnections = 0;
double net_time = 0.0;
int net_hostport = 26000;
int DEFAULTnet_hostport = 26000;
qboolean slistInProgress = false;
qboolean slistSilent = false;
qboolean slistLocal = true;
int hostCacheCount = 0;

// QuakeC VM
THREAD_LOCAL qcvm_t *qcvm = NULL;
