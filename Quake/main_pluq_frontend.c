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

// main_pluq_frontend.c -- PluQ Frontend-specific main entry point
// Ironwail PluQ Frontend: A slim client that receives world state via PluQ

#include "quakedef.h"
#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif
#include <stdio.h>
#include "pluq.h"
#include "pluq_frontend.h"

static void Sys_AtExit (void)
{
	SDL_Quit();
}

static void Sys_InitSDL (void)
{
	SDL_version v;
	SDL_version *sdl_version = &v;
	SDL_GetVersion(&v);

	Sys_Printf("Found SDL version %i.%i.%i\n",sdl_version->major,sdl_version->minor,sdl_version->patch);

	if (SDL_Init(0) < 0) {
		Sys_Error("Couldn't init SDL: %s", SDL_GetError());
	}
	atexit(Sys_AtExit);
}

/*
==================
Sys_WaitUntil
==================
*/
static double Sys_WaitUntil (double endtime)
{
	static double estimate = 1e-3;
	static double mean = 1e-3;
	static double m2 = 0.0;
	static double count = 1.0;

	double now = Sys_DoubleTime ();
	double before, observed, delta, stddev;

	endtime -= 1e-6; // allow finishing 1 microsecond earlier than requested

	while (now + estimate < endtime)
	{
		before = now;
		SDL_Delay (1);
		now = Sys_DoubleTime ();

		// Determine Sleep(1) mean duration & variance using Welford's algorithm
		// https://blog.bearcats.nl/accurate-sleep-function/
		if (count < 1e6) // skip this if we already have more than enough samples
		{
			++count;
			observed = now - before;
			delta = observed - mean;
			mean += delta / count;
			m2 += delta * (observed - mean);
			stddev = sqrt (m2 / (count - 1.0));
			estimate = mean + 1.5 * stddev;

			// Previous frame-limiting code assumed a duration of 2 msec.
			// We don't want to burn more cycles in order to be more accurate
			// in case the actual duration is higher.
			estimate = q_min (estimate, 2e-3);
		}
	}

	while (now < endtime)
	{
#ifdef USE_SSE2
		_mm_pause (); _mm_pause (); _mm_pause (); _mm_pause ();
		_mm_pause (); _mm_pause (); _mm_pause (); _mm_pause ();
		_mm_pause (); _mm_pause (); _mm_pause (); _mm_pause ();
		_mm_pause (); _mm_pause (); _mm_pause (); _mm_pause ();
#endif
		now = Sys_DoubleTime ();
	}

	return now;
}

/*
==================
Sys_Throttle
==================
*/
static double Sys_Throttle (double oldtime)
{
	return Sys_WaitUntil (oldtime + Host_GetFrameInterval ());
}

/*
==================
Host_Frame_PluQ_Frontend

PluQ Frontend-specific frame processing
Simplified version that only handles PluQ reception and rendering
==================
*/
void Host_Frame_PluQ_Frontend (double time)
{
	host_framecount++;

	realtime += time;
	host_frametime = time;

	// Get new key events
	Key_UpdateForDest ();
	IN_UpdateInputMode ();
	Sys_SendKeyEvents ();

	// Allow mice or other external controllers to add commands
	IN_Commands ();

	// Process console commands
	Cbuf_Execute ();

	// PluQ Frontend: Accumulate input
	CL_AccumulateCmd ();

	// PluQ Frontend: Send input to backend via PluQ
	CL_SendCmd ();

	// PluQ Frontend: Receive world state from backend via PluQ
	if (PluQ_Frontend_ReceiveWorldState())
		PluQ_Frontend_ApplyReceivedState();

	// Update video
	SCR_UpdateScreen ();
	CL_RunParticles (); //johnfitz -- separated from rendering

	// Update audio
	if (cls.signon == SIGNONS)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update();
	BGM_Update();
}

// Forward declarations
void Host_Init_PluQ_Frontend (void);
void Host_Shutdown_PluQ_Frontend (void);

#define DEFAULT_MEMORY (384 * 1024 * 1024) // ericw -- was 72MB (64-bit) / 64MB (32-bit)

static quakeparms_t	parms;

// On OS X we call SDL_main from the launcher, but SDL2 doesn't redefine main
// as SDL_main on OS X anymore, so we do it ourselves.
#if defined(__APPLE__)
#define main SDL_main
#endif

int main(int argc, char *argv[])
{
	int		t;
	double		time, oldtime, newtime;

	host_parms = &parms;
	parms.basedir = ".";

	parms.argc = argc;
	parms.argv = argv;

	parms.errstate = 0;

	COM_InitArgv(parms.argc, parms.argv);

	Sys_InitSDL ();

	Sys_Init();

	Sys_Printf("======================================\n");
	Sys_Printf("Ironwail PluQ Frontend v%s\n", IRONWAIL_VER_STRING);
	Sys_Printf("Slim Client for Remote Rendering\n");
	Sys_Printf("======================================\n");

	parms.memsize = DEFAULT_MEMORY;
	if (COM_CheckParm("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;
		if (t < com_argc)
			parms.memsize = Q_atoi(com_argv[t]) * 1024;
	}

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

	Sys_Printf("Host_Init_PluQ_Frontend\n");
	Host_Init_PluQ_Frontend();

	oldtime = Sys_DoubleTime();

	// PluQ Frontend main loop
	while (1)
	{
		/* If we have no input focus at all, sleep a bit */
		if (!VID_HasMouseOrInputFocus() || cl.paused)
		{
			SDL_Delay(16);
		}
		/* If we're minimised, sleep a bit more */
		if (VID_IsMinimized())
		{
			scr_skipupdate = 1;
			SDL_Delay(32);
		}
		else
		{
			scr_skipupdate = 0;
		}

		newtime = Sys_Throttle (oldtime);
		time = newtime - oldtime;

		Host_Frame_PluQ_Frontend (time);

		oldtime = newtime;
	}

	return 0;
}
