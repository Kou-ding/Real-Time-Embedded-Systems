################## Basic Makefile Symbols ###################
# $^ (all prerequisites): refers to whatever is after the ":"
# $@ (target): refers to whatever is before the ":"
# $< (first prerequisite): refers to the first thing that is after the ":"

# Compilers
CC=gcc
CROSSCC=aarch64-none-linux-gnu-gcc
# Compiler flags
CFLAGS=-Wall -pthread -I./libwebsockets/include
# Linker flags
LDFLAGS=-lwebsockets -lcrypto -lssl -lz -ljansson
# CrossCompiling-linker flags
CROSSLDFLAGS=-static -lwebsockets -lcrypto -lssl -lz -ljansson

# all is the default action in makefiles
all: websockets

# the main program
websockets: main.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
# the main program for the embedded system (raspberry pi)
embedded: main.c
	$(CROSSCC) $(CFLAGS) $^ -o $@ $(CROSSLDFLAGS)
# the websocket test program
wstest: wstest.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
# the pthreads test program
ptest: ptest.c
	$(CC) $(CFLAGS) $^ -o $@

# Cleanup functionality
clean:
	rm -f websockets embedded wstest ptest 
	rm -f logs/*.txt 
	rm -f candlesticks/*.txt 
	rm -f averages/*.txt
	rm -f delays/*.csv
	rm -f python_websockets/*.png