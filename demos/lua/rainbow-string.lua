local vga

local framecounter = 0
local colorcounter = 0
local colorprops = {}

local function cycle_color(i, props)
  if i == 1 then
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
  local r, g, b = image.hsl_to_rgb((colorcounter + (i-1) ) * 10, 50, 50)
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
  framecounter = framecounter + 1
  if framecounter == 300 then
    os.exit(0)
  end
end

return {
  onload = onload,
  onframe = onframe,
}
