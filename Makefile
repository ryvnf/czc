CC=gcc -std=c99
LEX=flex
INSTALL=install
RM=rm

CFLAGS=-g3 -Wall -Wno-switch -Wno-unused-variable
LFLAGS=--nounistd
PREFIX=/usr/local

SRC=$(wildcard *.c) lex.yy.c
OBJ=$(patsubst %.c,%.o,$(SRC)) lex.yy.o
BIN=czc

.PHONY: all
all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LINK) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -w -o $@ -c $^

ifeq (,$(shell which $(LEX)))
$(warning "Cannot update lex.yy.c because $(LEX) is not available")
else
lex.yy.c: lex.l
	$(LEX) $(LFLAGS) $<
endif

.PHONY: install
install:
	$(INSTALL) -m 0755 -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	$(RM) -f $(OUT) $(OBJ)
