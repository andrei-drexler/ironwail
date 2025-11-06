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

// ipc.c -- Inter-Process Communication implementation

#include "quakedef.h"
#include "ipc.h"

#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#else
#include <windows.h>
#endif

// Console variables
static cvar_t pluq_headless = {"pluq_headless", "0", CVAR_NONE};

// Global state
static qboolean ipc_initialized = false;
static ipc_mode_t ipc_mode = IPC_MODE_DISABLED;
static ipc_shared_memory_t *shared_mem = NULL;
static ipc_stats_t perf_stats = {0};

// Current input from frontend (for backend mode)
static ipc_input_cmd_t current_input = {0};
static qboolean has_current_input = false;

// Last received frame (for frontend mode)
static uint32_t last_received_frame = 0;

#ifndef _WIN32
static int shm_fd = -1;
static const char *shm_name = "/quake_ipc_ironwail";
#else
static HANDLE hMapFile = NULL;
static const char *shm_name = "Local\\quake_ipc_ironwail";
#endif

// Forward declarations
static qboolean IPC_InitializeSharedMemory(void);
static void IPC_CleanupSharedMemory(void);

/*
==================
IPC_Initialize

Initialize IPC system with specified mode
==================
*/
qboolean IPC_Initialize(ipc_mode_t mode)
{
	if (ipc_initialized && ipc_mode != IPC_MODE_DISABLED)
	{
		Con_Printf("IPC already initialized in mode %d\n", ipc_mode);
		return true;
	}

	if (mode == IPC_MODE_DISABLED)
	{
		Con_Printf("Cannot initialize IPC in disabled mode\n");
		return false;
	}

	Con_Printf("Initializing PluQ IPC system in mode: ");
	switch (mode)
	{
		case IPC_MODE_BACKEND:   Con_Printf("BACKEND\n"); break;
		case IPC_MODE_FRONTEND:  Con_Printf("FRONTEND\n"); break;
		case IPC_MODE_BOTH:      Con_Printf("BOTH\n"); break;
		default:                  Con_Printf("UNKNOWN\n"); break;
	}

	if (!IPC_InitializeSharedMemory())
	{
		Con_Printf("Failed to initialize shared memory\n");
		return false;
	}

	// Backend initializes shared memory structure
	if (mode == IPC_MODE_BACKEND || mode == IPC_MODE_BOTH)
	{
		memset(shared_mem, 0, sizeof(ipc_shared_memory_t));
		shared_mem->frame_sequence = 0;
		shared_mem->write_in_progress = 0;
		shared_mem->input_ready = 0;
		shared_mem->header.max_entities = IPC_MAX_ENTITIES;
	}

	// Reset statistics
	memset(&perf_stats, 0, sizeof(ipc_stats_t));
	last_received_frame = 0;

	ipc_initialized = true;
	ipc_mode = mode;

	Con_Printf("PluQ IPC system initialized successfully\n");
	return true;
}

/*
==================
IPC_Shutdown

Shutdown IPC system
==================
*/
void IPC_Shutdown(void)
{
	if (!ipc_initialized)
		return;

	Con_Printf("Shutting down PluQ IPC system...\n");

	IPC_CleanupSharedMemory();

	ipc_initialized = false;
	ipc_mode = IPC_MODE_DISABLED;
	shared_mem = NULL;

	Con_Printf("PluQ IPC system shut down\n");
}

/*
==================
IPC_GetMode

Get current IPC mode
==================
*/
ipc_mode_t IPC_GetMode(void)
{
	return ipc_mode;
}

/*
==================
IPC_SetMode

Set IPC mode (requires reinitialization)
==================
*/
void IPC_SetMode(ipc_mode_t mode)
{
	if (ipc_initialized && mode != ipc_mode)
	{
		Con_Printf("Switching PluQ mode requires reinitialization\n");
		IPC_Shutdown();
		if (mode != IPC_MODE_DISABLED)
			IPC_Initialize(mode);
	}
	else if (!ipc_initialized && mode != IPC_MODE_DISABLED)
	{
		IPC_Initialize(mode);
	}
}

/*
==================
IPC_IsEnabled

Check if IPC is enabled and active
==================
*/
qboolean IPC_IsEnabled(void)
{
	return ipc_initialized && ipc_mode != IPC_MODE_DISABLED && shared_mem != NULL;
}

