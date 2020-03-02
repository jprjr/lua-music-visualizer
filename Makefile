all: lua-music-visualizer

include Makefile.common

LUA = luajit
HOST_CC=$(CC)
PKGCONFIG = pkg-config

LUA_CFLAGS = $(shell $(PKGCONFIG) --cflags $(LUA))
LUA_LDFLAGS = $(shell $(PKGCONFIG) --libs $(LUA))

SAMPLERATE_CFLAGS  = $(shell $(PKGCONFIG) --cflags samplerate)
SAMPLERATE_LDFLAGS = $(shell $(PKGCONFIG) --libs samplerate)

ifeq ($(ENABLE_VGM),1)
VGM_CFLAGS = $(shell $(PKGCONFIG) --cflags vgm-player)
VGM_LDFLAGS = $(shell $(PKGCONFIG) --libs vgm-player)
endif

ifeq ($(ENABLE_FFTW3),1)
FFTW_CFLAGS = $(shell $(PKGCONFIG) --cflags fftw3)
FFTW_LDFLAGS = $(shell $(PKGCONFIG) --libs fftw3)
endif

CFLAGS += $(LUA_CFLAGS)
CFLAGS += $(SAMPLERATE_CFLAGS)
CFLAGS += $(VGM_CFLAGS)
CFLAGS += $(FFTW_CFLAGS)

LDFLAGS += $(LUA_LDFLAGS)
LDFLAGS += $(SAMPLERATE_LDFLAGS)
LDFLAGS += $(VGM_LDFLAGS)
LDFLAGS += $(FFTW_LDFLAGS)

LDFLAGS += -lm -pthread

lua-music-visualizer: $(OBJS)
	$(LINKER) -o $@ $^ $(LDFLAGS_LOCAL) $(LDFLAGS) $(LIBS)

release:
	docker build -t lua-music-vis .
	mkdir -p output
	docker run --rm -ti -v $(shell pwd)/output:/output lua-music-vis

everything:
	docker build -t lua-music-vis-max -f Dockerfile.everything .
	mkdir -p output
	docker run --rm -ti -v $(shell pwd)/output:/output lua-music-vis-max

