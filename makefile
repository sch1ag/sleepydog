CC=gcc
CFLAGS=-std=c11 -pthread

.PHONY: clean

sleepydog: sleepydog.c
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm -f sleepydog