/*
==================
IPC_IsBackend

Check if running in backend mode
==================
*/
qboolean IPC_IsBackend(void)
{
	return IPC_IsEnabled() && (ipc_mode == IPC_MODE_BACKEND || ipc_mode == IPC_MODE_BOTH);
}

/*
==================
IPC_IsFrontend

Check if running in frontend mode
==================
*/
qboolean IPC_IsFrontend(void)
{
	return IPC_IsEnabled() && (ipc_mode == IPC_MODE_FRONTEND || ipc_mode == IPC_MODE_BOTH);
}

/*
==================
IPC_IsHeadless

Check if running in headless mode (backend without rendering/input)
==================
*/
qboolean IPC_IsHeadless(void)
{
	return IPC_IsBackend() && pluq_headless.value != 0;
}

/*
==================
IPC_InitializeSharedMemory

Platform-specific shared memory initialization
==================
*/
static qboolean IPC_InitializeSharedMemory(void)
{
	size_t size = sizeof(ipc_shared_memory_t);

#ifndef _WIN32
	// Unix/Linux implementation using POSIX shared memory

	// First, try to unlink any existing shared memory
	shm_unlink(shm_name);

	// Create shared memory object
	shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
	if (shm_fd == -1)
	{
		Con_Printf("IPC: Failed to create shared memory: %s\n", strerror(errno));
		return false;
	}

	// Set the size
	if (ftruncate(shm_fd, size) == -1)
	{
		Con_Printf("IPC: Failed to set shared memory size: %s\n", strerror(errno));
		close(shm_fd);
		shm_unlink(shm_name);
		return false;
	}

	// Map the shared memory
	shared_mem = (ipc_shared_memory_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
	                                          MAP_SHARED, shm_fd, 0);
	if (shared_mem == MAP_FAILED)
	{
		Con_Printf("IPC: Failed to map shared memory: %s\n", strerror(errno));
		close(shm_fd);
		shm_unlink(shm_name);
		shared_mem = NULL;
		return false;
	}

	Con_Printf("IPC: Shared memory created at %s (%zu bytes)\n", shm_name, size);
	return true;

#else
	// Windows implementation using CreateFileMapping

	hMapFile = CreateFileMappingA(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		size,                    // maximum object size (low-order DWORD)
		shm_name                 // name of mapping object
	);

	if (hMapFile == NULL)
	{
		Con_Printf("IPC: Failed to create file mapping: %d\n", GetLastError());
		return false;
	}

	shared_mem = (ipc_shared_memory_t *)MapViewOfFile(
		hMapFile,                // handle to map object
		FILE_MAP_ALL_ACCESS,     // read/write permission
		0,
		0,
		size
	);

	if (shared_mem == NULL)
	{
		Con_Printf("IPC: Failed to map view of file: %d\n", GetLastError());
		CloseHandle(hMapFile);
		hMapFile = NULL;
		return false;
	}

	Con_Printf("IPC: Shared memory created at %s (%zu bytes)\n", shm_name, size);
	return true;
#endif
}

/*
==================
IPC_CleanupSharedMemory

Platform-specific shared memory cleanup
==================
*/
static void IPC_CleanupSharedMemory(void)
{
	if (shared_mem == NULL)
		return;

#ifndef _WIN32
	// Unix/Linux cleanup
	munmap(shared_mem, sizeof(ipc_shared_memory_t));
	close(shm_fd);
	shm_unlink(shm_name);
	shm_fd = -1;
#else
	// Windows cleanup
	UnmapViewOfFile(shared_mem);
	CloseHandle(hMapFile);
	hMapFile = NULL;
#endif

	shared_mem = NULL;
}

