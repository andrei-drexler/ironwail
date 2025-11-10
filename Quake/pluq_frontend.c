/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq_frontend.c -- PluQ Frontend IPC Implementation
// Frontend binary only - connects to backend, receives world state, sends input

#include "pluq_frontend.h"
#include <string.h>

// ============================================================================
// FRONTEND CONTEXT
// ============================================================================

static pluq_context_t frontend_ctx;
static qboolean frontend_initialized = false;
static uint32_t last_received_frame = 0;

// ============================================================================
// FRONTEND INITIALIZATION / SHUTDOWN
// ============================================================================

qboolean PluQ_Frontend_Init(void)
{
	int rv;

	if (frontend_initialized)
	{
		Con_Printf("PluQ Frontend already initialized\n");
		return true;
	}

	Con_Printf("Initializing PluQ Frontend IPC sockets (nng+FlatBuffers)...\n");

	// Note: nng library already initialized by PluQ_Init()

	memset(&frontend_ctx, 0, sizeof(frontend_ctx));
	frontend_ctx.is_backend = false;
	frontend_ctx.is_frontend = true;

	// Initialize frontend sockets (REQ, SUB, PUSH)

	// Resources channel (REQ/REP) - Frontend connects with REQ
	if ((rv = nng_req0_open(&frontend_ctx.resources_req)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create resources REQ socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.resources_dialer, frontend_ctx.resources_req, PLUQ_URL_RESOURCES)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.resources_dialer, 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}

	// Gameplay channel (PUB/SUB) - Frontend connects with SUB
	if ((rv = nng_sub0_open(&frontend_ctx.gameplay_sub)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create gameplay SUB socket: %s\n", nng_strerror(rv));
		goto error;
	}
	// Subscribe to all topics (empty string = all)
	// nng 2.0 API: use nng_sub0_socket_subscribe() instead of nng_socket_set_string()
	if ((rv = nng_sub0_socket_subscribe(frontend_ctx.gameplay_sub, "", 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to subscribe to gameplay channel: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.gameplay_dialer, frontend_ctx.gameplay_sub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.gameplay_dialer, 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}

	// Input channel (PUSH/PULL) - Frontend connects with PUSH
	if ((rv = nng_push0_open(&frontend_ctx.input_push)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create input PUSH socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.input_dialer, frontend_ctx.input_push, PLUQ_URL_INPUT)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.input_dialer, 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}

	Con_Printf("PluQ Frontend: IPC sockets initialized successfully\n");
	Con_Printf("PluQ Frontend: Connected to backend on ports 9001-9003\n");

	frontend_ctx.initialized = true;
	frontend_initialized = true;
	return true;

error:
	PluQ_Frontend_Shutdown();
	return false;
}

void PluQ_Frontend_Shutdown(void)
{
	if (!frontend_initialized)
		return;

	Con_Printf("PluQ Frontend: Shutting down\n");

	// Close frontend sockets
	nng_socket_close(frontend_ctx.resources_req);
	nng_socket_close(frontend_ctx.gameplay_sub);
	nng_socket_close(frontend_ctx.input_push);

	memset(&frontend_ctx, 0, sizeof(frontend_ctx));
	frontend_initialized = false;
}

// ============================================================================
// FRONTEND TRANSPORT LAYER
// ============================================================================

qboolean PluQ_Frontend_RequestResource(uint32_t resource_id)
{
	// TODO: Build ResourceRequest FlatBuffer and send
	Con_Printf("PluQ Frontend: Requesting resource ID %u (not yet implemented)\n", resource_id);
	return false;
}

qboolean PluQ_Frontend_ReceiveResource(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	if ((rv = nng_recvmsg(frontend_ctx.resources_req, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ Frontend: Failed to receive resource: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

qboolean PluQ_Frontend_ReceiveFrame(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	if ((rv = nng_recvmsg(frontend_ctx.gameplay_sub, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ Frontend: Failed to receive gameplay frame: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

qboolean PluQ_Frontend_SendInput(const void *flatbuf, size_t size)
{
	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	int rv = nng_send(frontend_ctx.input_push, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PluQ Frontend: Failed to send input command: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

// ============================================================================
// FRONTEND HIGH-LEVEL API
// ============================================================================

void PluQ_Frontend_SendCommand(const char *cmd_text)
{
	if (!frontend_initialized || !cmd_text || !cmd_text[0])
		return;

	// Build InputCommand FlatBuffer
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Create InputCommand with just the command text
	flatbuffers_string_ref_t cmd_str = flatbuffers_string_create_str(&builder, cmd_text);

	PluQ_InputCommand_start(&builder);
	PluQ_InputCommand_cmd_text_add(&builder, cmd_str);
	PluQ_InputCommand_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (buf)
	{
		// Send to backend
		PluQ_Frontend_SendInput(buf, size);
		flatcc_builder_aligned_free(buf);
	}

	flatcc_builder_clear(&builder);
}

qboolean PluQ_Frontend_ReceiveWorldState(void)
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

		// TODO: Store frame data for PluQ_Frontend_ApplyReceivedState()
		// For now, just log
		Con_DPrintf("PluQ Frontend: Received frame %u\n", last_received_frame);
	}
	else if (event_type == PluQ_GameplayEvent_MapChanged)
	{
		PluQ_MapChanged_table_t mapchange = (PluQ_MapChanged_table_t)event_value;
		const char *mapname = PluQ_MapChanged_mapname(mapchange);
		Con_Printf("PluQ Frontend: Map changed to %s\n", mapname);
	}
	else if (event_type == PluQ_GameplayEvent_Disconnected)
	{
		PluQ_Disconnected_table_t disc = (PluQ_Disconnected_table_t)event_value;
		const char *reason = PluQ_Disconnected_reason(disc);
		Con_Printf("PluQ Frontend: Disconnected: %s\n", reason);
	}

	nng_msg_free((nng_msg *)buf);
	return true;
}

void PluQ_Frontend_ApplyReceivedState(void)
{
	// TODO: Apply received state to local game
	// This will update cl.* state based on received FrameUpdate
}

void PluQ_Frontend_SendInputCommand(usercmd_t *cmd)
{
	// TODO: Build InputCommand FlatBuffer and send
	// For now, stub
}

void PluQ_Frontend_ApplyViewAngles(void)
{
	// TODO: Apply view angles from received state
}

void PluQ_Frontend_Move(usercmd_t *cmd)
{
	// TODO: Process move command for frontend
}
