-- this is loaded after the core C methods
-- are loaded
--

local reg = debug.getregistry()

local image_mt = reg["image"]
local image_mt_funcs = image_mt["__index"]

local ceil = math.ceil
local abs = math.abs

local function hsl_to_rgb(h,s,l)
  -- accepts 0 <= h < 360
  -- accepts 0 <= s <= 100
  -- accepts 0 <= l <= 100

  if l == 100 then
    return 255, 255, 255
  end

  if l == 0 then
    return 0, 0 ,0
  end

  if s == 0 then
    l = ceil(255 * (l / 100) )
    return l, l, l
  end

  while(h >= 360) do
    h = h - 360
  end

  l = l / 100
  s = s / 100

  local c = (1 - abs(2*l - 1)) * s
  local hp = h / 60
  local x = c * (1 - abs((hp % 2) - 1))
  local m = l - (c / 2)
  local r1 = 0
  local g1 = 0
  local b1 = 0

  if hp <= 1 then
    r1 = c
    g1 = x
  elseif hp <= 2 then
    r1 = x
    g1 = c
  elseif hp <= 3 then
    g1 = c
    b1 = x
  elseif hp <= 4 then
    g1 = x
    b1 = c
  elseif hp <= 5 then
    r1 = x
    b1 = c
  elseif hp <= 6 then
    r1 = c
    b1 = x
  end

  return ceil((r1 + m) * 255), ceil((g1 + m) * 255), ceil((b1 + m) * 255)
end

image.hsl_to_rgb = hsl_to_rgb

image_mt_funcs.set_pixel_hsl = function(self,x,y,h,s,l,a)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:set_pixel(x,y,r,g,b,a)
end

image_mt_funcs.draw_rectangle_hsl = function(self,x1,y1,x2,y2,h,s,l,a)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:draw_rectangle(x1,y1,x2,y2,r,g,b,a)
end

image_mt_funcs.draw_rectangle_rgb = image_mt_funcs.draw_rectangle

image_mt_funcs.stamp_letter = function(self,font,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset)

  if not font.widths[codepoint] then
    codepoint = 32
  end

  hloffset = hloffset or 0 -- "masks" the first pixels of the left of the string
  hroffset = hroffset or 0 -- "masks" the end pixels of the right of the string
  ytoffset = ytoffset or 0
  yboffset = yboffset or 0
  local pixel_hroffset = font.widths[codepoint] * scale - hroffset - 1
  local pixel_yboffset = font.height * scale - yboffset - 1
  local xcc
  local ycc

  for xc=1,font.widths[codepoint],1 do
    for yc=font.height,1,-1 do
      if font:pixel(codepoint,xc,yc) then
        for xi=0,scale-1,1 do
          for yi=0,scale-1,1 do
            xcc = (xc - 1) * scale + xi
            ycc = (yc - 1) * scale + yi
            if xcc >= hloffset and xcc <= pixel_hroffset and
               ycc >= ytoffset and ycc <= pixel_yboffset then
              self:set_pixel(x+xcc,y+((yc - 1) * scale)+yi,r,g,b)
            end
          end
        end
      end
    end
  end

  return font.widths[codepoint] * scale
end

local string_props = {
  'font',
  'scale',
  'x',
  'y',
  'r',
  'g',
  'b',
  'max',
  'lmask',
  'rmask',
  'tmask',
  'bmask',
}

image_mt_funcs.stamp_string_adv = function(self, str, props, userd)
  local p
  local tp = type(props)

  local codepoints = font.utf8_to_table(str)
  for i,codepoint in ipairs(codepoints) do
    local ap
    if tp == 'table' then
      ap = props[i]
    elseif tp == 'function' then
      ap = props(i,p,userd)
    else
      return false, nil
    end
    if not p then
      if not ap then
        return false, nil
      end
      p = {}
    end

    if ap then
      for _,k in ipairs(string_props) do
        if ap[k] then
          p[k] = ap[k]
        end
      end
    end

    if not p.font.widths[codepoint] then
      codepoint = 32
    end
    p.x = p.x + self:stamp_letter(p.font,codepoint,p.scale,p.x,p.y,p.r,p.g,p.b,p.lmask,p.rmask,p.tmask,p.bmask)
  end

end

