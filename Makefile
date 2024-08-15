################## Basic Makefile Symbols ###################
# $^ (all prerequisites): refers to whatever is after the ":"
# $@ (target): refers to whatever is before the ":"
# $< (first prerequisite): refers to the first thing that is after the ":"

# Compiler with Compiler and Linker flags
CC=gcc
CFLAGS=-Wall -pthread
LDFLAGS=-ljansson -lwebsockets

# Cross Compiler with Compiler and Linker flags
CROSSCC=/home/kou/coding/Real-Time-Embedded-Systems/musl/aarch64-linux-musl-cross/bin/aarch64-linux-musl-gcc
CROSSCFLAGS=-Wall -pthread
CROSSLDFLAGS=-static \
-I/home/kou/coding/Real-Time-Embedded-Systems/libwebsockets-build/include \
-I/home/kou/coding/Real-Time-Embedded-Systems/jansson-build/include \
-I/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/include \
-I/home/kou/coding/Real-Time-Embedded-Systems/zlib-build/include \
-L/home/kou/coding/Real-Time-Embedded-Systems/libwebsockets-build/lib \
-L/home/kou/coding/Real-Time-Embedded-Systems/jansson-build/lib \
-L/home/kou/coding/Real-Time-Embedded-Systems/openssl-build/lib \
-L/home/kou/coding/Real-Time-Embedded-Systems/zlib-build/lib \
-lwebsockets -lssl -lcrypto -lz -ljansson -ldl

# all is the default action in makefiles
all: websockets

# the main program
websockets: main.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
# the main program for the embedded system (raspberry pi)
embedded: main.c
	$(CROSSCC) $(CROSSCFLAGS) $^ -o $@ $(CROSSLDFLAGS)
# the websocket test program
wstest: wstest.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
# the pthreads test program
ptest: ptest.c
	$(CC) $(CFLAGS) $^ -o $@
# the cross compiler test program 
hello: hello.c
	$(CROSSCC) $(CROSSCFLAGS) $^ -o $@ $(CROSSLDFLAGS)
	
# Cleanup functionality
clean:
	rm -f websockets embedded wstest ptest hello
	rm -f logs/*.txt 
	rm -f candlesticks/*.txt 
	rm -f averages/*.txt
	rm -f delays/*.csv
	rm -f python_websockets/*.png