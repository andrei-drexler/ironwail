# Makefile for PluQ test tools

CC = gcc
CFLAGS = -Wall -I../Quake
LDFLAGS = -L../Quake/nng_lib -lnng -Wl,-rpath,../Quake/nng_lib

all: test-monitor

test-monitor: test-monitor.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
	@echo "Built test-monitor"

clean:
	rm -f test-monitor

.PHONY: all clean
