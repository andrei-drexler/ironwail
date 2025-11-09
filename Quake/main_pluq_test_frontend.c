/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// main_pluq_test_frontend.c -- Headless PluQ Frontend for IPC Testing
// Reuses host_pluq_frontend.c initialization, replaces only the main loop

#include "quakedef.h"
#include "pluq.h"
#include "pluq_frontend.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Import from host_pluq_frontend.c
extern void Host_Init_PluQ_Frontend(void);
extern void Host_Shutdown_PluQ_Frontend(void);

#define DEFAULT_MEMORY (64 * 1024 * 1024) // Minimal heap for test

static quakeparms_t parms;

/*
==================
Sys_CheckStdinAvailable
==================
*/
static qboolean Sys_CheckStdinAvailable(void)
{
	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
}

/*
==================
Host_Frame_TestFrontend

Simplified frame: stdin → IPC → backend, backend → IPC → stdout
==================
*/
void Host_Frame_TestFrontend(double time)
{
	static char input_line[1024];
	static int input_pos = 0;
	static uint32_t frame_count = 0;

	frame_count++;

	// Read stdin commands
	while (Sys_CheckStdinAvailable())
	{
		char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
		{
			if (c == '\n' || c == '\r')
			{
				if (input_pos > 0)
				{
					input_line[input_pos] = 0;
					printf("[Test Frontend] → Backend: %s\n", input_line);

					// Forward to backend
					PluQ_Frontend_SendCommand(input_line);

					// Also execute locally
					Cbuf_AddText(input_line);
					Cbuf_AddText("\n");

					input_pos = 0;
				}
			}
			else if (input_pos < (int)sizeof(input_line) - 1)
			{
				input_line[input_pos++] = c;
			}
		}
	}

	Cbuf_Execute();

	// Receive world state
	if (PluQ_Frontend_ReceiveWorldState())
	{
		if (frame_count % 60 == 0)
			printf("[Test Frontend] ← Backend: World state (frame %u)\n", frame_count);

		PluQ_Frontend_ApplyReceivedState();
	}
}

int main(int argc, char *argv[])
{
	double time, oldtime, newtime;

	host_parms = &parms;
	parms.basedir = ".";
	parms.argc = argc;
	parms.argv = argv;
	parms.errstate = 0;

	COM_InitArgv(parms.argc, parms.argv);

	Sys_Init();

	printf("======================================\n");
	printf("PluQ Test Frontend (Headless)\n");
	printf("======================================\n");

	parms.memsize = DEFAULT_MEMORY;
	parms.membase = malloc(parms.memsize);

	if (!parms.membase)
		Sys_Error("Not enough memory\n");

	// Reuse existing frontend initialization
	Host_Init_PluQ_Frontend();

	printf("\n[Type commands and press Enter]\n");
	printf("Examples: map e1m1, skill 2, god\n\n");

	// Set stdin to non-blocking
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

	oldtime = Sys_DoubleTime();

	// Main loop - simplified version of Host_Frame_PluQ_Frontend
	while (1)
	{
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		if (time < 0.016)
		{
			usleep(1000);
			continue;
		}

		Host_Frame_TestFrontend(time);

		oldtime = newtime;
	}

	Host_Shutdown_PluQ_Frontend();

	return 0;
}
