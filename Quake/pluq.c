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

#ifdef USE_PLUQ
// Full implementation follows below

// Console variables
static cvar_t pluq_headless = {"pluq_headless", "0", CVAR_NONE};

// Global state
static qboolean pluq_initialized = false;
static qboolean pluq_enabled = false;
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
	int rv;

	Cvar_RegisterVariable(&pluq_headless);

	// Initialize nng library (required for nng 2.0 API)
	if ((rv = nng_init(NULL)) != 0)
	{
		Sys_Error("PluQ: Failed to initialize nng library: %s", nng_strerror(rv));
	}

	Con_Printf("PluQ IPC system ready (nng 2.0 + FlatBuffers)\n");

	// Auto-enable backend mode when using -pluq
	// Note: -pluq requires -headless to be used together
	if (COM_CheckParm("-pluq"))
	{
		if (!COM_CheckParm("-headless"))
			Sys_Error("PluQ backend mode requires -headless flag");

		Con_Printf("PluQ backend mode enabled\n");
		Cvar_Set("pluq_headless", "1");
		PluQ_Enable();
	}
}

static qboolean PluQ_InitializeSockets(void)
{
	int rv;

	if (pluq_initialized)
	{
		Con_Printf("PluQ already initialized\n");
		return true;
	}

	Con_Printf("Initializing PluQ IPC sockets (nng+FlatBuffers)...\n");

	memset(&pluq_ctx, 0, sizeof(pluq_ctx));
	pluq_ctx.is_backend = true;   // Main binary is always backend
	pluq_ctx.is_frontend = false; // Frontend is separate binary

	// Initialize backend sockets (REP, PUB, PULL)
	if (pluq_ctx.is_backend)
	{
		// Resources channel (REQ/REP)
		if ((rv = nng_rep0_open(&pluq_ctx.resources_rep)) != 0)
		{
			Con_Printf("PluQ: Failed to create resources REP socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listener_create(&pluq_ctx.resources_listener, pluq_ctx.resources_rep, PLUQ_URL_RESOURCES)) != 0)
		{
			Con_Printf("PluQ: Failed to create listener for %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listener_start(pluq_ctx.resources_listener, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to start listener on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
			goto error;
		}

		// Gameplay channel (PUB/SUB)
		if ((rv = nng_pub0_open(&pluq_ctx.gameplay_pub)) != 0)
		{
			Con_Printf("PluQ: Failed to create gameplay PUB socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listener_create(&pluq_ctx.gameplay_listener, pluq_ctx.gameplay_pub, PLUQ_URL_GAMEPLAY)) != 0)
		{
			Con_Printf("PluQ: Failed to create listener for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listener_start(pluq_ctx.gameplay_listener, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to start listener on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
			goto error;
		}

		// Input channel (PUSH/PULL)
		if ((rv = nng_pull0_open(&pluq_ctx.input_pull)) != 0)
		{
			Con_Printf("PluQ: Failed to create input PULL socket: %s\n", nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listener_create(&pluq_ctx.input_listener, pluq_ctx.input_pull, PLUQ_URL_INPUT)) != 0)
		{
			Con_Printf("PluQ: Failed to create listener for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
			goto error;
		}
		if ((rv = nng_listener_start(pluq_ctx.input_listener, 0)) != 0)
		{
			Con_Printf("PluQ: Failed to start listener on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
			goto error;
		}

		Con_Printf("PluQ: IPC sockets initialized successfully\n");
	}

	pluq_ctx.initialized = true;
	pluq_initialized = true;
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

	// Main binary only has backend sockets
	nng_socket_close(pluq_ctx.resources_rep);
	nng_socket_close(pluq_ctx.gameplay_pub);
	nng_socket_close(pluq_ctx.input_pull);

	memset(&pluq_ctx, 0, sizeof(pluq_ctx));
	pluq_initialized = false;
	pluq_enabled = false;
}

// ============================================================================
// MODE MANAGEMENT
// ============================================================================

qboolean PluQ_IsEnabled(void)
{
	return pluq_enabled && pluq_initialized;
}

void PluQ_Enable(void)
{
	if (!pluq_initialized)
		PluQ_InitializeSockets();

	pluq_enabled = true;
	Con_Printf("PluQ IPC enabled\n");
}

void PluQ_Disable(void)
{
	pluq_enabled = false;
	Con_Printf("PluQ IPC disabled\n");
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

// Frontend resource functions moved to pluq_frontend.c

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

// Frontend gameplay and input functions moved to pluq_frontend.c

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
	static int debug_count = 0;

	if (!PluQ_IsEnabled())
		return;

	// Don't broadcast if not in game
	if (!cl.worldmodel || cls.state != ca_connected)
	{
		if (debug_count++ < 5)
			Con_DPrintf("PluQ_BroadcastWorldState: no worldmodel (%p) or not connected (state=%d)\n",
				cl.worldmodel, cls.state);
		return;
	}

	static uint32_t frame_counter = 0;
	double start_time = Sys_DoubleTime();

	// Debug: Log first few broadcasts
	if (frame_counter < 5)
		Con_Printf("PluQ: Broadcasting frame %u\n", frame_counter);

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

// Frontend world state functions moved to pluq_frontend.c
// (PluQ_Frontend_ReceiveWorldState and PluQ_Frontend_ApplyReceivedState)

qboolean PluQ_HasPendingInput(void)
{
	if (!PluQ_IsEnabled() || !pluq_initialized)
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

void PluQ_ProcessInputCommands(void)
{
	void *buf;
	size_t size;

	if (!PluQ_IsEnabled())
		return;

	// Process all pending input commands
	while (PluQ_Backend_ReceiveInput(&buf, &size))
	{
		// Parse FlatBuffer
		PluQ_InputCommand_table_t cmd = PluQ_InputCommand_as_root(buf);
		if (!cmd)
		{
			Con_Printf("PluQ: Failed to parse InputCommand\n");
			nng_msg_free((nng_msg *)buf);
			continue;
		}

		// Get command text
		const char *cmd_text = PluQ_InputCommand_cmd_text(cmd);
		if (cmd_text && cmd_text[0])
		{
			Con_Printf("PluQ: Received command: \"%s\"\n", cmd_text);
			Cbuf_AddText(cmd_text);
			Cbuf_AddText("\n");
		}

		nng_msg_free((nng_msg *)buf);
	}
}

void PluQ_Move(usercmd_t *cmd)
{
	if (!PluQ_IsEnabled())
		return;
	// TODO: Apply IPC-received movement to command
}

void PluQ_ApplyViewAngles(void)
{
	if (!PluQ_IsEnabled())
		return;
	// TODO: Apply IPC-received view angles to cl.viewangles
}

// Frontend input functions moved to pluq_frontend.c
// (PluQ_Frontend_SendInputCommand, PluQ_Frontend_Move, PluQ_Frontend_ApplyViewAngles)

void PluQ_GetStats(pluq_stats_t *stats)
{
	if (stats) *stats = perf_stats;
}

void PluQ_ResetStats(void)
{
	memset(&perf_stats, 0, sizeof(perf_stats));
}

#else // !USE_PLUQ

// Stub implementations when PluQ is disabled
void PluQ_Init(void) {}
void PluQ_Shutdown(void) {}
qboolean PluQ_IsEnabled(void) { return false; }
void PluQ_Enable(void) {}
void PluQ_Disable(void) {}
void PluQ_BroadcastWorldState(void) {}
qboolean PluQ_HasPendingInput(void) { return false; }
void PluQ_ProcessInputCommands(void) {}
void PluQ_Move(usercmd_t *cmd) { (void)cmd; }
void PluQ_ApplyViewAngles(void) {}
void PluQ_GetStats(pluq_stats_t *stats) { if (stats) memset(stats, 0, sizeof(*stats)); }
void PluQ_ResetStats(void) {}
qboolean PluQ_Backend_SendResource(const void *flatbuf, size_t size) { (void)flatbuf; (void)size; return false; }
qboolean PluQ_Backend_PublishFrame(const void *flatbuf, size_t size) { (void)flatbuf; (void)size; return false; }
qboolean PluQ_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out) { (void)flatbuf_out; (void)size_out; return false; }

#endif // USE_PLUQ
