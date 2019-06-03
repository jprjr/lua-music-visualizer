local band, bor, rshift, lshift, tobit, bnot

local ok, bit = pcall(require,'bit')
if ok then
  band, bor, rshift, lshift, tobit, bnot = bit.band, bit.bor, bit.rshift, bit.lshift, bit.tobit, bit.bnot
else
  ok, bit = pcall(require,'bit32')
  if ok then
    band, bor, rshift, lshift, bnot = bit.band, bit.bor, bit.rshift, bit.lshift, bit.bnot
    tobit = function(n)
      n = tonumber(n)
      return n <= 0x7fffffff and n or -(bnot(n) + 1)
    end
  else
    error('Unable to find bit library')
  end
end

local lines = io.lines
local find = string.find
local tonumber = tonumber
local setmetatable = setmetatable
local type = type
local insert = table.insert
local byte = string.byte

local _M = {
  _VERSION = '1.0.0'
}

-- from http://lua-users.org/wiki/LuaUnicode

local function Utf8to32(utf8str)
	assert(type(utf8str) == "string")
	local res, seq, val = {}, 0, nil
	for i = 1, #utf8str do
		local c = byte(utf8str, i)
		if seq == 0 then
			insert(res, val)
			seq = c < 0x80 and 1 or c < 0xE0 and 2 or c < 0xF0 and 3 or
			      c < 0xF8 and 4 or --c < 0xFC and 5 or c < 0xFE and 6 or
				  error("invalid UTF-8 character sequence")
			val = band(c, 2^(8-seq) - 1)
		else
			val = bor(lshift(val, 6), band(c, 0x3F))
		end
		seq = seq - 1
	end
	insert(res, val)
	return res
end

local font_mt = {}
local glyph_mt = {}

function font_mt:bitmap(index)
  local t = type(index)
  if t == 'number' then
    return self.bitmaps[index]
  elseif t == 'string' then
    local r = Utf8to32(index)
    local res = {}
    for i,v in ipairs(r) do
      if self.bitmaps[index] then
        insert(res,self.bitmaps[index])
      else
        insert(res,0)
      end
    end
    return res
  end
  return nil, 'bitmap should be called with a codepoint or string'
end

function font_mt:pixel(index,x,y)
  if not self.bitmaps[index] then return nil end
  local w = self.widths[index]
  if w < 9 then
    w = 8
  else
    w = w + (8 - (w % 8))
  end

  return
    band(
      self.bitmaps[index][y],
      rshift(
        lshift(1,w),
        x
      )
    ) > 0
end

function font_mt:pixeli(index,x,y)
  return self:pixel(index,x,self.height - (y - 1))
end

function font_mt:get_string_width(str,scale)
  local codepoints = Utf8to32(str)
  local w = 0
  for _,c in ipairs(codepoints) do
    if self.widths[c] then
      w = w + (self.widths[c] * scale)
    else
      w = w + (self.width * scale)
    end
  end
  return w
end

function font_mt.utf8_to_table(str)
  return Utf8to32(str)
end

font_mt.__index = font_mt

local function load_bdf(filename)
    if not filename then return nil end

    local font = {
      width = nil,
      height = nil,
      widths = {},
      bitmaps = {},
    }
    local width, height, bbx, bby, _
    local cur_index = 0
    local cur_width,cur_height,cur_bbx,cur_bby
    local dwidth
    local reading_bitmap = false
    local bitmap_line = 0
    for line in lines(filename) do
      if(find(line,"^FONTBOUNDINGBOX")) then
        _,_,width,height,bbx,bby = find(line,"^FONTBOUNDINGBOX%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)")
        width = tonumber(width)
        height = tonumber(height)
        bbx = tonumber(bbx)
        bby = tonumber(bby)
        font.width = width
        font.height = height
      elseif(find(line,'^ENCODING')) then
        _,_,cur_index = find(line,'^ENCODING%s+([^%s]+)')
        cur_index = tonumber(cur_index)
      elseif(find(line,'^DWIDTH')) then
        _,_,dwidth = find(line,'^DWIDTH%s+([^%s]+)')
        dwidth = tonumber(dwidth)
      elseif(find(line,'^BBX')) then
        _,_,cur_width,cur_height,cur_bbx,cur_bby = find(line,"^BBX%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)")
        cur_width = tonumber(cur_width)
        cur_height = tonumber(cur_height)
        cur_bbx = tonumber(cur_bbx)
        cur_bby = tonumber(cur_bby)

      elseif(find(line,'^BITMAP')) then
        reading_bitmap = true
        font.bitmaps[cur_index] = {}
        for i=1,height,1 do
          font.bitmaps[cur_index][i] = 0
        end

        if cur_bbx < 0 then
          dwidth = dwidth - cur_bbx
          cur_bbx = 0
        end

        if cur_bbx + cur_width > dwidth then
          dwidth = cur_bbx + cur_width
        end
        font.widths[cur_index] = dwidth
        bitmap_line = 1
      elseif(find(line,'^ENDCHAR')) then
        reading_bitmap = false
        bitmap_line = 0
      elseif(reading_bitmap == true) then
        local rel_line = height - (cur_height - (bitmap_line - 1) - bby + cur_bby - 1)
        font.bitmaps[cur_index][rel_line] = tobit('0x' .. line)
        bitmap_line = bitmap_line + 1
      end

    end

    setmetatable(font,font_mt)

    return font
end

_M.new = load_bdf
_M.load = load_bdf
_M.utf8_to_table = Utf8to32


return _M
