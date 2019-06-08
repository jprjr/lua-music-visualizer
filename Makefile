.PHONY: all clean
.SUFFIXES:
.PRECIOUS: src/%.lh

all: lua-music-visualizer

LUA = lua

OBJS = \
  src/console.o \
  src/video-generator.o \
  src/lua-image.o \
  src/lua-file.o \
  src/lua-audio.o \
  src/audio-processor.o \
  src/kiss_fftr.o \
  src/kiss_fft.o \
  src/audio-decoder.o \
  src/image.o \
  src/thread.o \
  src/utf.o \
  src/str.o \
  src/unpack.o \
  src/pack.o \
  src/char.o \

LUALHS = \
  src/font.lua.lh \
  src/image.lua.lh \
  src/stream.lua.lh

BIN2CSRC = src/bin2c.c

CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags $(LUA))
LDFLAGS = $(shell pkg-config --libs $(LUA)) -lm -pthread

lua-music-visualizer: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c $(LUALHS)
	$(CC) $(CFLAGS) -o $@ -c $<

src/%.lh: lua/% src/bin2c
	./src/bin2c $< $@ $(patsubst %.lua,%_lua,$(notdir $<))

src/bin2c: src/bin2c.c
	$(CC) -o src/bin2c src/bin2c.c


clean:
	rm -f lua-music-visualizer $(OBJS) $(LUALHS)
