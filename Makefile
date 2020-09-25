CC=gcc -std=c99
LEX=flex
INSTALL=install
RM=rm

CFLAGS=-g3 -Wall -Wno-switch -Wno-unused-variable
LFLAGS=--nounistd
PREFIX=/usr/local

SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
BIN=czc

.PHONY: all
all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LINK) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

.PHONY: install
install:
	$(INSTALL) -m 0755 -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	$(RM) -f $(OUT) $(OBJ)
