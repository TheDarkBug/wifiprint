CC = gcc
CFLAGS = -DTOKEN=\"$(TGTK)\" -Wall -Wextra -O3
NAME = main
FILES = main.c
LDFLAGS = -lcrypto -lssl
ifeq ($(PLATFORM), wasm)
	CC = emcc
	CFLAGS += -DWASM --no-entry -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS="['_wasm_main']"
	NAME = main.wasm
	LDFLAGS =
endif

build:
	$(CC) $(CFLAGS) -o $(NAME) $(FILES) $(LDFLAGS)

debug: build
	./$(NAME)

debug_gdb: build
	gdb ./$(NAME)
