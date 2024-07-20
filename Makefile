# $^ (all prerequisites): refers to whatever is after the ":"
# $@ (target): refers to whatever is before the ":"
# $< (first prerequisite): refers to the first thing that is after the ":"
# | if obj directory exists execute the code, otherwise create it in line 23
CC = gcc
ALTCC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -pthread 

all: websockets
# all is the default action in makefiles
websockets: main.c 
	$(CC) $(CFLAGS) $^ -o $@ -lwebsockets -lcrypto -lssl -lz
embedded: main.c
	$(ALTCC) $(CFLAGS) $^ -o $@
test: test.c
	$(CC) $(CFLAGS) $^ -o $@ -lwebsockets -lcrypto -lssl -lz
clean:
	rm -f websockets embedded test