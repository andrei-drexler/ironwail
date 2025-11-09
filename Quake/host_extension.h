/*
Copyright (C) 2024 Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// host_extension.h -- Generic extension/plugin API for Ironwail

#ifndef HOST_EXTENSION_H
#define HOST_EXTENSION_H

#include "quakedef.h"

// Extension operation hooks
// All callbacks are optional (NULL = not implemented)
typedef struct host_extension_ops_s
{
	// Extension metadata
	const char *name;              // Extension name (e.g., "PluQ IPC")
	const char *version;           // Version string (e.g., "1.0")

	// Lifecycle hooks
	void (*init)(void);            // Called during Host_Init()
	void (*shutdown)(void);        // Called during Host_Shutdown()

	// Frame lifecycle hooks
	void (*frame_begin)(void);     // Called at start of _Host_Frame()
	void (*frame_end)(void);       // Called at end of _Host_Frame()

	// Input hooks
	qboolean (*should_skip_local_input)(void);  // Return true to skip keyboard/mouse
	void (*process_input)(void);                 // Process extension-specific input

	// Server hooks
	qboolean (*should_skip_server)(void);   // Return true to skip server frame
	void (*pre_server_frame)(void);          // Called before Host_ServerFrame()
	void (*post_server_frame)(void);         // Called after Host_ServerFrame()

	// World state hooks
	void (*broadcast_world_state)(void);    // Called after server frame (backend)
	qboolean (*receive_world_state)(void);  // Called before client frame (frontend)
	void (*apply_received_state)(void);     // Called after receive returns true

	// Rendering hooks
	qboolean (*should_skip_rendering)(void);  // Return true to skip video/audio
	void (*pre_render)(void);                  // Called before SCR_UpdateScreen()
	void (*post_render)(void);                 // Called after audio/particles

	// Mode queries (for backward compatibility)
	qboolean (*is_active)(void);            // Return true if extension is enabled

} host_extension_ops_t;

// Extension registration
void Host_RegisterExtension(host_extension_ops_t *ops);
void Host_UnregisterExtension(host_extension_ops_t *ops);

// Extension hooks (called by host code)
void Host_Extension_Init(void);
void Host_Extension_Shutdown(void);
void Host_Extension_FrameBegin(void);
void Host_Extension_FrameEnd(void);
qboolean Host_Extension_ShouldSkipLocalInput(void);
void Host_Extension_ProcessInput(void);
qboolean Host_Extension_ShouldSkipServer(void);
void Host_Extension_PreServerFrame(void);
void Host_Extension_PostServerFrame(void);
void Host_Extension_BroadcastWorldState(void);
qboolean Host_Extension_ReceiveWorldState(void);
void Host_Extension_ApplyReceivedState(void);
qboolean Host_Extension_ShouldSkipRendering(void);
void Host_Extension_PreRender(void);
void Host_Extension_PostRender(void);

// Utility functions
int Host_Extension_Count(void);
const char* Host_Extension_GetName(int index);

#endif // HOST_EXTENSION_H
