CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o implement.o

nyush.o: nyush.c implement.h

implement.o: implement.c implement.h

.PHONY: clean
clean:
	rm -f *.o nyush
