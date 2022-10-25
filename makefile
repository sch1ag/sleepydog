CC=gcc
CFLAGS=-std=c11 -pthread

.PHONY: clean aixcheck

sleepydog: sleepydog.c
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm -f sleepydog