/*
==================
IPC_BroadcastWorldState

Broadcast current world state to frontend via shared memory
==================
*/
void IPC_BroadcastWorldState(void)
{
	if (!IPC_IsEnabled())
		return;

	double start_time = Sys_DoubleTime();

	// Set write flag to prevent frontend from reading during update
	shared_mem->write_in_progress = 1;

	// Update frame header
	shared_mem->header.frame_number = host_framecount;
	shared_mem->header.timestamp = realtime;
	shared_mem->header.paused = cl.paused;
	shared_mem->header.in_game = (cls.state == ca_connected && cl.worldmodel != NULL);

	// Copy map name
	if (cl.worldmodel)
		q_strlcpy(shared_mem->header.mapname, cl.worldmodel->name, sizeof(shared_mem->header.mapname));
	else
		shared_mem->header.mapname[0] = '\0';

	// Update player state
	if (cls.state == ca_connected && cl.viewentity > 0 && cl.viewentity < cl.num_entities)
	{
		VectorCopy(cl.entities[cl.viewentity].origin, shared_mem->header.player_origin);
		VectorCopy(cl.viewangles, shared_mem->header.player_angles);
		shared_mem->header.player_health = cl.stats[STAT_HEALTH];
		shared_mem->header.player_armor = cl.stats[STAT_ARMOR];
		shared_mem->header.player_weapon = cl.stats[STAT_WEAPON];
		shared_mem->header.player_ammo = cl.stats[STAT_AMMO];
	}
	else
	{
		VectorCopy(vec3_origin, shared_mem->header.player_origin);
		VectorCopy(vec3_origin, shared_mem->header.player_angles);
		shared_mem->header.player_health = 0;
		shared_mem->header.player_armor = 0;
		shared_mem->header.player_weapon = 0;
		shared_mem->header.player_ammo = 0;
	}

	// Copy entity data (zero-copy approach - direct memcpy)
	uint16_t num_entities = q_min(cl.num_entities, IPC_MAX_ENTITIES);
	shared_mem->header.num_entities = num_entities;

	if (num_entities > 0)
	{
		memcpy(shared_mem->entities, cl_entities,
		       sizeof(entity_t) * num_entities);
	}

	// Copy dynamic lights
	uint16_t num_dlights = 0;
	for (int i = 0; i < MAX_DLIGHTS; i++)
	{
		if (cl_dlights[i].die > cl.time)
		{
			shared_mem->dlights[num_dlights++] = cl_dlights[i];
		}
	}
	shared_mem->num_dlights = num_dlights;

	// Increment frame sequence number (signals frontend that new data is available)
	shared_mem->frame_sequence++;

	// Clear write flag
	shared_mem->write_in_progress = 0;

	// Update performance statistics
	double end_time = Sys_DoubleTime();
	double frame_time = end_time - start_time;

	perf_stats.frames_sent++;
	perf_stats.total_time += frame_time;
	perf_stats.total_entities += num_entities;

	if (frame_time > perf_stats.max_frame_time || perf_stats.max_frame_time == 0.0)
		perf_stats.max_frame_time = frame_time;

	if (frame_time < perf_stats.min_frame_time || perf_stats.min_frame_time == 0.0)
		perf_stats.min_frame_time = frame_time;
}

/*
==================
IPC_ReceiveWorldState

Frontend: Receive world state from backend via shared memory
Returns true if new data was received
==================
*/
qboolean IPC_ReceiveWorldState(void)
{
	if (!IPC_IsFrontend())
		return false;

	// Wait for backend to finish writing
	if (shared_mem->write_in_progress)
		return false;

	// Check if we have a new frame
	uint32_t current_frame = shared_mem->frame_sequence;
	if (current_frame == last_received_frame)
		return false;  // No new data

	last_received_frame = current_frame;
	return true;
}

/*
==================
IPC_ApplyReceivedState

Frontend: Apply received state to local client for rendering
==================
*/
void IPC_ApplyReceivedState(void)
{
	if (!IPC_IsFrontend() || !shared_mem)
		return;

	// Wait for backend to finish writing
	if (shared_mem->write_in_progress)
		return;

	// Apply player state
	VectorCopy(shared_mem->header.player_origin, cl.viewent.origin);
	VectorCopy(shared_mem->header.player_angles, cl.viewangles);
	cl.stats[STAT_HEALTH] = shared_mem->header.player_health;
	cl.stats[STAT_ARMOR] = shared_mem->header.player_armor;
	cl.stats[STAT_WEAPON] = shared_mem->header.player_weapon;
	cl.stats[STAT_AMMO] = shared_mem->header.player_ammo;

	// Apply paused state
	cl.paused = shared_mem->header.paused;

	// Copy entities for rendering
	uint16_t num_entities = shared_mem->header.num_entities;
	if (num_entities > 0)
	{
		cl.num_entities = q_min(num_entities, MAX_EDICTS);
		memcpy(cl_entities, shared_mem->entities,
		       sizeof(entity_t) * cl.num_entities);
	}

	// Copy dynamic lights
	for (int i = 0; i < shared_mem->num_dlights && i < MAX_DLIGHTS; i++)
	{
		cl_dlights[i] = shared_mem->dlights[i];
	}
}

