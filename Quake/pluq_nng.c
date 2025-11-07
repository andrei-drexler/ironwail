/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq_nng.c -- PluQ nng + FlatBuffers IPC implementation

#include "pluq_nng.h"
#include <string.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

static pluq_nng_context_t pluq_ctx;

// ============================================================================
// INITIALIZATION / SHUTDOWN
// ============================================================================

qboolean PluQ_NNG_Init(qboolean is_backend)
{
    int rv;

    memset(&pluq_ctx, 0, sizeof(pluq_ctx));
    pluq_ctx.is_backend = is_backend;
    pluq_ctx.is_frontend = !is_backend;

    Con_Printf("PluQ NNG: Initializing %s mode\n", is_backend ? "backend" : "frontend");

    if (is_backend)
    {
        // Backend: Set up REP socket for resource requests
        if ((rv = nng_rep0_open(&pluq_ctx.resources_rep)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to create resources REP socket: %s\n", nng_strerror(rv));
            goto error;
        }
        if ((rv = nng_listen(pluq_ctx.resources_rep, PLUQ_URL_RESOURCES, NULL, 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to listen on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
            goto error;
        }

        // Backend: Set up PUB socket for gameplay events
        if ((rv = nng_pub0_open(&pluq_ctx.gameplay_pub)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to create gameplay PUB socket: %s\n", nng_strerror(rv));
            goto error;
        }
        if ((rv = nng_listen(pluq_ctx.gameplay_pub, PLUQ_URL_GAMEPLAY, NULL, 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to listen on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
            goto error;
        }

        // Backend: Set up PULL socket for input commands
        if ((rv = nng_pull0_open(&pluq_ctx.input_pull)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to create input PULL socket: %s\n", nng_strerror(rv));
            goto error;
        }
        if ((rv = nng_listen(pluq_ctx.input_pull, PLUQ_URL_INPUT, NULL, 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to listen on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
            goto error;
        }

        Con_Printf("PluQ NNG: Backend initialized successfully\n");
    }
    else
    {
        // Frontend: Set up REQ socket for resource requests
        if ((rv = nng_req0_open(&pluq_ctx.resources_req)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to create resources REQ socket: %s\n", nng_strerror(rv));
            goto error;
        }
        if ((rv = nng_dial(pluq_ctx.resources_req, PLUQ_URL_RESOURCES, NULL, 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to dial %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
            goto error;
        }

        // Frontend: Set up SUB socket for gameplay events
        if ((rv = nng_sub0_open(&pluq_ctx.gameplay_sub)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to create gameplay SUB socket: %s\n", nng_strerror(rv));
            goto error;
        }
        // Subscribe to all topics (empty topic = all)
        if ((rv = nng_sub0_socket_subscribe(pluq_ctx.gameplay_sub, "", 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to subscribe to gameplay events: %s\n", nng_strerror(rv));
            goto error;
        }
        if ((rv = nng_dial(pluq_ctx.gameplay_sub, PLUQ_URL_GAMEPLAY, NULL, 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to dial %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
            goto error;
        }

        // Frontend: Set up PUSH socket for input commands
        if ((rv = nng_push0_open(&pluq_ctx.input_push)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to create input PUSH socket: %s\n", nng_strerror(rv));
            goto error;
        }
        if ((rv = nng_dial(pluq_ctx.input_push, PLUQ_URL_INPUT, NULL, 0)) != 0)
        {
            Con_Printf("PluQ NNG: Failed to dial %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
            goto error;
        }

        Con_Printf("PluQ NNG: Frontend initialized successfully\n");
    }

    pluq_ctx.initialized = true;
    return true;

error:
    PluQ_NNG_Shutdown();
    return false;
}

void PluQ_NNG_Shutdown(void)
{
    if (!pluq_ctx.initialized)
        return;

    Con_Printf("PluQ NNG: Shutting down\n");

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
}

// ============================================================================
// RESOURCES CHANNEL (REQ/REP)
// ============================================================================

qboolean PluQ_NNG_Backend_WaitForResourceRequest(void)
{
    if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
        return false;

    // This is a placeholder - actual implementation would receive and store the request
    // For now, just return false to indicate no request available
    return false;
}

qboolean PluQ_NNG_Backend_SendResource(const void *flatbuf, size_t size)
{
    int rv;

    if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
        return false;

    if ((rv = nng_send(pluq_ctx.resources_rep, (void *)flatbuf, size, 0)) != 0)
    {
        Con_Printf("PluQ NNG: Failed to send resource: %s\n", nng_strerror(rv));
        return false;
    }

    return true;
}

qboolean PluQ_NNG_Frontend_RequestResource(uint32_t resource_id)
{
    // This is a placeholder - actual implementation would build a ResourceRequest
    // message and send it via the REQ socket
    if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
        return false;

    // TODO: Build ResourceRequest FlatBuffer and send
    Con_Printf("PluQ NNG: Requesting resource ID %u (not yet implemented)\n", resource_id);

    return false;
}

qboolean PluQ_NNG_Frontend_ReceiveResource(void **flatbuf_out, size_t *size_out)
{
    int rv;
    nng_msg *msg;

    if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
        return false;

    if ((rv = nng_recvmsg(pluq_ctx.resources_req, &msg, NNG_FLAG_NONBLOCK)) != 0)
    {
        if (rv != NNG_EAGAIN)
            Con_Printf("PluQ NNG: Failed to receive resource: %s\n", nng_strerror(rv));
        return false;
    }

    *flatbuf_out = nng_msg_body(msg);
    *size_out = nng_msg_len(msg);
    // Note: Caller must call nng_msg_free(msg) when done
    return true;
}

// ============================================================================
// GAMEPLAY CHANNEL (PUB/SUB)
// ============================================================================

qboolean PluQ_NNG_Backend_PublishFrame(const void *flatbuf, size_t size)
{
    int rv;

    if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
        return false;

    if ((rv = nng_send(pluq_ctx.gameplay_pub, (void *)flatbuf, size, 0)) != 0)
    {
        Con_Printf("PluQ NNG: Failed to publish gameplay frame: %s\n", nng_strerror(rv));
        return false;
    }

    return true;
}

qboolean PluQ_NNG_Frontend_ReceiveFrame(void **flatbuf_out, size_t *size_out)
{
    int rv;
    nng_msg *msg;

    if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
        return false;

    if ((rv = nng_recvmsg(pluq_ctx.gameplay_sub, &msg, NNG_FLAG_NONBLOCK)) != 0)
    {
        if (rv != NNG_EAGAIN)
            Con_Printf("PluQ NNG: Failed to receive gameplay frame: %s\n", nng_strerror(rv));
        return false;
    }

    *flatbuf_out = nng_msg_body(msg);
    *size_out = nng_msg_len(msg);
    // Note: Caller must call nng_msg_free(msg) when done
    return true;
}

// ============================================================================
// INPUT CHANNEL (PUSH/PULL)
// ============================================================================

qboolean PluQ_NNG_Frontend_SendInput(const void *flatbuf, size_t size)
{
    int rv;

    if (!pluq_ctx.initialized || !pluq_ctx.is_frontend)
        return false;

    if ((rv = nng_send(pluq_ctx.input_push, (void *)flatbuf, size, 0)) != 0)
    {
        Con_Printf("PluQ NNG: Failed to send input command: %s\n", nng_strerror(rv));
        return false;
    }

    return true;
}

qboolean PluQ_NNG_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out)
{
    int rv;
    nng_msg *msg;

    if (!pluq_ctx.initialized || !pluq_ctx.is_backend)
        return false;

    if ((rv = nng_recvmsg(pluq_ctx.input_pull, &msg, NNG_FLAG_NONBLOCK)) != 0)
    {
        if (rv != NNG_EAGAIN)
            Con_Printf("PluQ NNG: Failed to receive input command: %s\n", nng_strerror(rv));
        return false;
    }

    *flatbuf_out = nng_msg_body(msg);
    *size_out = nng_msg_len(msg);
    // Note: Caller must call nng_msg_free(msg) when done
    return true;
}
