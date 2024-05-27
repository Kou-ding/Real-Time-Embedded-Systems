# $^ (all prerequisites): refers to whatever is after the ":"
# $@ (target): refers to whatever is before the ":"
# $< (first prerequisite): refers to the first thing that is after the ":"
# | if obj directory exists execute the code, otherwise create it in line 23
CC = gcc
CFLAGS = -Wall -pthread

all: websockets
# all is the default action in makefiles
websockets: main.c 
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f websockets