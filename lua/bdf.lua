local bdf_mt, bdf, bdf_load, bdf_free = ...

local utf8_to_table = bdf.utf8_to_table

bdf_mt.utf8_to_table = utf8_to_table

function bdf_mt:pixeli(index,x,y)
  return self:pixel(index,x,self.height - (y - 1))
end

function bdf_mt:bitmap(index)
  local t = type(index)
  if t == 'number' then
    return self.glyphs[index]
  elseif t == 'string' then
    local r = utf8_to_table(index)
    local res = {}
    for i=1,#r,1 do
      res[i] = self.glyphs[r[i]]
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
