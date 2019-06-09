all: lua-music-visualizer

include Makefile.common

LUA = luajit

OBJS += src/console.o
CFLAGS += $(shell pkg-config --cflags $(LUA))
LDFLAGS += $(shell pkg-config --libs $(LUA)) -lm -pthread
CLEAN += lua-music-visualizer

lua-music-visualizer: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

