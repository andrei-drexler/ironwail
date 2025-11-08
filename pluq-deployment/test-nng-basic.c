/*
 * Simple nng test - verify TCP transport works
 */

#include <stdio.h>
#include <nng/nng.h>

int main(void)
{
	int rv;
	nng_socket pub;
	nng_listener listener;

	printf("Testing nng TCP transport...\n");

	// Initialize nng (required in nng 2.0)
	if ((rv = nng_init(NULL)) != 0)
	{
		fprintf(stderr, "Failed to initialize nng: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Initialized nng\n");

	// Try to create a PUB socket
	if ((rv = nng_pub0_open(&pub)) != 0)
	{
		fprintf(stderr, "Failed to create PUB socket: %s\n", nng_strerror(rv));
		return 1;
	}
	printf("✓ Created PUB socket\n");

	// Create and start listener (nng 2.0 API)
	if ((rv = nng_listener_create(&listener, pub, "tcp://127.0.0.1:9999")) != 0)
	{
		fprintf(stderr, "Failed to create listener: %s\n", nng_strerror(rv));
		nng_socket_close(pub);
		return 1;
	}
	printf("✓ Created listener\n");

	if ((rv = nng_listener_start(listener, 0)) != 0)
	{
		fprintf(stderr, "Failed to start listener: %s\n", nng_strerror(rv));
		nng_socket_close(pub);
		return 1;
	}
	printf("✓ Listening on tcp://127.0.0.1:9999\n");

	nng_socket_close(pub);
	printf("✓ Test passed!\n");
	return 0;
}
