/*
 * PluQ Minimal IPC Test
 * Tests only the PluQ IPC initialization without full game engine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nng/nng.h>

#define PLUQ_URL_RESOURCES "tcp://127.0.0.1:9001"
#define PLUQ_URL_GAMEPLAY  "tcp://127.0.0.1:9002"
#define PLUQ_URL_INPUT     "tcp://127.0.0.1:9003"

int main(void)
{
	int rv;
	nng_socket resources_req, gameplay_sub, input_push;
	nng_listener resources_listener, gameplay_listener;

	printf("PluQ Minimal IPC Test\n");
	printf("=====================\n\n");

	// Initialize nng
	if ((rv = nng_init(NULL)) != 0)
	{
		fprintf(stderr, "Failed to initialize nng: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ nng initialized\n");

	// 1. Resources channel (REQ/REP) - Backend listens
	if ((rv = nng_rep0_open(&resources_req)) != 0)
	{
		fprintf(stderr, "Failed to create REP socket: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Resources REP socket created\n");

	if ((rv = nng_listener_create(&resources_listener, resources_req, PLUQ_URL_RESOURCES)) != 0)
	{
		fprintf(stderr, "Failed to create resources listener: %s\n", nng_strerror(rv));
		return 1;
	}

	if ((rv = nng_listener_start(resources_listener, 0)) != 0)
	{
		fprintf(stderr, "Failed to start resources listener: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Resources channel listening on %s\n", PLUQ_URL_RESOURCES);

	// 2. Gameplay channel (PUB/SUB) - Backend publishes
	if ((rv = nng_pub0_open(&gameplay_sub)) != 0)
	{
		fprintf(stderr, "Failed to create PUB socket: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Gameplay PUB socket created\n");

	if ((rv = nng_listener_create(&gameplay_listener, gameplay_sub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		fprintf(stderr, "Failed to create gameplay listener: %s\n", nng_strerror(rv));
		return 1;
	}

	if ((rv = nng_listener_start(gameplay_listener, 0)) != 0)
	{
		fprintf(stderr, "Failed to start gameplay listener: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Gameplay channel listening on %s\n", PLUQ_URL_GAMEPLAY);

	// 3. Input channel (PUSH/PULL) - Backend pulls (listens)
	nng_listener input_listener;
	if ((rv = nng_pull0_open(&input_push)) != 0)
	{
		fprintf(stderr, "Failed to create PULL socket: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Input PULL socket created\n");

	if ((rv = nng_listener_create(&input_listener, input_push, PLUQ_URL_INPUT)) != 0)
	{
		fprintf(stderr, "Failed to create input listener: %s\n", nng_strerror(rv));
		return 1;
	}

	if ((rv = nng_listener_start(input_listener, 0)) != 0)
	{
		fprintf(stderr, "Failed to start input listener: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Input channel listening on %s\n", PLUQ_URL_INPUT);

	printf("\n✓ All 3 PluQ IPC channels initialized successfully!\n");
	printf("  - Resources:  REQ/REP on %s\n", PLUQ_URL_RESOURCES);
	printf("  - Gameplay:   PUB/SUB on %s\n", PLUQ_URL_GAMEPLAY);
	printf("  - Input:      PUSH/PULL on %s\n", PLUQ_URL_INPUT);

	// Clean up
	printf("\nCleaning up...\n");
	nng_socket_close(resources_req);
	nng_socket_close(gameplay_sub);
	nng_socket_close(input_push);

	printf("✓ Test complete!\n");
	return 0;
}