/*
==================
IPC_SendInput

Frontend: Send input to backend
==================
*/
void IPC_SendInput(usercmd_t *cmd)
{
	if (!IPC_IsFrontend())
		return;

	// Prepare input command
	ipc_input_cmd_t input;
	memset(&input, 0, sizeof(input));

	input.sequence = host_framecount;
	input.timestamp = Sys_DoubleTime();

	// Movement
	if (cmd)
	{
		input.forward_move = cmd->forwardmove;
		input.side_move = cmd->sidemove;
		input.up_move = cmd->upmove;
	}

	// View angles
	VectorCopy(cl.viewangles, input.view_angles);

	// Buttons
	input.buttons = 0;
	if (in_attack.state & 1) input.buttons |= 1;
	if (in_jump.state & 1) input.buttons |= 2;
	if (in_use.state & 1) input.buttons |= 4;

	// Impulse
	input.impulse = in_impulse;

	// Copy to shared memory
	memcpy(&shared_mem->input_cmd, &input, sizeof(ipc_input_cmd_t));

	// Signal backend
	shared_mem->input_ready = 1;
}

/*
==================
IPC_HasPendingInput

Check if there is pending input from frontend (backend only)
==================
*/
qboolean IPC_HasPendingInput(void)
{
	if (!IPC_IsBackend())
		return false;

	return (shared_mem->input_ready != 0);
}

/*
==================
IPC_ProcessInputCommands

Process input commands from frontend (backend only)
==================
*/
void IPC_ProcessInputCommands(void)
{
	if (!IPC_IsBackend() || !IPC_HasPendingInput())
		return;

	// Copy input command from shared memory
	memcpy(&current_input, &shared_mem->input_cmd, sizeof(ipc_input_cmd_t));
	has_current_input = true;

	// Clear ready flag
	shared_mem->input_ready = 0;

	// Process console commands if any
	if (current_input.cmd_text[0] != '\0')
	{
		Cbuf_AddText(current_input.cmd_text);
		Cbuf_AddText("\n");
	}

	// Movement, view angles, and buttons are processed in IPC_Move() and IPC_ApplyViewAngles()
}

/*
==================
IPC_Move

Apply IPC input to movement command (backend only)
Called during input accumulation phase
==================
*/
void IPC_Move(usercmd_t *cmd)
{
	if (!IPC_IsBackend() || !has_current_input)
		return;

	// Add movement from IPC frontend
	cmd->forwardmove += current_input.forward_move;
	cmd->sidemove += current_input.side_move;
	cmd->upmove += current_input.up_move;
}

/*
==================
IPC_ApplyViewAngles

Apply IPC view angles and buttons to client (backend only)
Called during input accumulation phase
==================
*/
void IPC_ApplyViewAngles(void)
{
	if (!IPC_IsBackend() || !has_current_input)
		return;

	// Apply view angles from IPC frontend
	VectorCopy(current_input.view_angles, cl.viewangles);

	// Apply button states
	// Bit 0: Attack
	if (current_input.buttons & 1)
		in_attack.state |= 1 + 2;  // down + impulse down
	else
		in_attack.state &= ~1;  // clear down state

	// Bit 1: Jump
	if (current_input.buttons & 2)
		in_jump.state |= 1 + 2;  // down + impulse down
	else
		in_jump.state &= ~1;  // clear down state

	// Bit 2: Use
	if (current_input.buttons & 4)
		in_use.state |= 1 + 2;  // down + impulse down
	else
		in_use.state &= ~1;  // clear down state

	// Apply impulse command (weapon selection, etc)
	if (current_input.impulse > 0)
		in_impulse = current_input.impulse;

	// Clear the input after processing
	has_current_input = false;
}

/*
==================
IPC_GetStats

Get performance statistics
==================
*/
void IPC_GetStats(ipc_stats_t *stats)
{
	if (stats)
		memcpy(stats, &perf_stats, sizeof(ipc_stats_t));
}

/*
==================
IPC_ResetStats

Reset performance statistics
==================
*/
void IPC_ResetStats(void)
{
	memset(&perf_stats, 0, sizeof(ipc_stats_t));
}

/*
==================
Console Commands
==================
*/

