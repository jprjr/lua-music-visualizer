# lua-music-visualizer

This is a program to create videos from music files, using Lua. It's the successor
to my [mpd-visualizer](https://github.com/jprjr/mpd-visualizer) and keeps much of the
same API. It's able to decode audio from several formats (FLAC, MP3, WAVE, and raw PCM),
making it suitable for creating single videos offline, or for a never-ending livestream
with MPD.

Unlike `mpd-visualizer`, this only allows piping the raw video into some sub-process,
whether it's `ffmpeg`, `ffplay`. You can save videos locally using `ffmpeg` to encode,
or just use `cat` and redirect your standard output to a file if you want to save
the raw RGB video (warning: this will be a large file).

There's a CLI interface as well as a GUI interface.

# Usage

```bash
lua-music-visualizer \
  --about (shows licensing info and quits) \
  --width=1280 (video width) \
  --height=720 (video height) \
  --fps=30 (video fps) \
  --bars=24 (number of spectrum analyzer bars to compute) \
  --samplerate=48000 (input sample rate, only for raw PCM) \
  --channels=2 (input audio channels, only for raw PCM) \
  --resample=48000 (desired output sample rate, off by default) \
  -joff (disable JIT) \
  -l<modulename> (calls require("modulename") when Lua is initialized) \
  --probe (probes music file for meta info, ignores remaining paramters) \
  /path/to/song.mp3/flac/wave/raw \
  /path/to/lua/script.lua \
  prog args...
```

## MPD

`lua-music-visualizer` will connect to MPD if the `MPD_HOST` environment variable is set,
otherwise it won't connect.

On Unix/Linux, you can connect via TCP/IP or Unix sockets. On Windows, you can only
connect using TCP/IP.

If your MPD instances needs a password, use `MPD_HOST=password@hostname` or
`MPD_HOST=password@/path/to/socket`

If you need to connect on a different TCP port, set the `MPD_PORT` environment
variable.

## Requirements

* Lua or LuaJIT
* libsamplerate
* (optional) FFTW3
* (optional) [snes_spc](https://github.com/jprjr/snes_spc)
* (optional) [libid666](https://github.com/jprjr/libid666)
* (optional) [nsfplay](https://github.com/bbbradsmith/nsfplay)
* (optional) [libvgm](https://github.com/ValleyBell/libvgm)
* (optional) [libnezplug](https://github.com/jprjr/libnezplug)

Note that the optional libraries have different licensing, compiling against them
may make the resulting binary non-redistributable.

* FFTW3: GPL 2, resulting binary is non-redistributable?
  * TODO: Am I in violation of the GPL by optionally using FFTW3 in my non-GPL app?
* snes_spc: LGPL 2.1, no effect on redistribution/licensing.
* libid666: MIT, no effect on redistribution/licensing.
* libnezplug: public domain, no effect no redistribution.
* nsfplay: custom license, unsure of effect on redistribution/licensing.
* libvgm: unknown license, unsure of effect on redistribution/licensing.

## Installation

Hopefully you can just run `make`. Look at the Makefile if that doesn't work.

Different decoders can be enabled/disabled with your `make` command. The available
parameters (and their default state) are:

* `ENABLE_LIBSAMPLERATE=1`
* `ENABLE_PCM=1`
* `ENABLE_WAV=1`
* `ENABLE_MP3=1`
* `ENABLE_FLAC=1`
* `ENABLE_SPC=0` (requires `snes_spc` and `libid666`, see above for URLs)
* `ENABLE_NEZ=0` (requires `libnezplug`, see above for URL)
* `ENABLE_NSF=0` (requires `nsfplay`, see above for URL)
* `ENABLE_VGM=0` (requires `libvgm`, see above for URL)
* `ENABLE_ALL=0` (enables all decoders)

I've also included Docker files for cross-compiling to Windows, Linux, and OSX.

The OSX cross-compiler requires a copy of the macOS SDK. Long-story short, you'll
have to build your own local copy of my [macOS cross-compiler image](https://github.com/jprjr/docker-osxcross)
before using the Docker images.

I've added easy targets to the Makefile for using Docker:

* `make release` makes the standard release binaries.
    * Decodes raw PCM, WAV, FLAC, MP3
    * Uses libsamplerate
    * Uses LuaJIT
    * Uses KISSFFT
* `make everything` makes binaries with all decoders, which may not be redistributable.
    * Same as above, also decodes SPC, NSF/NSFe, VGM, HES, KSS, GBS, GBR, AY, SGC, NSD
    * Uses snes_spc
    * Uses NSFPlay
    * Uses libvgm
    * uses libnezplug

## What happens

When `lua-music-visualizer` starts up, it will attempt to open your songfile
and parse metadata. If the `MPD_HOST` environment variable is set, it will
connect to MPD.

Your Lua script should return either a function, or an object.

```lua
return function()
  print('making a video frame')
end
```

Or:

```lua
return {
  onframe = function(self)
    print('making a video frame')
  end
}
```

Or, if you want to be more object-oriented:

```lua
local M = {}
M.__index = M

function M.new()
  local self = {}
  setmetatable(self,M)
  return self
end

function M:onframe()
  print('making a video frame')
end

function M:onload()
  print('performing initialization stuff')
end

function M:onunload()
  print('shutting down')
end

function M:onchange(e)
  print('MPD saw a change, type: ' .. e)
end

function M:onreload()
  print('visualizer requested reload')
end

return M.new()
```

The only required function is `onframe`.

`onchange` is used when connected to MPD, and called whenever
a new message comes in, or the player status changes
(such as the song changing).

`onload` is called at app start-up, and `onunload` is called
when quitting.

`onreload` is used to signal a reload. The program does
*not* reload your main script, it just calls your own
`onreload` function, which you could use to perform your
own reloading procedures. On UNIX/Linux, the reload is
triggered by sending a `USR1` signal to the process.

For reference, here's all the function signatures you can expose:

* `onframe(self)`
* `onchange(self,event)`
* `onload(self)`
* `onunload(self)`
* `onreload(self)`


# The Lua environment

## Globals

Before any script is called, your Lua folder is added to the `package.path` variable,
meaning you can create submodules within your Lua folder and load them using `require`.

Within your Lua script, you have a few pre-defined global variables:

* `stream` - a table representing the video stream
* `image` - a module for loading image files
* `font` - a module for loading BDF fonts
* `file` - a module for filesystem operations
* `song` - a table of what's playing, from MPD. Also used for message relaying

### The global `stream` object

The `stream` table has two keys:

* `stream.video` - this represents the current frame of video, it's actually an instance of a `frame` which has more details below
  * `stream.video.framerate` - the video framerate
* `stream.audio` - a table of audio data
  * `stream.audio.samplerate` - audio samplerate, like `48000`
  * `stream.audio.channels` - audio channels, like `2`
  * `stream.audio.samplesize` - sample size in bytes, like `2` for 16-bit audio
  * `stream.audio.freqs` - an array of available frequencies, suitable for making a visualizer
  * `stream.audio.amps` - an array of available amplitudes, suitable for making a visualizer - values between 0.0 and 1.0
  * `stream.audio.spectrum_len` - the number of available amplitudes/frequencies

### The global `image` object

The `image` module can load most images, including GIFs. All images have a 2-stage loading process. Initially, it
just probes the image for information like height, width, etc. You can then load the image synchronously or asynchronously.
If you're loading images in the `onload` function (that is, at the very beginning of the program's execution), its safe
to load images synchronously. Otherwise, you should load images asynchronously.

* `img = image.new(filename, width, height, channels)`
  * Either filename is required, or `width/height/channels` if you pass `nil` for the filename
  * If filename is given, this will probe an image file. Returns an image object on success, nil on failure
    * If width, height, or channels is 0 or nil, then the image will not be resized or processed
    * If width or height are set, the image will be resized
    * If channels is set, the image will be forced to use that number of channels
      * Basically, channels = 3 for most bitmaps, channels = 4 for transparent images.
    * The actual image data is NOT loaded, use `img:load()` to load data.
  * If filename is nil, then an empty image is created with the given width/height/channels

Scroll down to "Image Instances" for details on image methods like `img:load()`

### The global `font` object

The `font` object can load BDF (bitmap) fonts.

* `f = font.new(filename)`
  * Loads a BDF font and returns a font object

Scroll down to "Font Instances" for details on font methods

### The global `file` object

The `file` object has methods for common file operations:

* `dir = file.ls(path)`
  * Lists files in a directory
  * Returns an array of file objects with two keys:
    * `file` - the actual file path
    * `mtime` - file modification time

* `dirname = file.dirname(path)`
  * Equivalent to the [dirname call](http://pubs.opengroup.org/onlinepubs/009696799/functions/dirname.html)

* `basename = file.basename(path)`
  * Equivalent to the [basename call](http://pubs.opengroup.org/onlinepubs/009696799/functions/basename.html)

* `realpath = file.realpath(path)`
  * Equivalent to the [realpath call](http://pubs.opengroup.org/onlinepubs/009696799/functions/realpath.html)

* `cwd = file.getcwd()`
  * Equivalent to the [getcwd call](http://pubs.opengroup.org/onlinepubs/009695399/functions/getcwd.html)

* `ok = file.exists(path)`
  * Returns `true` if a path exists, `nil` otherwise.

### The global `song` object

The `song` object has metadata on the current song. The only guaranteed key is `elapsed`. Everything else can be nil.

If you're playing from a local FLAC/MP3/WAVE file, these will be pre-populated. If you're
connected to MPD, it may take a frame or two for these to be populated.

* `song.file` - the filename of the playing song
* `song.id` - the id of the playing song
* `song.elapsed` - the elapsed time of the current song, in seconds
* `song.total` - the total time of the current song, in seconds
* `song.title` - the title of the current song
* `song.artist` - the artist of the current song
* `song.album` - the album of the current song
* `song.message` - `lua-music-visualizer` uses MPD's [client-to-client](https://www.musicpd.org/doc/protocol/client_to_client.html) functionality, It listens on a channel named `visualizer`, if there's a new message on that channel, it will appear here in the song object.
* `song.sendmessage(msg)` - allows sending a message on the `visualier` channel.

## Image Instances

An image instance has the following methods and properties

* `img.state` - one of `error`, `unloaded`, `loading`, `loaded`, `fixed`
* `img.width` - the image width
* `img.height` - the image height
* `img.channels` - the image channels (3 for RGB, 4 for RGBA)
* `img.frames` - only available after calling `img:load`, an array of one or more frames
* `img.framecount` - only available after calling `img:load`, total number of frames in the `frames` array
* `img.delays` - only available after calling `img:load` - an array of frame delays (only applicable to gifs)
* `img.rotated` - only available after calling `img:rotate()`, a frame that's been resized and rotated
* `img.tiles` - only available after calling `img:tile()`, a 2d array of tiles (x, then y)
* `img:load(async)` - loads an image into memory
  * If `async` is true, image is loaded in the background and available on some future iteration of `onframe`
  * else, image is loaded immediately
* `img:unload()` - unloads an image from memory
* `img:rotate(frameno,degrees)` - rotates a frame (by number) clockwise into a new frame, saved as `rotation`
  * the rotated frame has two extra fields, `width_offset` and `height_offset`
* `img:tile(frameno,width,height)` - copies a frame into `width` x `height` tiles, saved as `tiled`
  * the array is a 2d array, arranged as rows, then columns.
  * `tiled[1][1]` is the top-left tile
  * `tiled[1][x]` is the (xth) column of the first row
  * `tiled[x][1]` is the top-most tile of the `xth` row

If `img:load()` fails, either asynchronously or synchronously, then the `state` key will be set to `error`

### Frame instances
Once the image is loaded, it will contain an array of frames. Additionally, `stream.video` is an instance of a `frame`

For convenience, most `frame` functions can be used on the `stream` object directly, instead of `stream.video`, ie,
`stream:get_pixel(x,y)` can be used in place of `stream.video:get_pixel(x,y)`

* `frame.width` - same as `img.width`
* `frame.height` - same as `img.height`
* `frame.channels` - same as `img.channels`
* `frame.state` - all frames are `fixed` images
* `r, g, b, a = frame:get_pixel(x,y)`
  * retrieves the red, green, blue, and alpha values for a given pixel
  * `x,y` starts at `1,1` for the top-left corner of the image
* `frame:set_pixel(x,y,r,g,b,a)` - sets an individual pixel of an image
  * `x,y` starts at `1,1` for the top-left corner of the image
  * `r, g, b` represents the red, green, and blue values, 0 - 255
  * `a` is an optional alpha value, 0 - 255
* `frame:set_pixel_hsl(x,y,r,g,b,a)` - sets a pixel using Hue, Saturation, Lightness
  * `x,y` starts at `1,1` for the top-left corner of the image
  * `h, s, l` represents hue (0-360), saturation (0-100), and lightness (0-100)
  * `a` is an optional alpha value, 0 - 255
* `frame:draw_rectangle(x1,y1,x2,y2,r,g,b,a)` - draws a rectangle from x1,y1 to x2, y2
  * `x,y` starts at `1,1` for the top-left corner of the image
  * `r, g, b` represents the red, green, and blue values, 0 - 255
  * `a` is an optional alpha value, 0 - 255
* `frame:draw_rectangle_hsl(x1,y1,x2,y2,h,s,l,a)` - draws a rectangle from x1,y1 to x2, y2 using hue, saturation, and lightness
  * `x,y` starts at `1,1` for the top-left corner of the image
  * `h, s, l` represents hue (0-360), saturation (0-100), and lightness (0-100)
  * `a` is an optional alpha value, 0 - 255
* `frame:set(frame)`
  * copies a whole frame as-is to the frame
  * the source and destination frame must have the same width, height, and channels values
* `frame:stamp_image(stamp,x,y,flip,mask,a)`
  * stamps a frame (`stamp`) on top of `frame` at `x,y`
  * `x,y` starts at `1,1` for the top-left corner of the image
  * `flip` is an optional table with the following keys:
    * `hflip` - flip `stamp` horizontally
    * `vflip` - flip `stamp` vertically
  * `mask` is an optional table with the following keys:
    * `left` - mask `stamp`'s pixels left
    * `right` - mask `stamp`'s pixels right
    * `top` - mask `stamp`'s pixels top
    * `bottom` - mask `stamp`'s pixels bottom
  * `a` is an optional alpha value
    * if `stamp is an RGBA image, `a` is only applied for `stamp`'s pixels with >0 alpha
* `frame:blend(f,a)`
  * blends `f` onto `frame`, using `a` as the alpha paramter
* `frame:stamp_string(font,str,scale,x,y,r,g,b,max,lmask,rmask)`
  * renders `str` on top of the `frame`, using `font` (a font object)
  * `scale` controls how many pixels to scroll the font, ie, `1` for the default resolution, `2` for double resolution, etc.
  * `x,y` starts at `1,1` for the top-left corner of the image
  * `r, g, b` represents the red, green, and blue values, 0 - 255
  * `max` is the maximum pixel (width) to render the string at. If the would have gone past this pixel, it is truncated
  * `lmask` - mask the string by this many pixels on the left  (after scaling)
  * `rmask` - mask the string by this many pixels on the right (after scaling)
* `frame:stamp_string_hsl(font,str,scale,x,y,h,s,l,max,lmask,rmask)`
  * same as `stamp_string`, but with hue, saturation, and lightness values instead of red, green, and blue
* `frame:stamp_string_adv(str,props,userdata)`
  * renders `str` on top of the `frame`
  * `props` can be a table of per-frame properties, or a function
  * in the case of a table, you need frame 1 defined at a minimum
  * in the case of a function, the function will receive three arguments - the index, and the current properties (may be nil), and the `userdata` value
* `frame:stamp_letter(font,codepoint,scale,x,y,r,g,b,lmask,rmask,tmask,bmask)`
  * renders an individual letter
  * the letter is a UTF-8 codepoint, NOT a character. Ie, 'A' is 65
  * lmask specifies pixels to mask on the left   (after scaling)
  * rmask specifies pixels to mask on the right  (after scaling)
  * tmask specifies pixels to mask on the top    (after scaling)
  * bmask specifies pixels to mask on the bottom (after scaling)
* `frame:stamp_letter(font,codepoint,scale,x,y,h,s,l,lmask,rmask,tmask,bmask)`
  * same as `stamp_letter`, but with hue, saturation, and lightness values instead of red, green, blue

## Font instances

Loaded fonts have the following properties/methods:

* `f:pixel(codepoint,x,y)`
  * returns true if the pixel at `x,y` is active
  * codepoint is UTF-8 codepoint, ie, 'A' is 65
* `f:pixeli(codepoint,x,y)`
  * same as `pixel()`, but inverted
* `f:get_string_width(str,scale)`
  * calculates the width of a rendered string
  * scale needs to be 1 or greater
* `f:utf8_to_table(str)`
  * converts a string to a table of UTF-8 codepoints

## Examples


### example: square

Draw a white square in the top-left corner:

```lua
return function()
  stream.video:draw_rectangle(1,1,200,200,255,255,255)
end
```

### example: stamp image

Load an image and stamp it over the video

```lua
-- register a global "img" to use
-- globals can presist across script reloads

img = img or nil

return {
    onload = function()
      img = image.new('something.jpg')
      img:load(false) -- load immediately
    end,
    onframe = function()
      stream.video:stamp_image(img.frames[1],1,1)
    end
}
```

### example: load a background

```lua
-- register a global 'bg' variable
bg = bg or nil

return {
    onload = function()
      bg = image.new('something.jpg',stream.video.width,stream.video.height,stream.video.channels)
      bg:load(false) -- load immediately
      -- image will be resized to fill the video frame
    end,
    onframe = function()
      stream.video:set(bg)
    end
}
```

### example: display song title

```lua
-- register a global 'f' to use for a font
f = f or nil

return {
    onload = function()
      f = font.new('some-font.bdf')
    end,
    onframe = function()
      if song.title then
          stream.video:stamp_string(f,song.title,3,1,1)
          -- places the song title at top-left (1,1), with a 3x scale
      end
    end
}
```

### example: draw visualizer bars

```lua
return {
    onframe = function()
        -- draws visualizer bars
        -- each bar is 10px wide
        -- bar height is between 0 and 90
        for i=1,stream.audio.spectrum_len,1 do
            stream.video:draw_rectangle((i-1)*20, 680 ,10 + (i-1)*20, 680 - (ceil(stream.audio.amps[i])) , 255, 255, 255)
        end

    end
}
```

### example: animate a gif

```lua
local frametime = 1000 / stream.video.framerate
-- frametime is how long each frame of video lasts in milliseconds
-- we'll use this to figure out when to advance to the next
-- frame of the gif

-- register a global 'gif' variable
gif = gif or nil

return {
    onload = function()
      gif = image.new('agif.gif')
      gif:load(false) -- load immediately

      -- initialize the gif with the first frame and frametime
      gif.frameno = 1
      gif.nextframe = gif.delays[gif.frameno]
    end,
    onframe = function()
      stream.video:stamp_image(gif.frames[gif.frameno],1,1)
      gif.nextframe = gif.nextframe - frametime
      if gif.nextframe <= 0 then
          -- advance to the next frame
          gif.frameno = gif.frameno + 1
          if gif.frameno > gif.framecount then
              gif.frameno = 1
          end
          gif.nextframe = gif.delays[gif.frameno]
      end
    end
}
```

### example: use `stamp_string_adv` with a function to generate a rainbow

```lua
local vga

local colorcounter = 0
local colorprops = {}

local function cycle_color(i, props)
  if i == 1 then
    -- at the beginning of the string, increase our color counter
    colorcounter = colorcounter + 1
    props = {
      x = 1,
    }
  end
  if colorcounter == 36 then
    -- one cycle is 30 degrees
    -- we move 10 degrees per frame, so 36 frames for a full cycle
    colorcounter = 0
  end
  
  -- use the color counter offset + i to change per-letter colors
  local r, g, b = image.hsl_to_rgb((colorcounter + (i-1) ) * 10, 50, 50)

  -- also for fun, we make each letter drop down
  return {
    x = props.x,
    y = 50 + i * (vga.height/2),
    font = vga,
    scale = 3,
    r = r,
    g = g,
    b = b,
  }
end

local function onload()
  vga = font.load('demos/fonts/7x14.bdf')
end

local function onframe()
  stream:stamp_string(vga, "Just some text", 3, 1, 1, 255, 255, 255)
  stream:stamp_string_adv("Some more text", cycle_color )
end

return {
  onload = onload,
  onframe = onframe,
}
```

Output:

![output of rainbow demo](gifs/rainbow-string.gif)

### example: use `stamp_string_adv` with a function to do the wave

```lua
local vga
local sin = math.sin
local ceil = math.ceil

local sincounter = -1
local default_y = 30
local wiggleprops = {}

local function wiggle_letters(i, props)
  if i == 1 then
    sincounter = sincounter + 1
    props = {
      x = 10,
    }
  end
  if sincounter == (26) then
    sincounter = 0
  end

  return {
    x = props.x,
    y = default_y + ceil( sin((sincounter / 4) + i - 1) * 10),
    font = vga,
    scale = 3,
    r = 255,
    g = 255,
    b = 255,
  }
end

local function onload()
  vga = font.load('demos/fonts/7x14.bdf')
end

local function onframe()
  stream:stamp_string_adv("Do the wave", wiggle_letters )
end

return {
  onload = onload,
  onframe = onframe,
}
```

Output:

![output of wave demo](gifs/wiggle-letters.gif)


# License

Unless otherwise stated, all files are released under
an MIT-style license. Details in `LICENSE`

Some exceptions:

* `src/dr_flac.h` - public domain, details found in file
* `src/dr_mp3.h` - public domain, details found in file
* `src/dr_wav.h` - public domain, details found in file
within the file.
* `src/stb_image.h` - public domain, details found in file
* `src/stb_image_resize.h` - public domain, details found in file
* `src/stb_leakcheck.h` - public domain, details found in file
* `src/jpr_proc.h` - public domain, details found in file
* `src/thread.h` - available under an MIT-style license or Public Domain, see file
for details.
* `src/_kiss_fft_guts.h` - BSD-3-Clause, see https://github.com/mborgerding/kissfft
* `src/kiss_fft.c` - BSD-3-Clause, see https://github.com/mborgerding/kissfft
* `src/kiss_fft.h` - BSD-3-Clause, see https://github.com/mborgerding/kissfft
* `src/kiss_fftr.c` - BSD-3-Clause, see https://github.com/mborgerding/kissfft
* `src/kiss_fftr.h` - BSD-3-Clause, see https://github.com/mborgerding/kissfft
* `src/asm.inc` - public domain, details found in file
* `src/attr.h` - public domain, details found in file
* `src/char.c` - public domain, details found in file
* `src/char.h` - public domain, details found in file
* `src/dir.c` - public domain, details found in file
* `src/dir.h` - public domain, details found in file
* `src/file.c` - public domain, details found in file
* `src/file.h` - public domain, details found in file
* `src/fmt.c` - public domain, details found in file
* `src/fmt.h` - public domain, details found in file
* `src/int.h` - public domain, details found in file
* `src/mat.h` - public domain, details found in file
* `src/norm.h` - public domain, details found in file
* `src/pack.c` - public domain, details found in file
* `src/pack.h` - public domain, details found in file
* `src/path.c` - public domain, details found in file
* `src/path.h` - public domain, details found in file
* `src/scan.c` - public domain, details found in file
* `src/scan.h` - public domain, details found in file
* `src/str.c` - public domain, details found in file
* `src/str.h` - public domain, details found in file
* `src/text.c` - public domain, details found in file
* `src/text.h` - public domain, details found in file
* `src/unpack.c` - public domain, details found in file
* `src/unpack.h` - public domain, details found in file
* `src/utf.c` - public domain, details found in file
* `src/utf.h` - public domain, details found in file
* `src/util.h` - public domain, details found in file
* `src/HandmadeMath.h` - public domain, see https://github.com/HandmadeMath/Handmade-Math/blob/master/LICENSE

# Known users

* [Game That Tune's 24/7 VGM Stream](https://youtube.com/gamethattune)
