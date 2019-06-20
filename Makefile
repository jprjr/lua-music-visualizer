all: lua-music-visualizer

include Makefile.common

#LUA = luajit
HOST_CC=$(CC)

OBJS += src/console.o

CFLAGS += -I/usr/local/include
LDFLAGS += -llua -ldl -lm -pthread

#CFLAGS += $(shell pkg-config --cflags $(LUA))
#LDFLAGS += $(shell pkg-config --libs $(LUA)) -lm -pthread

CLEAN += lua-music-visualizer

lua-music-visualizer: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

