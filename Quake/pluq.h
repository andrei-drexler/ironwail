/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef _PLUQ_H_
#define _PLUQ_H_

// pluq.h -- PluQ Inter-Process Communication via nng + FlatBuffers
// Three-channel architecture: Resources, Gameplay, Input

#include "quakedef.h"
#include <nng/nng.h>

// Include generated FlatBuffers C headers
#include "pluq_reader.h"
#include "pluq_builder.h"

// ============================================================================
// CHANNEL ENDPOINTS
// ============================================================================

#define PLUQ_URL_RESOURCES  "tcp://127.0.0.1:9001"
#define PLUQ_URL_GAMEPLAY   "tcp://127.0.0.1:9002"
#define PLUQ_URL_INPUT      "tcp://127.0.0.1:9003"

// ============================================================================
// PLUQ MODE (Simplified: enabled/disabled)
// Main binary: PluQ IPC enabled or disabled
// Frontend binary: Uses separate entry point (main_pluq_frontend.c)
// ============================================================================

// ============================================================================
// NNG CONTEXT
// ============================================================================

typedef struct {
	nng_socket resources_rep, resources_req;
	nng_socket gameplay_pub, gameplay_sub;
	nng_socket input_pull, input_push;
	nng_listener resources_listener, gameplay_listener, input_listener;
	nng_dialer resources_dialer, gameplay_dialer, input_dialer;
	qboolean is_backend, is_frontend, initialized;
} pluq_context_t;

// Input command structure
typedef struct
{
	uint32_t sequence;
	double timestamp;
	float forward_move, side_move, up_move;
	vec3_t view_angles;
	uint32_t buttons;
	uint8_t impulse;
	char cmd_text[256];
} pluq_input_cmd_t;

// Performance statistics
typedef struct
{
	uint64_t frames_sent;
	double total_time;
	size_t total_entities;
	double max_frame_time, min_frame_time;
} pluq_stats_t;

// ============================================================================
// PUBLIC API
// ============================================================================

// Initialize/Shutdown
void PluQ_Init(void);
void PluQ_Shutdown(void);

// Enable/Disable IPC
qboolean PluQ_IsEnabled(void);
void PluQ_Enable(void);
void PluQ_Disable(void);

// Backend API (main binary)
void PluQ_BroadcastWorldState(void);
qboolean PluQ_HasPendingInput(void);
void PluQ_ProcessInputCommands(void);
void PluQ_Move(usercmd_t *cmd);
void PluQ_ApplyViewAngles(void);

// Frontend API moved to pluq_frontend.h
// (PluQ_Frontend_ReceiveWorldState, PluQ_Frontend_ApplyReceivedState,
//  PluQ_Frontend_SendInputCommand, PluQ_Frontend_Move, PluQ_Frontend_ApplyViewAngles)

void PluQ_GetStats(pluq_stats_t *stats);
void PluQ_ResetStats(void);

// Backend transport layer (main binary)
qboolean PluQ_Backend_SendResource(const void *flatbuf, size_t size);
qboolean PluQ_Backend_PublishFrame(const void *flatbuf, size_t size);
qboolean PluQ_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out);

// Frontend transport layer moved to pluq_frontend.h
// (PluQ_Frontend_RequestResource, PluQ_Frontend_ReceiveResource,
//  PluQ_Frontend_ReceiveFrame, PluQ_Frontend_SendInput)

// vec3_t conversion helpers
static inline PluQ_Vec3_t QuakeVec3_To_FB(const vec3_t v)
{
	PluQ_Vec3_t fb_vec;
	memcpy(&fb_vec, v, sizeof(PluQ_Vec3_t));
	return fb_vec;
}

static inline void FB_Vec3_To_Quake(const PluQ_Vec3_t *fb_vec, vec3_t v)
{
	memcpy(v, fb_vec, sizeof(PluQ_Vec3_t));
}

#endif // _PLUQ_H_
