/*
 * PluQ Monitor - Simple test program to receive and display PluQ frames
 * 
 * Connects to a PluQ backend and prints received frame data
 */

#include <stdio.h>
#include <stdlib.h>
#include <nng/nng.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>

#define PLUQ_URL_GAMEPLAY "ipc:///tmp/quake_pluq_gameplay"

static int running = 1;

void signal_handler(int sig)
{
	printf("\nReceived signal %d, shutting down...\n", sig);
	running = 0;
}

int main(void)
{
	int rv;
	nng_socket sub;
	
	printf("PluQ Monitor - FlatBuffers Frame Receiver\n");
	printf("==========================================\n\n");
	
	// Setup signal handler
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	
	// Open SUB socket
	if ((rv = nng_sub0_open(&sub)) != 0)
	{
		fprintf(stderr, "Failed to create SUB socket: %s\n", nng_strerror(rv));
		return 1;
	}
	
	// Subscribe to all topics (nng 2.0 API)
	if ((rv = nng_sub0_socket_subscribe(sub, "", 0)) != 0)
	{
		fprintf(stderr, "Failed to subscribe: %s\n", nng_strerror(rv));
		nng_socket_close(sub);
		return 1;
	}
	
	// Connect to backend
	printf("Connecting to %s...\n", PLUQ_URL_GAMEPLAY);
	if ((rv = nng_dial(sub, PLUQ_URL_GAMEPLAY, NULL, 0)) != 0)
	{
		fprintf(stderr, "Failed to dial: %s\n", nng_strerror(rv));
		nng_socket_close(sub);
		return 1;
	}
	
	printf("Connected! Waiting for frames...\n\n");
	
	uint32_t frame_count = 0;
	
	// Receive loop
	while (running)
	{
		nng_msg *msg;

		if ((rv = nng_recvmsg(sub, &msg, NNG_FLAG_NONBLOCK)) == 0)
		{
			frame_count++;
			size_t size = nng_msg_len(msg);
			printf("Frame %u: Received %zu bytes\n", frame_count, size);

			// TODO: Parse FlatBuffers message and display fields
			// For now, just show we're receiving data

			nng_msg_free(msg);
		}
		else if (rv != NNG_EAGAIN)
		{
			fprintf(stderr, "Receive error: %s\n", nng_strerror(rv));
			break;
		}

		usleep(16666);  // ~60 FPS
	}

	printf("\nTotal frames received: %u\n", frame_count);
	nng_socket_close(sub);
	return 0;
}