image_mt_funcs.stamp_string = function(self,font,str,scale,x,y,r,g,b,max,lmask,rmask,tmask,bmask)
  local lmask_applied = false
  if not lmask then
    lmask_applied = true
  end

  rmask = rmask or 0
  tmask = tmask or 0
  bmask = bmask or 0

  tmask = tmask * scale
  bmask = bmask * scale

  local codepoints = font.utf8_to_table(str)

  local xi = x
  for _,codepoint in ipairs(codepoints) do
    if not font.widths[codepoint] then
      codepoint = 32
    end
    if lmask and lmask >= (font.widths[codepoint] * scale) then
      lmask = lmask - (font.widths[codepoint] * scale)
      xi = xi + (font.widths[codepoint] * scale)
    else
      if max and  xi + (font.widths[codepoint] * scale) > max then
        rmask =  (font.widths[codepoint] * scale) - (max - xi)
      end
      xi = xi + self:stamp_letter(font,codepoint,scale,xi,y,r,g,b,
        lmask_applied == false and lmask or 0,
        rmask,tmask,bmask)
      if lmask_applied == false then
        lmask_applied = true
        lmask = 0
      end
    end
  end
end

image_mt_funcs.stamp_string_hsl = function(self,font,str,scale,x,y,h,s,l,max,lmask,rmask,tmask,bmask)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:stamp_string(font,str,scale,x,y,r,g,b,max,lmask,rmask,tmask,bmask)
end

image_mt_funcs.stamp_letter_hsl = function(self,font,codepoint,scale,x,y,h,s,l,hloffset,hroffset,ytoffset,yboffset)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:stamp_letter(font,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset)
end

-- try over-riding c methods with ffi versions

local ok, ffi = pcall(require,'ffi')
if not ok then return end
local bit = require'bit'

local image_c_mt = reg["image_c"]
local image_c_funcs = image_c_mt["__index"]

local lshift = bit.lshift
local rshift = bit.rshift

ffi.cdef[[
enum IMAGE_STATE {
    IMAGE_ERR,
    IMAGE_UNLOADED,
    IMAGE_LOADING,
    IMAGE_LOADED,
    IMAGE_FIXED,
};

void
free (void *ptr);

void *
memcpy(void *dst, void *src, size_t n);

typedef int (*image_probe)
(const char *filename, unsigned int *width, unsigned int *height, unsigned int *channels);

typedef uint8_t *(*image_load)
(const char *filename, unsigned int *width, unsigned int *height, unsigned int *channels, unsigned int *frames);

typedef void (*queue_image_load)
(intptr_t table_ref,const char* filename, unsigned int width, unsigned int height, unsigned int channels);

typedef void (*image_blend)
(uint8_t *dst, uint8_t *src, unsigned int len, uint8_t a);

typedef void (*video_set_image_cb)
(void *,void (*)(void *, intptr_t, unsigned int, uint8_t *));

]]

local args = {...}

local image_probe = ffi.cast('image_probe',args[1])
local image_load = ffi.cast('image_load',args[2])
local queue_image_load = ffi.cast('queue_image_load',args[3])
local image_blend = ffi.cast('image_blend',args[4])
local video_set_image_cb = ffi.cast('video_set_image_cb',args[5])

local function load_image_mem_chunk(t_image,frames,img)
  local x = t_image.width
  local y = t_image.height
  local c = t_image.channels

  t_image.frames = {}
  t_image.delays = {}

  for i=0,frames-1,1 do
    local chunk = img + (i * (x * y * c)) + (i * 2)
    local frame = ffi.new("uint8_t[?]",x * y * c)
    ffi.C.memcpy(frame,chunk,x*y*c)

    if frames > 1 then
      chunk = chunk + (x * y * c)
      local delay_one = ffi.new("uint8_t[1]")
      local delay_two = ffi.new("uint8_t[1]")
      ffi.C.memcpy(delay_one,chunk,1)
      chunk = chunk + 1
      ffi.C.memcpy(delay_two,chunk,1)
      t_image.delays[i+1] = (delay_one[0] + lshift(delay_two[0],8))
    else
      t_image.delays[i+1] = 0
    end

    t_image.frames[i+1] = {
      image = frame,
      width = x,
      height = y,
      channels = c,
      image_len = x * y * c,
      image_state = ffi.C.IMAGE_FIXED,
      state = "fixed",
    }
    setmetatable(t_image.frames[i+1],image_mt)
  end

  t_image.image_state = ffi.C.IMAGE_LOADED
  t_image.state = 'loaded'
  t_image.framecount = frames

  return
