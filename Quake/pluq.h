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

#ifndef _PLUQ_H_
#define _PLUQ_H_

// pluq.h -- PluQ Inter-Process Communication for QuakeSpasm/Ironwail
// Implements shared memory-based IPC for broadcasting game state
// and receiving input commands from external frontends.

#include "quakedef.h"

// Maximum number of entities to support in PluQ
#define PLUQ_MAX_ENTITIES 8192

// PluQ operation modes
typedef enum
{
	PLUQ_MODE_DISABLED,   // PluQ not active
	PLUQ_MODE_BACKEND,    // Backend: run simulation, broadcast state, receive PluQ input (additive to normal operation)
	PLUQ_MODE_FRONTEND,   // Frontend: receive state from backend, send input to backend (no local simulation)
	PLUQ_MODE_BOTH        // Same as BACKEND (legacy name for clarity)
} pluq_mode_t;

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
} pluq_input_cmd_t;

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
} pluq_frame_header_t;

// Shared memory layout
typedef struct
{
	// Synchronization (using atomic operations)
	volatile uint32_t frame_sequence;
	volatile uint32_t write_in_progress;

	// Frame data
	pluq_frame_header_t header;

	// Entity data
	entity_t entities[PLUQ_MAX_ENTITIES];

	// Dynamic lights
	dlight_t dlights[MAX_DLIGHTS];
	uint16_t num_dlights;

	// Input command (from frontend to backend)
	pluq_input_cmd_t input_cmd;
	volatile uint32_t input_ready;

} pluq_shared_memory_t;

// Performance statistics
typedef struct
{
	uint64_t frames_sent;
	double total_time;
	size_t total_entities;
	double max_frame_time;
	double min_frame_time;
} pluq_stats_t;

// PluQ initialization and shutdown
qboolean PluQ_Initialize(pluq_mode_t mode);
void PluQ_Shutdown(void);

// Mode management
pluq_mode_t PluQ_GetMode(void);
void PluQ_SetMode(pluq_mode_t mode);
qboolean PluQ_IsEnabled(void);
qboolean PluQ_IsBackend(void);
qboolean PluQ_IsFrontend(void);
qboolean PluQ_IsHeadless(void);  // Check if running in headless mode

// Backend functions (state broadcasting)
void PluQ_BroadcastWorldState(void);

// Frontend functions (state reception)
qboolean PluQ_ReceiveWorldState(void);
void PluQ_ApplyReceivedState(void);

// Input handling (backend receives, frontend sends)
qboolean PluQ_HasPendingInput(void);
void PluQ_ProcessInputCommands(void);
void PluQ_SendInput(usercmd_t *cmd);  // Frontend: send input to backend
void PluQ_Move(usercmd_t *cmd);  // Backend: add PluQ input to movement command
void PluQ_ApplyViewAngles(void);  // Backend: apply PluQ view angles

// Performance monitoring
void PluQ_GetStats(pluq_stats_t *stats);
void PluQ_ResetStats(void);

// Console registration
void PluQ_Init(void);

#endif // _PLUQ_H_
