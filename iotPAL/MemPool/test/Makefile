# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license.

IDIR=../
CC=g++

CFLAGS=-lstdc++ -std=c++0x -I$(IDIR)

.PHONY: all clean

all:
	$(CC) -c ../mempool.cpp -DVERBOSE_LOGGING_ENABLED=1 -o mempool.o
	$(CC) -c test.cpp -o test.o
	$(CC) -o app.o test.o mempool.o
	rm -rf test.o mempool.o

clean:
	rm -rf test.o mempool.o app.o