end

local function image_cb(lua,table_ref,frames,img)
  local t_image = image.from_ref(tonumber(table_ref))

  if img == ffi.NULL then
    t_image.frames = nil
    t_image.delays = nil
    t_image.image_state = ffi.C.IMAGE_ERR
    t_image.state = 'error'
    return
  end

  load_image_mem_chunk(t_image,frames,img)

  ffi.C.free(img)
  return
end

image_mt_funcs.draw_rectangle = function(self,x1,y1,x2,y2,r,g,b,a)
  local xstart, xend
  local ystart, yend

  a = a or 255

  if(r < 0 or b < 0 or g < 0 or a < 0 or
     r > 255 or b > 255 or g > 255 or a > 255) then
     return false
  end

  if a == 0 then
    return true
  end

  if x1 < 1 then
    x1 = 1
  end

  if x2 < 1 then
    x2 = 1
  end

  if y1 < 1 then
    y1 = 1
  end

  if y2 < 1 then
    y2 = 1
  end

  if x1 <= x2 then
    xstart = x1
    xend = x2
  else
    xstart = x2
    xend = x1
  end

  if y1 <= y2 then
    ystart = y2
    yend = y1
  else
    ystart = y1
    yend = y2
  end

  if xend > self.width then
    xend = self.width
  end

  if yend > self.height then
    yend = self.height
  end

  xstart = xstart - 1
  xend = xend - 1
  ystart = self.height - ystart
  yend = self.height - yend

  local alpha = 1 + a
  local alpha_inv = 256 - a

  for x=xstart,xend,1 do
    for y=ystart,yend,1 do
      local index = (y * self.width * self.channels) + (x * self.channels)

      if a == 255 then
        self.image[index] = b
        self.image[index + 1] = g
        self.image[index + 2] = r
      end

      self.image[index]   = rshift( ((self.image[index] * alpha_inv) + (b * alpha)), 8)
      self.image[index+1] = rshift( ((self.image[index+1] * alpha_inv) + (g * alpha)), 8)
      self.image[index+2] = rshift( ((self.image[index+2] * alpha_inv) + (r * alpha)), 8)
    end
  end

  return true
end

image.new = function(filename,width,height,channels)
  if not filename and (not width or not height or not channels) then
    return nil,"Need either filename, or width/height/channels"
  end
  width = width or 0
  height = height or 0
  channels = channels or 0

  width    = ffi.new("unsigned int[1]",width)
  height   = ffi.new("unsigned int[1]",height)
  channels = ffi.new("unsigned int[1]",channels);

  if filename then
    if image_probe(filename,width,height,channels) == 0 then
      return nil,"Unable to probe image"
    end
  end

  local img = {
    width    = tonumber(width[0]),
    height   = tonumber(height[0]),
    channels = tonumber(channels[0]),
    filename = filename,
  }
  if filename then
    img.image_state = ffi.C.IMAGE_UNLOADED
    img.state = "unloaded"
  else
    img.frames = {}
    img.frames[1] = {
        image_state = ffi.C.IMAGE_FIXED,
        image_len = img.width * img.height * img.channels,
        state = "fixed",
        image = ffi.new("uint8_t[?]",img.width * img.height * img.channels),
        width = img.width,
        height = img.height,
        channels = img.channels,
    }
    setmetatable(img.frames[1],image_mt)
  end

  setmetatable(img,image_c_mt)

  return img
end

image_mt_funcs.blend = function(self,b,a)
  if self.image_len ~= b.image_len then
    return
  end
  image_blend(self.image,b.image,self.image_len,a)
end

image_mt_funcs.set_pixel = function(self,x,y,r,g,b,a)
  if not x or not y or not r or not g or not b then return false end

  if(x < 1 or y < 1 or x > self.width or y > self.width) then
    return false
  end
  a = a or 255

  if a == 0 then
    return true
  end

  if(r < 0 or b < 0 or g < 0 or a < 0 or
     r > 255 or b > 255 or g > 255 or a > 255) then
     return false
  end

  x = x - 1
  y = self.height - y

  local index = (y * self.width * self.channels) + (x * self.channels)

  if a == 255 then
    self.image[index] = b
    self.image[index + 1] = g
    self.image[index + 2] = r
    return true
  end

  local alpha = 1 + a
  local alpha_inv = 256 - a

  self.image[index]   = rshift( ((self.image[index] * alpha_inv) + (b * alpha)), 8)
  self.image[index+1] = rshift( ((self.image[index+1] * alpha_inv) + (g * alpha)), 8)
  self.image[index+2] = rshift( ((self.image[index+2] * alpha_inv) + (r * alpha)), 8)

  return true
