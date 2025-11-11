/*
 * PluQ Command Sender - Send console commands to backend via InputCommand
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nng/nng.h>

#define PLUQ_URL_INPUT "tcp://127.0.0.1:9003"

// Include flatcc headers
#include "pluq_builder.h"

int main(int argc, char **argv)
{
	int rv;
	nng_socket push;
	nng_dialer dialer;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <command>\n", argv[0]);
		fprintf(stderr, "Example: %s \"map start\"\n", argv[0]);
		return 1;
	}

	// Concatenate all arguments into command string
	char cmd_text[256] = {0};
	for (int i = 1; i < argc; i++)
	{
		if (i > 1) strcat(cmd_text, " ");
		strncat(cmd_text, argv[i], sizeof(cmd_text) - strlen(cmd_text) - 1);
	}

	printf("Sending command: \"%s\"\n", cmd_text);

	// Initialize nng
	if ((rv = nng_init(NULL)) != 0)
	{
		fprintf(stderr, "Failed to initialize nng: %s\n", nng_strerror(rv));
		return 1;
	}

	// Create PUSH socket
	if ((rv = nng_push0_open(&push)) != 0)
	{
		fprintf(stderr, "Failed to create PUSH socket: %s\n", nng_strerror(rv));
		return 1;
	}

	// Connect to backend
	if ((rv = nng_dialer_create(&dialer, push, PLUQ_URL_INPUT)) != 0)
	{
		fprintf(stderr, "Failed to create dialer: %s\n", nng_strerror(rv));
		nng_socket_close(push);
		return 1;
	}

	if ((rv = nng_dialer_start(dialer, 0)) != 0)
	{
		fprintf(stderr, "Failed to connect to %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		nng_socket_close(push);
		return 1;
	}

	printf("Connected to backend input channel\n");

	// Build InputCommand FlatBuffer
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Create string first (must be done before starting table)
	flatbuffers_string_ref_t cmd_ref = flatbuffers_string_create_str(&builder, cmd_text);

	PluQ_Vec3_t angles = {0.0f, 0.0f, 0.0f};

	// Start buffer before building table
	flatcc_builder_start_buffer(&builder, 0, 0, 0);

	// Build InputCommand table using start/end pattern
	PluQ_InputCommand_start(&builder);
	PluQ_InputCommand_sequence_add(&builder, 0);
	PluQ_InputCommand_timestamp_add(&builder, 0.0);
	PluQ_InputCommand_forward_move_add(&builder, 0.0f);
	PluQ_InputCommand_side_move_add(&builder, 0.0f);
	PluQ_InputCommand_up_move_add(&builder, 0.0f);
	PluQ_InputCommand_view_angles_add(&builder, &angles);
	PluQ_InputCommand_buttons_add(&builder, 0);
	PluQ_InputCommand_impulse_add(&builder, 0);
	PluQ_InputCommand_cmd_text_add(&builder, cmd_ref);
	PluQ_InputCommand_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (!buf)
	{
		fprintf(stderr, "Failed to build FlatBuffer\n");
		flatcc_builder_clear(&builder);
		nng_socket_close(push);
		return 1;
	}

	printf("Sending %zu byte InputCommand message...\n", size);

	// Send via nng (use nng_msg for proper memory management in nng 2.0)
	nng_msg *msg;
	if ((rv = nng_msg_alloc(&msg, size)) != 0)
	{
		fprintf(stderr, "Failed to allocate message: %s\n", nng_strerror(rv));
		flatcc_builder_aligned_free(buf);
		flatcc_builder_clear(&builder);
		nng_socket_close(push);
		return 1;
	}

	memcpy(nng_msg_body(msg), buf, size);
	flatcc_builder_aligned_free(buf);

	if ((rv = nng_sendmsg(push, msg, 0)) != 0)
	{
		fprintf(stderr, "Failed to send: %s\n", nng_strerror(rv));
		nng_msg_free(msg);
		flatcc_builder_clear(&builder);
		nng_socket_close(push);
		return 1;
	}

	printf("Command sent successfully!\n");

	flatcc_builder_clear(&builder);
	nng_socket_close(push);
	return 0;
}
