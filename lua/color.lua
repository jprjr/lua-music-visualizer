local color = ...

if not jit or jit.status() == false then
  -- we're in regular lua, or jit is off, so
  -- stick with C version of the module
  return
end

local floor = math.floor
local min = math.min
local abs = math.abs

local function round(x)
  return floor(x + (2^52 + 2^51) - (2^52 + 2^51))
end

local function findmax(r,g,b)
  local m = -1.0
  local which = 0

  if r > m then
    m = r
    which = 1
  end

  if g > m then
    m = g
    which = 2
  end

  if b > m then
    m = b
    which = 3
  end

  return m, which
end

local function findmin(r, g, b)
  local m = 2.0
  if r < m then
    m = r
  end
  if g < m then
    m = g
  end
  if b < m then
    m = b
  end

  return m
end

local function rgb_to_hsl(r,g,b)
  if r < 0 then r = 0.0 end
  if r > 255 then r = 255.0 end
  if g < 0 then g = 0.0 end
  if g > 255 then g = 255.0 end
  if b < 0 then b = 0.0 end
  if b > 255.0 then b = 255.0 end

  local rp = r / 255.0
  local gp = g / 255.0
  local bp = b / 255.0

  local cmax, which = findmax(rp, gp, bp)
  local cmin = findmin(rp, gp, bp)
  local delta = cmax - cmin

  local h = 0.0
  local s = 0.0
  local l = (cmax + cmin) / 2.0

  if delta > 0.0 then
    s = delta / ( 1.0 - abs((2.0*l) - 1.0) )
    if which == 1 then
      h = 60.0 * (((gp - bp) / delta) % 6)
    elseif which == 2 then
      h = 60.0 * (((bp - rp) /delta) + 2.0)
    elseif which == 3 then
      h = 60.0 * (((rp - gp) /delta) + 4.0)
    end
  end

  return round(h), round(s * 100.0), round(l * 100.0)
end


local function rgb_to_hsv(r,g,b)
  if r < 0 then r = 0.0 end
  if r > 255 then r = 255.0 end
  if g < 0 then g = 0.0 end
  if g > 255 then g = 255.0 end
  if b < 0 then b = 0.0 end
  if b > 255 then b = 255.0 end

  local rp = r / 255.0
  local gp = g / 255.0
  local bp = b / 255.0

  local cmax, which = findmax(rp, gp, bp)
  local cmin = findmin(rp, gp, bp)
  local delta = cmax - cmin

  local h = 0.0
  local s = 0.0
  local v = cmax

  if delta > 0.0 then
    if which == 1 then
      h = 60.0 * (((gp - bp) / delta) % 6)
    elseif which == 2 then
      h = 60.0 * (((bp - rp) /delta) + 2.0)
    elseif which == 3 then
      h = 60.0 * (((rp - gp) /delta) + 4.0)
    end
  end

  if cmax ~= 0.0 then
    s = delta / cmax
  end

  return round(h), round(s * 100.0), round(v * 100.0)
end

local function hsl_to_rgb(h,s,l)
  -- accepts 0 <= h < 360
  -- accepts 0 <= s <= 100.0
  -- accepts 0 <= l <= 100.0

  if s <= 0 then s = 0.0 end
  if s >= 100 then s = 100.0 end
  if l <= 0 then l = 0.0 end
  if l >= 100 then l = 100.0 end

  if l == 100.0 then
    return 255.0, 255.0, 255.0
  end

  if l == 0.0 then
    return 0, 0 ,0
  end

  if s == 0.0 then
    l = round(255.0 * (l / 100.0) )
    return l, l, l
  end

  while(h >= 360) do
    h = h - 360
  end

  s = s / 100.0
  l = l / 100.0

  local c = (1.0 - abs(2.0*l - 1.0)) * s
  local hp = h / 60.0
  local x = c * (1.0 - abs((hp % 2.0) - 1.0))
  local m = l - (c / 2.0)
  local r1 = 0.0
  local g1 = 0.0
  local b1 = 0.0

  if hp < 1 then
    r1 = c
    g1 = x
  elseif hp < 2 then
    r1 = x
    g1 = c
  elseif hp < 3 then
    g1 = c
    b1 = x
  elseif hp < 4 then
    g1 = x
    b1 = c
  elseif hp < 5 then
    r1 = x
    b1 = c
  elseif hp < 6 then
    r1 = c
    b1 = x
  end

  return round((r1 + m) * 255.0), round((g1 + m) * 255.0), round((b1 + m) * 255.0)
end

local function hsv_to_rgb(h,s,v)
  -- accepts 0 <= h < 360
  -- accepts 0 <= s <= 100.0
  -- accepts 0 <= v <= 100.0

  if s <= 0 then s = 0.0 end
  if s >= 100 then s = 100.0 end
  if v <= 0 then v = 0.0 end
  if v >= 100 then v = 100.0 end

  while(h >= 360) do
    h = h - 360
  end

  s = s / 100.0
  v = v / 100.0

  local c = v * s
  local hp = h / 60.0
  local x = c * (1.0 - abs((hp % 2.0) - 1.0))
  local m = v - c
  local r1 = 0.0
  local g1 = 0.0
  local b1 = 0.0

  if hp < 1 then
    r1 = c
    g1 = x
  elseif hp < 2 then
    r1 = x
    g1 = c
  elseif hp < 3 then
    g1 = c
    b1 = x
  elseif hp < 4 then
    g1 = x
    b1 = c
  elseif hp < 5 then
    r1 = x
    b1 = c
  elseif hp < 6 then
    r1 = c
    b1 = x
  end

  return round((r1 + m) * 255.0), round((g1 + m) * 255.0), round((b1 + m) * 255.0)
end

local function hsl_to_hsv(h, s, l)
  if s <= 0 then s = 0.0 end
  if s >= 100 then s = 100.0 end
  if l <= 0 then l = 0.0 end
  if l >= 100 then l = 100.0 end

  while(h >= 360) do
    h = h - 360
  end

  s = s / 100.0
  l = l / 100.0

  local v = l + (s * min(l,1.0-l))
  local sv = 0.0
  if v ~= 0.0 then
    sv = 2.0 * (1.0 - (l/v))
  end

  return round(h), round(sv*100.0), round(v*100.0)
end

local function hsv_to_hsl(h, s, v)
  if s <= 0 then s = 0.0 end
  if s >= 100 then s = 100.0 end
  if v <= 0 then v = 0.0 end
  if v >= 100 then v = 100.0 end

  while(h >= 360) do
    h = h - 360
  end

  s = s / 100.0
  v = v / 100.0

  local l = v * ( 1.0 - (s / 2.0) )
  local sl = 0.0
  if l > 0.0 and l < 1.0 then
    sl = (v - l) / (min(l,1.0 - l))
  end

  return round(h), round(sl*100.0), round(l*100.0)
end

color.hsl_to_rgb = hsl_to_rgb
color.hsv_to_rgb = hsv_to_rgb
color.rgb_to_hsl = rgb_to_hsl
color.rgb_to_hsv = rgb_to_hsv
color.hsl_to_hsv = hsl_to_hsv
color.hsv_to_hsl = hsv_to_hsl
