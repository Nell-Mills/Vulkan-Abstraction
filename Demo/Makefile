# Compile the Vulkan Abstraction project demo.

CONF_URL	:= https://gitlab.com/Nell-Mills/Config.git
CONF_DIR	:= Include/NM-Config
CONF_FILES	:= Config.h LICENSE
CONF_CLONE	:= git clone $(CONF_URL) $(CONF_DIR)/Temp && \
		$(foreach file, $(CONF_FILES), cp $(CONF_DIR)/Temp/$(file) $(CONF_DIR) &&)\
		rm -rf $(CONF_DIR)/Temp

DEPS_CLONE := $(CONF_CLONE) &&

DEPS_PROJECT :=\
$(wildcard ../*.c)

DEPS_THIRD_PARTY :=\
$(wildcard Include/Volk/*.c)

CC	:= gcc -std=c99 -Wall -Wextra -Wno-unused-parameter
OUT	:= -o VKA-Demo
MAIN	:= VKA-Demo.c
DEPS	:= $(DEPS_PROJECT) $(DEPS_THIRD_PARTY)
CFLAGS	:= -I Include
LFLAGS	:= -L Libs -Wl,-rpath '$$ORIGIN/Libs' -lSDL3
DEBUG	:= -D VKA_DEBUG -g -O0

release: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(CFLAGS) $(OUT) $(LFLAGS)

debug: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEBUG) $(CFLAGS) $(OUT) $(LFLAGS)
