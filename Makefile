CC = gcc
CFLAGS = -DTOKEN=\"$(TGTK)\" -g -Wall -Wextra -O0
NAME = main
FILES = main.c
LDFLAGS = -lcrypto -lssl

build:
	$(CC) $(CFLAGS) -o $(NAME) $(FILES) $(LDFLAGS)

debug: build
	./$(NAME)

debug_gdb: build
	gdb ./$(NAME)
