/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq.c -- PluQ IPC via nng + FlatBuffers

#include "pluq.h"
#include <string.h>

// Console variables
static cvar_t pluq_headless = {"pluq_headless", "0", CVAR_NONE};

// Global state
static qboolean pluq_initialized = false;
static pluq_mode_t pluq_mode = PLUQ_MODE_DISABLED;
static pluq_context_t pluq_ctx;
static pluq_input_cmd_t current_input = {0};
static qboolean has_current_input = false;
static uint32_t last_received_frame = 0;
static pluq_stats_t perf_stats = {0};

// ============================================================================
// INITIALIZATION / SHUTDOWN
// ============================================================================

void PluQ_Init(void)
{
	Cvar_RegisterVariable(&pluq_headless);
	Con_Printf("PluQ IPC system ready (nng + FlatBuffers)\n");
}

qboolean PluQ_Initialize(pluq_mode_t mode)
{
	int rv;

	if (pluq_initialized && pluq_mode != PLUQ_MODE_DISABLED)
	{
		Con_Printf("PluQ already initialized in mode %d\n", pluq_mode);
		return true;
	}

	if (mode == PLUQ_MODE_DISABLED)
	{
		Con_Printf("Cannot initialize PluQ in disabled mode\n");
		return false;
	}

	Con_Printf("Initializing PluQ (nng+FlatBuffers) in mode: ");
	switch (mode)
	{
		case PLUQ_MODE_BACKEND:   Con_Printf("BACKEND\n"); break;
		case PLUQ_MODE_FRONTEND:  Con_Printf("FRONTEND\n"); break;
		case PLUQ_MODE_BOTH:      Con_Printf("BOTH (same as BACKEND)\n"); break;
		default:                  Con_Printf("UNKNOWN\n"); break;
	}

	memset(&pluq_ctx, 0, sizeof(pluq_ctx));
	pluq_ctx.is_backend = (mode == PLUQ_MODE_BACKEND || mode == PLUQ_MODE_BOTH);
	pluq_ctx.is_frontend = (mode == PLUQ_MODE_FRONTEND);

	if (pluq_ctx.is_backend)
	{
		if ((rv = nng_rep0_open(&pluq_ctx.resources_rep)) != 0)
		{
			Con_Printf("PluQ: Failed to create resources REP socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listen(pluq_ctx.resources_rep, PLUQ_URL_RESOURCES, NULL, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to listen on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
			goto error;
		}

		if ((rv = nng_pub0_open(&pluq_ctx.gameplay_pub)) != 0)
		{
			Con_Printf("PluQ: Failed to create gameplay PUB socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listen(pluq_ctx.gameplay_pub, PLUQ_URL_GAMEPLAY, NULL, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to listen on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
			goto error;
		}

		if ((rv = nng_pull0_open(&pluq_ctx.input_pull)) != 0)
		{
			Con_Printf("PluQ: Failed to create input PULL socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listen(pluq_ctx.input_pull, PLUQ_URL_INPUT, NULL, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to listen on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
			goto error;
		}

		Con_Printf("PluQ: Backend initialized successfully\n");
	}
	else
	{
		if ((rv = nng_req0_open(&pluq_ctx.resources_req)) != 0)
		{
			Con_Printf("PluQ: Failed to create resources REQ socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_dial(pluq_ctx.resources_req, PLUQ_URL_RESOURCES, NULL, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to dial %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
			goto error;
		}

		if ((rv = nng_sub0_open(&pluq_ctx.gameplay_sub)) != 0)
		{
			Con_Printf("PluQ: Failed to create gameplay SUB socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_sub0_socket_subscribe(pluq_ctx.gameplay_sub, "", 0)) != 0)
		{
			Con_Printf("PluQ: Failed to subscribe to gameplay events: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_dial(pluq_ctx.gameplay_sub, PLUQ_URL_GAMEPLAY, NULL, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to dial %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
			goto error;
		}

		if ((rv = nng_push0_open(&pluq_ctx.input_push)) != 0)
		{
			Con_Printf("PluQ: Failed to create input PUSH socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_dial(pluq_ctx.input_push, PLUQ_URL_INPUT, NULL, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to dial %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
			goto error;
		}

		Con_Printf("PluQ: Frontend initialized successfully\n");
	}

	pluq_ctx.initialized = true;
	pluq_initialized = true;
	pluq_mode = mode;
	return true;

error:
	PluQ_Shutdown();
	return false;
}

void PluQ_Shutdown(void)
{
	if (!pluq_initialized)
		return;

	Con_Printf("PluQ: Shutting down\n");

	if (pluq_ctx.is_backend)
	{
		nng_socket_close(pluq_ctx.resources_rep);
		nng_socket_close(pluq_ctx.gameplay_pub);
		nng_socket_close(pluq_ctx.input_pull);
	}
	else
	{
		nng_socket_close(pluq_ctx.resources_req);
		nng_socket_close(pluq_ctx.gameplay_sub);
		nng_socket_close(pluq_ctx.input_push);
	}

	memset(&pluq_ctx, 0, sizeof(pluq_ctx));
	pluq_initialized = false;
	pluq_mode = PLUQ_MODE_DISABLED;
}

// ============================================================================
// MODE MANAGEMENT
// ============================================================================

pluq_mode_t PluQ_GetMode(void) { return pluq_mode; }
qboolean PluQ_IsEnabled(void) { return pluq_initialized && pluq_mode != PLUQ_MODE_DISABLED; }
qboolean PluQ_IsBackend(void) { return pluq_mode == PLUQ_MODE_BACKEND || pluq_mode == PLUQ_MODE_BOTH; }
qboolean PluQ_IsFrontend(void) { return pluq_mode == PLUQ_MODE_FRONTEND; }
qboolean PluQ_IsHeadless(void) { return (COM_CheckParm("-headless") != 0); }

void PluQ_SetMode(pluq_mode_t mode)
{
	if (pluq_initialized && mode != pluq_mode)
	{
		PluQ_Shutdown();
		PluQ_Initialize(mode);
	}
	else if (!pluq_initialized && mode != PLUQ_MODE_DISABLED)
	{
		PluQ_Initialize(mode);
	}
}

// ============================================================================
// TRANSPORT LAYER (nng + FlatBuffers)
// ============================================================================

qboolean PluQ_Backend_SendResource(const void *flatbuf, size_t size)
{
	if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
		return false;

	int rv = nng_send(pluq_ctx.resources_rep, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PluQ: Failed to send resource: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

qboolean PluQ_Frontend_RequestResource(uint32_t resource_id)
{
	// TODO: Build ResourceRequest FlatBuffer and send
	Con_Printf("PluQ: Requesting resource ID %u (not yet implemented)\n", resource_id);
	return false;
}

qboolean PluQ_Frontend_ReceiveResource(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
		return false;

	if ((rv = nng_recvmsg(pluq_ctx.resources_req, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ: Failed to receive resource: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

qboolean PluQ_Backend_PublishFrame(const void *flatbuf, size_t size)
{
	if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
		return false;

	int rv = nng_send(pluq_ctx.gameplay_pub, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PluQ: Failed to publish gameplay frame: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

qboolean PluQ_Frontend_ReceiveFrame(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
		return false;

	if ((rv = nng_recvmsg(pluq_ctx.gameplay_sub, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ: Failed to receive gameplay frame: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

qboolean PluQ_Frontend_SendInput(const void *flatbuf, size_t size)
{
	if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
		return false;

	int rv = nng_send(pluq_ctx.input_push, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PluQ: Failed to send input command: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

qboolean PluQ_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
		return false;

	if ((rv = nng_recvmsg(pluq_ctx.input_pull, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ: Failed to receive input command: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

// ============================================================================
// HIGH-LEVEL API (TODO: Implement using FlatBuffers)
// ============================================================================

void PluQ_BroadcastWorldState(void)
{
	if (!pluq_initialized || !pluq_ctx.is_backend)
		return;

	// Don't broadcast if not in game
	if (!cl.worldmodel || cls.state != ca_connected)
		return;

	static uint32_t frame_counter = 0;
	double start_time = Sys_DoubleTime();

	// Initialize FlatBuffers builder
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Build FrameUpdate
	PluQ_FrameUpdate_start(&builder);

	// Frame info
	PluQ_FrameUpdate_frame_number_add(&builder, frame_counter++);
	PluQ_FrameUpdate_timestamp_add(&builder, cl.time);

	// View state
	PluQ_Vec3_t view_origin = QuakeVec3_To_FB(r_refdef.vieworg);
	PluQ_Vec3_t view_angles = QuakeVec3_To_FB(cl.viewangles);
	PluQ_FrameUpdate_view_origin_add(&builder, &view_origin);
	PluQ_FrameUpdate_view_angles_add(&builder, &view_angles);

	// Player stats
	PluQ_FrameUpdate_health_add(&builder, (int16_t)cl.stats[STAT_HEALTH]);
	PluQ_FrameUpdate_armor_add(&builder, (int16_t)cl.stats[STAT_ARMOR]);
	PluQ_FrameUpdate_weapon_add(&builder, (uint8_t)cl.stats[STAT_WEAPON]);
	PluQ_FrameUpdate_ammo_add(&builder, (uint16_t)cl.stats[STAT_AMMO]);

	// Game state
	PluQ_FrameUpdate_paused_add(&builder, (cl.paused != 0));
	PluQ_FrameUpdate_in_game_add(&builder, true);

	// TODO: Add entities (cl_visedicts)
	// For now, just send player state

	PluQ_FrameUpdate_ref_t frame_ref = PluQ_FrameUpdate_end(&builder);

	// Wrap in GameplayMessage
	PluQ_GameplayEvent_union_ref_t event;
	event.type = PluQ_GameplayEvent_FrameUpdate;
	event.value = frame_ref;

	PluQ_GameplayMessage_create(&builder, event);
	PluQ_GameplayMessage_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (buf)
	{
		// Publish frame
		PluQ_Backend_PublishFrame(buf, size);

		// Update stats
		perf_stats.frames_sent++;
		double frame_time = Sys_DoubleTime() - start_time;
		perf_stats.total_time += frame_time;
		if (frame_time > perf_stats.max_frame_time)
			perf_stats.max_frame_time = frame_time;
		if (perf_stats.min_frame_time == 0.0 || frame_time < perf_stats.min_frame_time)
			perf_stats.min_frame_time = frame_time;

		// Free buffer
		flatcc_builder_aligned_free(buf);
	}

	flatcc_builder_clear(&builder);
}

qboolean PluQ_ReceiveWorldState(void)
{
	void *buf;
	size_t size;

	if (!PluQ_Frontend_ReceiveFrame(&buf, &size))
		return false;

	// Parse GameplayMessage
	PluQ_GameplayMessage_table_t msg = PluQ_GameplayMessage_as_root(buf);
	if (!msg)
	{
		nng_msg_free((nng_msg *)buf);  // Free the nng_msg wrapper
		return false;
	}

	// Get event type and value
	PluQ_GameplayEvent_union_type_t event_type = PluQ_GameplayMessage_event_type(msg);
	flatbuffers_generic_t event_value = PluQ_GameplayMessage_event(msg);

	if (event_type == PluQ_GameplayEvent_FrameUpdate)
	{
		PluQ_FrameUpdate_table_t frame = (PluQ_FrameUpdate_table_t)event_value;

		// Update last received frame
		last_received_frame = PluQ_FrameUpdate_frame_number(frame);

		// TODO: Store frame data for PluQ_ApplyReceivedState()
		// For now, just log
		Con_DPrintf("PluQ: Received frame %u\n", last_received_frame);
	}
	else if (event_type == PluQ_GameplayEvent_MapChanged)
	{
		PluQ_MapChanged_table_t mapchange = (PluQ_MapChanged_table_t)event_value;
		const char *mapname = PluQ_MapChanged_mapname(mapchange);
		Con_Printf("PluQ: Map changed to %s\n", mapname);
	}
	else if (event_type == PluQ_GameplayEvent_Disconnected)
	{
		PluQ_Disconnected_table_t disc = (PluQ_Disconnected_table_t)event_value;
		const char *reason = PluQ_Disconnected_reason(disc);
		Con_Printf("PluQ: Disconnected: %s\n", reason);
	}

	nng_msg_free((nng_msg *)buf);
	return true;
}

void PluQ_ApplyReceivedState(void)
{
	// TODO: Apply received state to local game
}

qboolean PluQ_HasPendingInput(void)
{
	if (!PluQ_IsBackend() || !pluq_initialized)
		return false;

	nng_msg *msg;
	int rv = nng_recvmsg(pluq_ctx.input_pull, &msg, NNG_FLAG_NONBLOCK);
	if (rv == 0)
	{
		// TODO: Parse InputCommand FlatBuffer
		nng_msg_free(msg);
	}
	return false;
}

void PluQ_ProcessInputCommands(void) { /* TODO */ }
void PluQ_SendInput(usercmd_t *cmd) { /* TODO */ }
void PluQ_Move(usercmd_t *cmd) { /* TODO */ }
void PluQ_ApplyViewAngles(void) { /* TODO */ }

void PluQ_GetStats(pluq_stats_t *stats)
{
	if (stats) *stats = perf_stats;
}

void PluQ_ResetStats(void)
{
	memset(&perf_stats, 0, sizeof(perf_stats));
}
