CC=gcc
CFLAGS=-Wall -O3 -pipe

all:
	$(CC) $(CFLAGS) cbf.c -o cbf
