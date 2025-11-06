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

// host_pluq_frontend.c -- PluQ Frontend-specific host initialization
// This is a slim client that receives world state via PluQ and only handles rendering/input

#include "quakedef.h"
#include "pluq.h"

extern cvar_t pausable;

/*
==================
Host_Init_PluQ_Frontend

Minimal initialization for PluQ frontend (slim client)
Skips server, networking, and level loading
==================
*/
void Host_Init_PluQ_Frontend (void)
{
	minimum_memory = MINIMUM_MEMORY;

	if (COM_CheckParm ("-heapsize"))
	{
		int heapsize = Q_atoi (com_argv[COM_CheckParm("-heapsize")+1]) * 1024;
		Con_Printf ("Requested heap size: %i kb\n", heapsize/1024);
	}

	if (host_parms->memsize < minimum_memory)
		Sys_Error ("Only %4.1f megs of memory available, can't execute game", host_parms->memsize / (float)0x100000);

	// Note: host_memsize not needed in frontend
	Memory_Init (host_parms->membase, host_parms->memsize);
	Cbuf_Init ();
	Cmd_Init ();
	Cvar_Init (); //johnfitz
	COM_Init ();
	COM_InitFilesystem ();
	Host_InitLocal ();  // Stubbed in stubs_pluq_frontend.c
	W_LoadWadFile (); //johnfitz -- filename is now hard-coded for honesty

	Key_Init ();
	Con_Init ();

	// PluQ Frontend mode: Skip PR_Init() - no QuakeC VM needed
	// PluQ Frontend mode: Skip Mod_Init() - no level loading needed
	// PluQ Frontend mode: Skip NET_Init() - no networking needed
	// PluQ Frontend mode: Skip SV_Init() - no server needed

	Con_Printf ("Exe: " __TIME__ " " __DATE__ " (%s %d-bit)\n", SDL_GetPlatform (), (int)sizeof(void*)*8);
	Con_Printf ("%4.1f megabyte heap\n", host_parms->memsize/ (1024*1024.0));
	Con_Printf ("PluQ Frontend mode: Slim client (rendering only)\n");

	host_colormap = (byte *)COM_LoadHunkFile ("gfx/colormap.lmp", NULL);
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

	// PluQ Frontend doesn't need these:
	// ExtraMaps_Init (); //johnfitz
	// DemoList_Init (); //ericw
	// SaveList_Init ();
	// SkyList_Init ();
	// M_CheckMods ();

	LOC_Init (); // for 2021 rerelease support.

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	// Note: host_hunklevel tracking not needed in frontend

	// Initialize PluQ in frontend mode
	PluQ_Init ();
	if (!PluQ_Initialize(PLUQ_MODE_FRONTEND))
	{
		Sys_Error ("Failed to initialize PluQ in frontend mode");
	}

	host_initialized = true;
	Con_Printf ("\n========= Ironwail PluQ Frontend Initialized =========\n\n");
	Con_Printf ("Waiting for backend connection...\n");

	// PluQ Frontend executes minimal config
	Cbuf_InsertText ("exec quake.rc\n");
	// note: two leading newlines because the command buffer swallows one of them.
	Cbuf_AddText ("\n\nvid_unlock\n");

	// Set cls.state to connected so we can receive PluQ data
	cls.state = ca_connected;

	// Disable pause in frontend (backend controls this)
	Cvar_SetValue ("pausable", 0);
}

/*
==================
Host_Shutdown_PluQ_Frontend

PluQ Frontend-specific shutdown
==================
*/
void Host_Shutdown_PluQ_Frontend(void)
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

	PluQ_Shutdown (); // Shutdown PluQ subsystem

	// Note: Steam and AsyncQueue not needed in frontend

	Host_WriteConfiguration ();

	// PluQ Frontend doesn't have networking or modlist
	// Modlist_ShutDown ();
	// NET_Shutdown ();

	if (con_initialized)
		History_Shutdown ();
	// Note: Audio shutdown handled by main shutdown path
	S_Shutdown ();
	IN_Shutdown ();
	VID_Shutdown();

	LOC_Shutdown ();
}
