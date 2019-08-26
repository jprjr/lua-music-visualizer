all: lua-music-visualizer

include Makefile.common

LUA = luajit
HOST_CC=$(CC)
PKGCONFIG = pkg-config

OBJS += src/console.o src/mpd_ez.o src/mpdc.o src/scan.o

CFLAGS += $(shell $(PKGCONFIG) --cflags $(LUA))
LDFLAGS += $(shell $(PKGCONFIG) --libs $(LUA)) -lm -pthread

CLEAN += lua-music-visualizer

lua-music-visualizer: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

release:
	docker build -t lua-music-vis .
	mkdir -p output
	docker run --rm -ti -v $(shell pwd)/output:/output lua-music-vis
