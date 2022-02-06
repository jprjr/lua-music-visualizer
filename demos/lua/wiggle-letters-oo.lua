-- luacheck: globals stream
local bdf = require'lmv.bdf'
local sin = math.sin
local ceil = math.ceil

local OODemo = {}
local OODemo__metatable = {
  __index = OODemo
}

-- returns a closure suitable for stream:stamp_string_adv
function OODemo:new_wiggle_letters()
  return function(i, props)
    if i == 1 then
      self.sincounter = self.sincounter + 1
      props = {
        x = 10,
      }
    end
    if self.sincounter == (26) then
      self.sincounter = 0
    end

    return {
      x = props.x,
      y = self.default_y + ceil( sin((self.sincounter / 4) + i - 1) * 10),
      font = self.vga,
      scale = 3,
      r = 255,
      g = 255,
      b = 255,
    }
  end
end

-- initialize default values, load a font, generate the closure
function OODemo:onload()
  self.framecounter = 0
  self.sincounter = -1
  self.default_y = 30
  self.vga = bdf.load('demos/fonts/7x14.bdf')
  self.wiggle_letters = self:new_wiggle_letters()
end

function OODemo:onframe()
  stream:stamp_string_adv("Do the wave", self.wiggle_letters )
  self.framecounter = self.framecounter + 1
  if self.framecounter == 300 then
    os.exit(0)
  end
end

return setmetatable({},OODemo__metatable)
