-- this is loaded after the core C methods
-- are loaded

local band, rshift, lshift, bnot
local ok, bit = pcall(require,'bit')
if ok then
  band, rshift, lshift, bnot = bit.band, bit.rshift, bit.lshift, bit.bnot
else
  ok, bit = pcall(require,'bit32')
  if ok then
    band, rshift, lshift, bnot = bit.band, bit.rshift, bit.lshift, bit.bnot
  end
end

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

