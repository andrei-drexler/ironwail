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
client_t *host_client = NULL;
edict_t *sv_player = NULL;

// Server stubs
void SV_Init (void) {}
void SV_Shutdown (void) {}
void SV_Frame (void) {}
qboolean SV_FilterPacket (void) { return false; }
void SV_ReadClientMessage (void) {}
void SV_RunClients (void) {}
void SV_SaveSpawnparms (void) {}
void SV_SpawnServer (const char *server, const char *startspot) {}
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

// Network stubs (PluQ frontend uses PluQ only)
void NET_Init (void) {}
void NET_Shutdown (void) {}
void NET_Poll (void) {}
int NET_SendMessage (qsocket_t *sock, sizebuf_t *data) { return 0; }
int NET_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data) { return 0; }
qboolean NET_CanSendMessage (qsocket_t *sock) { return false; }
int NET_GetMessage (qsocket_t *sock) { return 0; }
qsocket_t *NET_Connect (const char *host) { return NULL; }
void NET_Close (qsocket_t *sock) {}

// QuakeC/Progs stubs (no game logic in PluQ frontend)
void PR_Init (void) {}
void PR_Shutdown (void) {}
void PR_LoadProgs (const char *filename, qboolean fatal) {}
void PR_SwitchQCVM (void *vm) {}
string_t PR_SetEngineString (const char *s) { return 0; }

// Model/World loading stubs (PluQ frontend receives from PluQ)
void Mod_Init (void) {}
void Mod_ClearAll (void) {}
void Mod_Shutdown (void) {}
qboolean Mod_LoadMapDescription (char *out, size_t outsize, const char *mapname) { return false; }

// World physics stubs
void SV_Physics (void) {}
void SV_CheckVelocity (edict_t *ent) {}
void SV_AddGravity (edict_t *ent) {}
void SV_Impact (edict_t *e1, edict_t *e2) {}
void SV_ClipMoveToEntity (edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end) {}

// Extra map/demo/save stubs
void ExtraMaps_Init (void) {}
void ExtraMaps_ShutDown (void) {}
void DemoList_Init (void) {}
void SaveList_Init (void) {}
void SkyList_Init (void) {}
void M_CheckMods (void) {}
void Modlist_ShutDown (void) {}

// Host command stubs (some may call server functions)
void Host_ServerFrame (void) {}
void Host_ClearMemory (void) {}

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
