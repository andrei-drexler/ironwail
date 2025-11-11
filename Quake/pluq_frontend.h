/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef _PLUQ_FRONTEND_H_
#define _PLUQ_FRONTEND_H_

// pluq_frontend.h -- PluQ Frontend IPC Code
// Frontend binary only - receives world state and sends input

#include "quakedef.h"
#include "pluq.h"

// ============================================================================
// FRONTEND INITIALIZATION / SHUTDOWN
// ============================================================================

// Initialize PluQ frontend sockets (REQ, SUB, PUSH)
qboolean PluQ_Frontend_Init(void);

// Shutdown frontend sockets
void PluQ_Frontend_Shutdown(void);

// ============================================================================
// FRONTEND TRANSPORT LAYER
// ============================================================================

// Resources channel (REQ/REP)
qboolean PluQ_Frontend_RequestResource(uint32_t resource_id);
qboolean PluQ_Frontend_ReceiveResource(void **flatbuf_out, size_t *size_out);

// Gameplay channel (PUB/SUB)
qboolean PluQ_Frontend_ReceiveFrame(void **flatbuf_out, size_t *size_out);

// Input channel (PUSH/PULL)
qboolean PluQ_Frontend_SendInput(const void *flatbuf, size_t size);

// ============================================================================
// FRONTEND HIGH-LEVEL API
// ============================================================================

// Send console command string to backend
void PluQ_Frontend_SendCommand(const char *cmd_text);

// Receive and parse world state from backend
qboolean PluQ_Frontend_ReceiveWorldState(void);

// Apply received state to local game
void PluQ_Frontend_ApplyReceivedState(void);

// Send input command to backend
void PluQ_Frontend_SendInputCommand(usercmd_t *cmd);

// Apply view angles from received state
void PluQ_Frontend_ApplyViewAngles(void);

// Process move command (frontend-specific)
void PluQ_Frontend_Move(usercmd_t *cmd);

#endif // _PLUQ_FRONTEND_H_
