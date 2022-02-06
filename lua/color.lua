local ceil = math.ceil
local abs = math.abs

local function hsl_to_rgb(h,s,l)
  -- accepts 0 <= h < 360
  -- accepts 0 <= s <= 100
  -- accepts 0 <= l <= 100

  if s < 0 then s = 0 end
  if s > 100 then s = 100 end
  if l < 0 then l = 0 end
  if l > 100 then l = 100 end

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

return {
  hsl_to_rgb = hsl_to_rgb,
}
