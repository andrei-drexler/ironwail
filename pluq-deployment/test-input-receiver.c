/*
 * PluQ Input Command Receiver - Test input channel
 * Demonstrates that the backend can receive and parse InputCommand messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nng/nng.h>
#include <signal.h>
#include <unistd.h>

#define PLUQ_URL_INPUT "tcp://127.0.0.1:9003"

// Include FlatBuffers reader
#include "pluq_reader.h"

static volatile int keep_running = 1;

void sigint_handler(int sig)
{
	(void)sig;
	keep_running = 0;
}

int main(void)
{
	int rv;
	nng_socket pull;
	nng_listener listener;
	int msg_count = 0;

	printf("=== PluQ Input Command Receiver Test ===\n");
	printf("This simulates the backend receiving input commands\n");
	printf("Press Ctrl+C to stop\n\n");

	// Set up signal handler
	signal(SIGINT, sigint_handler);

	// Initialize nng
	if ((rv = nng_init(NULL)) != 0)
	{
		fprintf(stderr, "Failed to initialize nng: %s\n", nng_strerror(rv));
		return 1;
	}

	// Create PULL socket (backend side)
	if ((rv = nng_pull0_open(&pull)) != 0)
	{
		fprintf(stderr, "Failed to create PULL socket: %s\n", nng_strerror(rv));
		return 1;
	}

	// Create listener
	if ((rv = nng_listener_create(&listener, pull, PLUQ_URL_INPUT)) != 0)
	{
		fprintf(stderr, "Failed to create listener for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		nng_socket_close(pull);
		return 1;
	}

	// Start listener
	if ((rv = nng_listener_start(listener, 0)) != 0)
	{
		fprintf(stderr, "Failed to start listener on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		nng_socket_close(pull);
		return 1;
	}

	printf("Listening on %s\n", PLUQ_URL_INPUT);
	printf("Waiting for input commands...\n\n");

	// Receive loop
	while (keep_running)
	{
		nng_msg *msg;

		// Try to receive with timeout
		if ((rv = nng_recvmsg(pull, &msg, NNG_FLAG_NONBLOCK)) != 0)
		{
			if (rv != NNG_EAGAIN)
			{
				fprintf(stderr, "Receive error: %s\n", nng_strerror(rv));
			}
			usleep(100000); // 100ms
			continue;
		}

		// Got a message!
		msg_count++;
		size_t size = nng_msg_len(msg);
		void *data = nng_msg_body(msg);

		printf("╔═══════════════════════════════════════════════════════════════\n");
		printf("║ Message #%d (size: %zu bytes)\n", msg_count, size);
		printf("╠═══════════════════════════════════════════════════════════════\n");

		// Parse FlatBuffers InputCommand
		PluQ_InputCommand_table_t cmd = PluQ_InputCommand_as_root(data);
		if (!cmd)
		{
			printf("║ ERROR: Failed to parse InputCommand FlatBuffer\n");
			printf("╚═══════════════════════════════════════════════════════════════\n\n");
			nng_msg_free(msg);
			continue;
		}

		// Extract fields
		uint32_t sequence = PluQ_InputCommand_sequence(cmd);
		double timestamp = PluQ_InputCommand_timestamp(cmd);
		float forward_move = PluQ_InputCommand_forward_move(cmd);
		float side_move = PluQ_InputCommand_side_move(cmd);
		float up_move = PluQ_InputCommand_up_move(cmd);
		PluQ_Vec3_struct_t angles = PluQ_InputCommand_view_angles(cmd);
		uint32_t buttons = PluQ_InputCommand_buttons(cmd);
		uint8_t impulse = PluQ_InputCommand_impulse(cmd);
		const char *cmd_text = PluQ_InputCommand_cmd_text(cmd);

		// Display parsed data
		printf("║ Sequence:    %u\n", sequence);
		printf("║ Timestamp:   %.3f\n", timestamp);
		printf("║ Movement:    forward=%.1f side=%.1f up=%.1f\n", forward_move, side_move, up_move);
		printf("║ View Angles: (%.1f, %.1f, %.1f)\n", angles->x, angles->y, angles->z);
		printf("║ Buttons:     0x%08X\n", buttons);
		printf("║ Impulse:     %u\n", impulse);

		if (cmd_text && cmd_text[0])
		{
			printf("║ \n");
			printf("║ ╔═══ CONSOLE COMMAND ═══════════════════════════════════╗\n");
			printf("║ ║ %s\n", cmd_text);
			printf("║ ╚═══════════════════════════════════════════════════════╝\n");
			printf("║ \n");
			printf("║ → This command would be executed via: Cbuf_AddText(\"%s\\n\")\n", cmd_text);
		}
		else
		{
			printf("║ Command:     (none)\n");
		}

		printf("╚═══════════════════════════════════════════════════════════════\n\n");

		nng_msg_free(msg);
	}

	printf("\nShutting down...\n");
	printf("Total messages received: %d\n", msg_count);

	nng_socket_close(pull);
	return 0;
}
