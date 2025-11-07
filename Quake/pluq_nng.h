/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef _PLUQ_NNG_H_
#define _PLUQ_NNG_H_

// pluq_nng.h -- PluQ nng + FlatBuffers IPC
// Three-channel architecture: Resources, Gameplay, Input

#include "quakedef.h"
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/pipeline0/push.h>
#include <nng/protocol/pipeline0/pull.h>

// Include generated FlatBuffers C headers
// (Generated from pluq.fbs using: flatcc -a pluq.fbs)
#include "pluq_reader.h"
#include "pluq_builder.h"

// ============================================================================
// CHANNEL ENDPOINTS
// ============================================================================

#define PLUQ_URL_RESOURCES  "ipc:///tmp/quake_pluq_resources"
#define PLUQ_URL_GAMEPLAY   "ipc:///tmp/quake_pluq_gameplay"
#define PLUQ_URL_INPUT      "ipc:///tmp/quake_pluq_input"

// ============================================================================
// NNG CONTEXT
// ============================================================================

typedef struct {
    // Resources channel (REQ/REP pattern)
    nng_socket resources_rep;     // Backend: Reply to resource requests
    nng_socket resources_req;     // Frontend: Request resources

    // Gameplay channel (PUB/SUB pattern)
    nng_socket gameplay_pub;      // Backend: Publish game frames
    nng_socket gameplay_sub;      // Frontend: Subscribe to frames

    // Input channel (PUSH/PULL pattern)
    nng_socket input_pull;        // Backend: Pull input commands
    nng_socket input_push;        // Frontend: Push input commands

    qboolean is_backend;
    qboolean is_frontend;
    qboolean initialized;
} pluq_nng_context_t;

// ============================================================================
// API FUNCTIONS
// ============================================================================

// Initialize/Shutdown
qboolean PluQ_NNG_Init(qboolean is_backend);
void PluQ_NNG_Shutdown(void);

// Resources channel (Backend → Frontend, REQ/REP)
// Frontend requests resources, backend replies
qboolean PluQ_NNG_Backend_WaitForResourceRequest(void);
qboolean PluQ_NNG_Backend_SendResource(const void *flatbuf, size_t size);

qboolean PluQ_NNG_Frontend_RequestResource(uint32_t resource_id);
qboolean PluQ_NNG_Frontend_ReceiveResource(void **flatbuf_out, size_t *size_out);

// Gameplay channel (Backend → Frontend, PUB/SUB)
qboolean PluQ_NNG_Backend_PublishFrame(const void *flatbuf, size_t size);
qboolean PluQ_NNG_Frontend_ReceiveFrame(void **flatbuf_out, size_t *size_out);

// Input channel (Frontend → Backend, PUSH/PULL)
qboolean PluQ_NNG_Frontend_SendInput(const void *flatbuf, size_t size);
qboolean PluQ_NNG_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Convert Quake vec3_t to FlatBuffers Vec3
static inline PluQ_Vec3_t QuakeVec3_To_FB(vec3_t v)
{
    PluQ_Vec3_t fb_vec;
    fb_vec.x = v[0];
    fb_vec.y = v[1];
    fb_vec.z = v[2];
    return fb_vec;
}

// Convert FlatBuffers Vec3 to Quake vec3_t
static inline void FB_Vec3_To_Quake(PluQ_Vec3_t fb_vec, vec3_t v)
{
    v[0] = fb_vec.x;
    v[1] = fb_vec.y;
    v[2] = fb_vec.z;
}

#endif // _PLUQ_NNG_H_
