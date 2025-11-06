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

#ifndef _IPC_H_
#define _IPC_H_

// ipc.h -- Inter-Process Communication for QuakeSpasm/Ironwail
// Implements shared memory-based IPC for broadcasting game state
// and receiving input commands from external frontends.

#include "quakedef.h"

// Maximum number of entities to support in IPC
#define IPC_MAX_ENTITIES 8192

// PluQ operation modes
typedef enum
{
	IPC_MODE_DISABLED,   // IPC not active
	IPC_MODE_BACKEND,    // Backend: run simulation, broadcast state, receive IPC input (additive to normal operation)
	IPC_MODE_FRONTEND,   // Frontend: receive state from backend, send input to backend (no local simulation)
	IPC_MODE_BOTH        // Same as BACKEND (legacy name for clarity)
} ipc_mode_t;

// Input command structure
typedef struct
{
	uint32_t sequence;
	double timestamp;

	// Movement
	float forward_move;
	float side_move;
	float up_move;

	// View
	vec3_t view_angles;

	// Buttons (bitfield)
	// Bit 0: Attack
	// Bit 1: Jump
	// Bit 2: Use
	uint32_t buttons;

	// Impulse command (weapon selection, etc)
	uint8_t impulse;

	// Console commands (optional)
	char cmd_text[256];
} ipc_input_cmd_t;

// Frame header structure
typedef struct
{
	uint32_t frame_number;
	double timestamp;
	uint16_t num_entities;
	uint16_t max_entities;

	// Player/view state
	vec3_t player_origin;
	vec3_t player_angles;
	float player_health;
	float player_armor;
	int player_weapon;
	int player_ammo;

	// Game state
	qboolean paused;
	qboolean in_game;
	char mapname[64];
} ipc_frame_header_t;

// Shared memory layout
typedef struct
{
	// Synchronization (using atomic operations)
	volatile uint32_t frame_sequence;
	volatile uint32_t write_in_progress;

	// Frame data
	ipc_frame_header_t header;

	// Entity data
	entity_t entities[IPC_MAX_ENTITIES];

	// Dynamic lights
	dlight_t dlights[MAX_DLIGHTS];
	uint16_t num_dlights;

	// Input command (from frontend to backend)
	ipc_input_cmd_t input_cmd;
	volatile uint32_t input_ready;

} ipc_shared_memory_t;

// Performance statistics
typedef struct
{
	uint64_t frames_sent;
	double total_time;
	size_t total_entities;
	double max_frame_time;
	double min_frame_time;
} ipc_stats_t;

// IPC initialization and shutdown
qboolean IPC_Initialize(ipc_mode_t mode);
void IPC_Shutdown(void);

// Mode management
ipc_mode_t IPC_GetMode(void);
void IPC_SetMode(ipc_mode_t mode);
qboolean IPC_IsEnabled(void);
qboolean IPC_IsBackend(void);
qboolean IPC_IsFrontend(void);
qboolean IPC_IsHeadless(void);  // Check if running in headless mode

// Backend functions (state broadcasting)
void IPC_BroadcastWorldState(void);

// Frontend functions (state reception)
qboolean IPC_ReceiveWorldState(void);
void IPC_ApplyReceivedState(void);

// Input handling (backend receives, frontend sends)
qboolean IPC_HasPendingInput(void);
void IPC_ProcessInputCommands(void);
void IPC_SendInput(usercmd_t *cmd);  // Frontend: send input to backend
void IPC_Move(usercmd_t *cmd);  // Backend: add IPC input to movement command
void IPC_ApplyViewAngles(void);  // Backend: apply IPC view angles

// Performance monitoring
void IPC_GetStats(ipc_stats_t *stats);
void IPC_ResetStats(void);

// Console registration
void IPC_Init(void);

#endif // _IPC_H_