static void IPC_Mode_f(void)
{
	if (Cmd_Argc() < 2)
	{
		const char *mode_str = "disabled";
		switch (ipc_mode)
		{
			case IPC_MODE_DISABLED:  mode_str = "disabled"; break;
			case IPC_MODE_BACKEND:   mode_str = "backend"; break;
			case IPC_MODE_FRONTEND:  mode_str = "frontend"; break;
			case IPC_MODE_BOTH:      mode_str = "both (backend)"; break;
		}
		Con_Printf("Usage: pluq_mode <disabled|backend|frontend|both>\n");
		Con_Printf("\n");
		Con_Printf("Modes:\n");
		Con_Printf("  disabled - PluQ IPC is off\n");
		Con_Printf("  backend  - Run simulation + broadcast state + receive IPC input (additive)\n");
		Con_Printf("  frontend - Receive state from backend, send input (no local simulation)\n");
		Con_Printf("  both     - Same as backend (legacy alias)\n");
		Con_Printf("\n");
		Con_Printf("Current mode: %s\n", mode_str);
		Con_Printf("Headless: %s\n", pluq_headless.value ? "yes" : "no");
		return;
	}

	const char *mode_arg = Cmd_Argv(1);
	ipc_mode_t new_mode = IPC_MODE_DISABLED;

	if (!Q_strcasecmp(mode_arg, "disabled") || !Q_strcasecmp(mode_arg, "0"))
		new_mode = IPC_MODE_DISABLED;
	else if (!Q_strcasecmp(mode_arg, "backend") || !Q_strcasecmp(mode_arg, "1"))
		new_mode = IPC_MODE_BACKEND;
	else if (!Q_strcasecmp(mode_arg, "frontend") || !Q_strcasecmp(mode_arg, "2"))
		new_mode = IPC_MODE_FRONTEND;
	else if (!Q_strcasecmp(mode_arg, "both") || !Q_strcasecmp(mode_arg, "3"))
		new_mode = IPC_MODE_BOTH;
	else
	{
		Con_Printf("Invalid mode. Use: disabled, backend, frontend, or both\n");
		return;
	}

	IPC_SetMode(new_mode);
}

static void IPC_Stats_f(void)
{
	if (!IPC_IsEnabled())
	{
		Con_Printf("IPC is not enabled\n");
		return;
	}

	ipc_stats_t stats;
	IPC_GetStats(&stats);

	if (stats.frames_sent == 0)
	{
		Con_Printf("No IPC performance data available\n");
		return;
	}

	double avg_time = stats.total_time / stats.frames_sent;
	double avg_entities = (double)stats.total_entities / stats.frames_sent;
	double frame_budget = 1.0 / 60.0; // 60 FPS = 16.67ms per frame

	Con_Printf("\nIPC Performance Statistics:\n");
	Con_Printf("==========================\n");
	Con_Printf("Frames sent: %llu\n", (unsigned long long)stats.frames_sent);
	Con_Printf("Average time: %.3fms (%.1f%% of frame budget)\n",
	           avg_time * 1000.0, (avg_time / frame_budget) * 100.0);
	Con_Printf("Min/Max time: %.3fms / %.3fms\n",
	           stats.min_frame_time * 1000.0,
	           stats.max_frame_time * 1000.0);
	Con_Printf("Average entities: %.1f\n", avg_entities);
	Con_Printf("Bandwidth: %.1f entities/sec\n",
	           stats.total_entities / stats.total_time);
	Con_Printf("Shared memory size: %zu bytes\n", sizeof(ipc_shared_memory_t));
	Con_Printf("\n");
}

static void IPC_ResetStats_f(void)
{
	IPC_ResetStats();
	Con_Printf("IPC statistics reset\n");
}

/*
==================
IPC_Init

Initialize IPC console commands
Called from Host_Init
==================
*/
void IPC_Init(void)
{
	// Register cvar
	Cvar_RegisterVariable(&pluq_headless);

	// Register commands
	Cmd_AddCommand("pluq_mode", IPC_Mode_f);
	Cmd_AddCommand("ipc_stats", IPC_Stats_f);
	Cmd_AddCommand("ipc_reset_stats", IPC_ResetStats_f);

	Con_Printf("PluQ IPC subsystem ready\n");
	Con_Printf("Use 'pluq_mode backend' to enable backend mode\n");
	Con_Printf("Use 'pluq_mode frontend' to enable frontend mode\n");
	Con_Printf("Use 'pluq_headless 1' for headless backend (no rendering/input)\n");
}