end

image_mt_funcs.get_pixel = function(self,x,y)
  if x < 1 or y < 1 or x > self.width or y > self.height then
    return nil
  end
  local r, g, b, a
  x = x - 1
  y = self.height - y
  index = (y * self.width * self.channels) + (x * self.channels)

  b = self.image[index]
  g = self.image[index+1]
  r = self.image[index+2]

  if self.channels == 4 then
    a = self.image[index+3]
  else
    a = 255
  end

  return r, g, b, a
end

image_mt_funcs.stamp_image = function(self,img,x,y,flip,mask,alpha)
  x = x or 1
  y = y or 1
  flip = flip or {}
  mask = mask or {}

  local xi = 1
  local yi = 1

  local xm = img.width
  local ym = img.height

  local xt, yt, dxt, dyt, aa, aa_inv

  if x < 1 then
    xi = xi + abs(x) + 1
  end
  if y < 1 then
    yi = yi + abs(y) + 1
  end


  if mask.left then
    xi = xi + mask.left
  end
  if mask.right then
    xm = xm - mask.right
  end
  if mask.top then
    yi = yi + mask.top
  end
  if mask.bottom then
    ym = ym - mask.bottom
  end

  for yii=yi,ym,1 do
    for xii=xi,xm, 1 do
      xt, yt = xii, yii

      if flip.hflip then
        xt = img.width - xii + 1
      end

      if flip.vflip then
        yt = img.height - yii + 1
      end

      dxt = x - 1 + xii
      dyt = y - 1 + yii

      if dxt <= self.width and dyt <= self.height then
        xt = xt - 1
        yt = img.height - yt

        dxt = dxt - 1
        dyt = self.height - dyt

        local byte = (yt * img.width * img.channels) + (xt * img.channels)
        local r, g, b, a

        if img.channels == 4 then
          a = img.image[byte + 3]
        else
          a = 255
        end

        if a > 0 then

          if alpha then
            a = alpha
          end

          b = img.image[byte]
          g = img.image[byte + 1]
          r = img.image[byte + 2]

          byte = (dyt * self.width * self.channels) + (dxt * self.channels)

          if a == 255 then
            self.image[byte] = b
            self.image[byte+1] = g
            self.image[byte+2] = r
          else
            aa = 1 + a
            aa_inv = 256 - a
            self.image[byte]   = rshift( ((self.image[byte] * aa_inv) + (b * aa)), 8)
            self.image[byte+1] = rshift( ((self.image[byte+1] * aa_inv) + (g * aa)), 8)
            self.image[byte+2] = rshift( ((self.image[byte+2] * aa_inv) + (r * aa)), 8)
          end
        end
      end
    end
  end
end


image_c_funcs.load = function(self,async)
  if self.state ~= "unloaded" then return false end

  self.state = 'loading'
  self.image_state = ffi.C.IMAGE_LOADING

  if not async then
    local frames     = ffi.new("unsigned int[1]",0)
    local width    = ffi.new("unsigned int[1]",self.width)
    local height   = ffi.new("unsigned int[1]",self.height)
    local channels = ffi.new("unsigned int[1]",self.channels);

    local image = image_load(self.filename,width,height,channels,frames)

    if image == ffi.NULL then
      self.frames = nil
      self.delays = nil
      self.image_state = ffi.C.IMAGE_ERR
      self.state = 'error'
      return false
    end

    load_image_mem_chunk(self,tonumber(frames[0]),image)

    ffi.C.free(image)

    return true
  end

  queue_image_load(self:get_ref(),self.filename,self.width,self.height,self.channels)
  return true

end

image_c_funcs.unload = function(self)
  if self.state ~= "loaded" then
    return false
  end
  self.frames = nil
  self.delays = nil
  self.framecount = nil
  self.state = "unloaded"
  self.image_state = ffi.C.IMAGE_UNLOADED
  return true
end

image_mt_funcs.set = function(self,a)
  if self.image_len ~= a.image_len then return end
  ffi.C.memcpy(self.image,a.image,self.image_len)
end

video_set_image_cb(args[6],image_cb)
