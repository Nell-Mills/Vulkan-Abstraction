# Compile the Vulkan Abstraction project demo.

DEPS_PROJECT :=\
$(wildcard ../*.c)

DEPS_THIRD_PARTY :=\
$(wildcard Third-Party/Volk/*.c)

CC	:= gcc -std=c99 -Wall -Wextra -Wno-unused-parameter
OUT	:= VKA-Demo
MAIN	:= VKA-Demo.c
DEPS	:= $(DEPS_PROJECT) $(DEPS_THIRD_PARTY)
CFLAGS	:= -I Third-Party
LFLAGS	:= -L Libs -Wl,-rpath '$$ORIGIN/Libs' -lSDL3
DEFINES	:= -D MAX_NAME_LENGTH=64 -D MAX_ERROR_LENGTH=1024
DEBUG	:= -D VKA_DEBUG -g -O0

release: $(MAIN)
	$(CC) $(MAIN) $(DEPS) $(DEFINES) $(CFLAGS) -o $(OUT) $(LFLAGS)

debug: $(MAIN)
	$(CC) $(MAIN) $(DEPS) $(DEFINES) $(DEBUG) $(CFLAGS) -o $(OUT) $(LFLAGS)
