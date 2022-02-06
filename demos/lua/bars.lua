-- luacheck: globals stream
local bar_height = 100
local bar_width = 10
local bar_offset = 5 -- start at this x offset in the image
local bar_spacing = 5 -- number of pixels between bars

local audio = require'lmv.audio'
local analyzer = audio.analyzer(20)

local framecounter = 0

return {
    onframe = function()
        analyzer:update()
        for i=1,#analyzer.amps,1 do
            stream.video:draw_rectangle(
              bar_offset + (i-1)*(bar_width + bar_spacing),
              110 - (math.ceil(analyzer.amps[i] * bar_height)),
              bar_offset + (i-1)*(bar_width + bar_spacing) + bar_width,
              110,
              255, 255, 255)
        end

        framecounter = framecounter + 1
        if framecounter == 300 then
          os.exit(0)
        end

    end
}

