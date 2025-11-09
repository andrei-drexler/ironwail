/*
Copyright (C) 2024 Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// host_extension.c -- Generic extension/plugin system implementation

#include "quakedef.h"
#include "host_extension.h"

#define MAX_EXTENSIONS 8

static host_extension_ops_t *extensions[MAX_EXTENSIONS];
static int num_extensions = 0;

// ============================================================================
// REGISTRATION
// ============================================================================

void Host_RegisterExtension(host_extension_ops_t *ops)
{
	if (!ops)
	{
		Con_Warning("Host_RegisterExtension: NULL ops\n");
		return;
	}

	if (num_extensions >= MAX_EXTENSIONS)
	{
		Con_Warning("Host_RegisterExtension: Too many extensions (max %d)\n", MAX_EXTENSIONS);
		return;
	}

	extensions[num_extensions++] = ops;
	Con_Printf("Registered extension: %s %s\n",
		ops->name ? ops->name : "Unknown",
		ops->version ? ops->version : "");
}

void Host_UnregisterExtension(host_extension_ops_t *ops)
{
	for (int i = 0; i < num_extensions; i++)
	{
		if (extensions[i] == ops)
		{
			// Shift remaining extensions down
			for (int j = i; j < num_extensions - 1; j++)
				extensions[j] = extensions[j + 1];
			num_extensions--;
			return;
		}
	}
}

// ============================================================================
// LIFECYCLE HOOKS
// ============================================================================

void Host_Extension_Init(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->init)
			extensions[i]->init();
}

void Host_Extension_Shutdown(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->shutdown)
			extensions[i]->shutdown();
}

void Host_Extension_FrameBegin(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->frame_begin)
			extensions[i]->frame_begin();
}

void Host_Extension_FrameEnd(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->frame_end)
			extensions[i]->frame_end();
}

// ============================================================================
// INPUT HOOKS
// ============================================================================

qboolean Host_Extension_ShouldSkipLocalInput(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->should_skip_local_input)
			if (extensions[i]->should_skip_local_input())
				return true;
	return false;
}

void Host_Extension_ProcessInput(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->process_input)
			extensions[i]->process_input();
}

// ============================================================================
// SERVER HOOKS
// ============================================================================

qboolean Host_Extension_ShouldSkipServer(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->should_skip_server)
			if (extensions[i]->should_skip_server())
				return true;
	return false;
}

void Host_Extension_PreServerFrame(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->pre_server_frame)
			extensions[i]->pre_server_frame();
}

void Host_Extension_PostServerFrame(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->post_server_frame)
			extensions[i]->post_server_frame();
}

// ============================================================================
// WORLD STATE HOOKS
// ============================================================================

void Host_Extension_BroadcastWorldState(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->broadcast_world_state)
			extensions[i]->broadcast_world_state();
}

qboolean Host_Extension_ReceiveWorldState(void)
{
	qboolean received = false;
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->receive_world_state)
			if (extensions[i]->receive_world_state())
				received = true;
	return received;
}

void Host_Extension_ApplyReceivedState(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->apply_received_state)
			extensions[i]->apply_received_state();
}

// ============================================================================
// RENDERING HOOKS
// ============================================================================

qboolean Host_Extension_ShouldSkipRendering(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->should_skip_rendering)
			if (extensions[i]->should_skip_rendering())
				return true;
	return false;
}

void Host_Extension_PreRender(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->pre_render)
			extensions[i]->pre_render();
}

void Host_Extension_PostRender(void)
{
	for (int i = 0; i < num_extensions; i++)
		if (extensions[i]->post_render)
			extensions[i]->post_render();
}

// ============================================================================
// UTILITY
// ============================================================================

int Host_Extension_Count(void)
{
	return num_extensions;
}

const char* Host_Extension_GetName(int index)
{
	if (index < 0 || index >= num_extensions)
		return NULL;
	return extensions[index]->name;
}
