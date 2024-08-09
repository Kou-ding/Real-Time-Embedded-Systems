################## Basic Makefile Symbols ###################
# $^ (all prerequisites): refers to whatever is after the ":"
# $@ (target): refers to whatever is before the ":"
# $< (first prerequisite): refers to the first thing that is after the ":"

CC = gcc
ALTCC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -pthread 

# all is the default action in makefiles
all: websockets

# the main program
websockets: main.c 
	$(CC) $(CFLAGS) $^ -o $@ -lwebsockets -lcrypto -lssl -lz -ljansson
# the main program for the embedded system (raspberry pi)
embedded: main.c
	$(ALTCC) $(CFLAGS) $^ -o $@ -lwebsockets -lcrypto -lssl -lz -ljansson
# the websocket test program
wstest: wstest.c
	$(CC) $(CFLAGS) $^ -o $@ -lwebsockets -lcrypto -lssl -lz -ljansson
# the pthreads test program
ptest: ptest.c
	$(CC) $(CFLAGS) $^ -o $@

# cleanup functionality
clean:
	rm -f websockets embedded wstest ptest logs/*.txt candlesticks/*.txt
