# C& Programming Language — Native Compiler Makefile
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -I.

SRCS = src/cand_lexer.c src/cand_parser.c src/cand_semantic.c src/cand_codegen.c src/cand_driver.c
TARGET = cand.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o *.exe
