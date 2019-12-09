all: lua-music-visualizer

include Makefile.common

LUA = luajit
HOST_CC=$(CC)
PKGCONFIG = pkg-config

CFLAGS += $(shell $(PKGCONFIG) --cflags $(LUA)) $(OPT_CFLAGS)
LDFLAGS += $(shell $(PKGCONFIG) --libs $(LUA)) -lm -pthread

CLEAN += lua-music-visualizer

lua-music-visualizer: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

release:
	docker build -t lua-music-vis .
	mkdir -p output
	docker run --rm -ti -v $(shell pwd)/output:/output lua-music-vis

debug:
	docker build -t lua-music-vis-debug -f Dockerfile.debug .
	mkdir -p output
	docker run --rm -ti -v $(shell pwd)/tests:/tests lua-music-vis-debug
