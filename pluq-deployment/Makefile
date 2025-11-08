# Makefile for PluQ test tools

CC = gcc
CFLAGS = -Wall -I../Quake
LDFLAGS = -L../Quake/nng_lib -lnng -Wl,-rpath,../Quake/nng_lib
LDFLAGS_FLATCC = -L../Quake/nng_lib -lnng -lflatccrt -Wl,-rpath,../Quake/nng_lib

all: test-monitor test-command

test-monitor: test-monitor.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
	@echo "Built test-monitor"

test-command: test-command.c
	$(CC) $(CFLAGS) $< $(LDFLAGS_FLATCC) -o $@
	@echo "Built test-command"

clean:
	rm -f test-monitor test-command

.PHONY: all clean
