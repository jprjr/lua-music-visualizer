.PHONY: all clean compile luajit
.SUFFIXES:
.PRECIOUS: src/%.lh src/%.o

OBJS = \
  src/main.o \
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
  src/str.o

LUALHS = \
  src/font.lua.lh \
  src/image.lua.lh \
  src/stream.lua.lh

BIN2CSRC = src/bin2c.c

CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS =

LIBLUAJIT = src/lua-5.3.5/src/liblua.a

HOST_CC = clang

TARGET = x86_64-w64-mingw32
CC = $(TARGET)-gcc
LD = $(TARGET)-gcc
EXT = .exe
LDFLAGS += -lwinmm -lshlwapi

#CC = clang
#LD = clang
#EXT =

all: lua-music-visualizer$(EXT)

compile: $(OBJS)

$(LIBLUAJIT):
	#make -C src/LuaJIT-2.1.0-beta3/src DEFAULT_CC="$(LUAJIT_DEFAULT_CC)" HOST_CC="$(HOST_CC)" CROSS="$(LUAJIT_CROSS)" TARGET_SYS="$(LUAJIT_TARGET_SYS)" MACOSX_DEPLOYMENT_TARGET=10.6 libluajit.a
	make -C src/lua-5.3.5/src CC="$(CC)" RANLIB="$(TARGET)-ranlib" AR="$(TARGET)-ar rcu" liblua.a

lua-music-visualizer$(EXT): $(OBJS) $(LIBLUAJIT)
	$(LD) -s -mconsole -o $@ $^ $(LDFLAGS)

%.o: %.c $(LUALHS)
	$(CC) $(CFLAGS) -o $@ -c $<

src/%.lh: lua/% src/bin2c
	./src/bin2c $< $@ $(patsubst %.lua,%_lua,$(notdir $<))

src/bin2c: src/bin2c.c
	$(HOST_CC) -o src/bin2c src/bin2c.c

clean:
	rm -f lua-music-visualizer$(EXT) lua-music-visualizer.exe $(OBJS)
	make -C src/LuaJIT-2.1.0-beta3 clean
	make -C src/lua-5.3.5 clean
