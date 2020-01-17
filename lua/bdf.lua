local bdf_mt, bdf, bdf_load, bdf_free, utf_dec_utf8 = ...
local band, rshift, lshift, bnot
local len = string.len
local insert = table.insert

local ok, bit = pcall(require,'bit')
if ok then
  band, rshift, lshift, bnot = bit.band, bit.rshift, bit.lshift, bit.bnot
else
  ok, bit = pcall(require,'bit32')
  if ok then
    band, rshift, lshift, bnot = bit.band, bit.rshift, bit.lshift, bit.bnot
  end
end


local utf8_to_table = bdf.utf8_to_table
local dec_utf8

local ok, ffi = pcall(require,'ffi')
if ok then
  ffi.cdef[[
typedef uint8_t (*utf_decoder)(uint32_t *cp, const uint8_t *str)
]]
  dec_utf8 = ffi.cast("utf_decoder",utf_dec_utf8)
  utf8_to_table = function(str)
    local ret = {}
    local n = len(str)
    local t
    local cp = ffi.new("uint32_t[1]")
    local str_ptr = ffi.cast('uint8_t *',str)
    while(n > 0) do
      t = dec_utf8(cp,str_ptr)
      insert(ret,cp[0])
      str_ptr = str_ptr + t
      n = n - t
    end
    return ret
  end
  bdf.utf8_to_table = utf8_to_table
end


bdf_mt.utf8_to_table = utf8_to_table

local function pixel(self,index,x,y)
  if not self.bitmaps[index] then return nil end
  local w = band(self.widths[index] + 7,bnot(7))

  local r =  band(
      self.bitmaps[index][y],
      rshift(
        lshift(1,w),
        x
      )
    ) > 0
  return r
end

if band ~= nil then
  bdf_mt.pixel = pixel
end

function bdf_mt:pixeli(index,x,y)
  return self:pixel(index,x,self.height - (y - 1))
end

function bdf_mt:bitmap(index)
  local t = type(index)
  if t == 'number' then
    return { self.bitmaps[index] }
  elseif t == 'string' then
    local r = utf8_to_table(index)
    local res = {}
    for i=1,#r,1 do
      res[i] = self.bitmaps[r[i]]
    end
    return res
  end
  return nil, 'bitmap should be called with a codepoint or string'
end

function bdf_mt:get_string_width(str,scale)
  local codepoints = utf8_to_table(str)
  local w = 0
  for i=1,#codepoints,1 do
    if self.widths[codepoints[i]] then
      w = w + (self.widths[codepoints[i]] * scale)
    else
      w = w + (self.width * scale)
    end
  end
  return w
end

return bdf
