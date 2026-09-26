# C& Programming Language — Native Compiler Makefile
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I. -Isrc
LDFLAGS ?= -lm

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/cand

SRCS = src/cand_lexer.c src/cand_parser.c src/cand_semantic.c src/cand_codegen.c src/cand_driver.c

ifeq ($(OS),Windows_NT)
    TARGET = cand.exe
else
    TARGET = cand
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/cand
	install -d $(DESTDIR)$(DATADIR)
	cp -r std $(DESTDIR)$(DATADIR)/
	install -m 644 cand.toml $(DESTDIR)$(DATADIR)/cand.toml

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/cand
	rm -rf $(DESTDIR)$(DATADIR)

clean:
	rm -f cand cand.exe *.o *.exe cand_run_tmp* cand_build_tmp.bat
