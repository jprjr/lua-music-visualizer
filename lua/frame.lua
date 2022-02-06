local frame_mt_funcs = ...

local bdf = require'lmv.bdf'
local color = require'lmv.color'

local hsl_to_rgb = color.hsl_to_rgb

frame_mt_funcs.set_pixel_hsl = function(self,x,y,h,s,l,a)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:set_pixel(x,y,r,g,b,a)
end

frame_mt_funcs.draw_line_hsl = function(self,x1,y1,x2,y2,h,s,l,a)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:draw_line(x1,y1,x2,y2,r,g,b,a)
end

frame_mt_funcs.draw_rectangle_hsl = function(self,x1,y1,x2,y2,h,s,l,a)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:draw_rectangle(x1,y1,x2,y2,r,g,b,a)
end

frame_mt_funcs.draw_line_rgb = frame_mt_funcs.draw_line
frame_mt_funcs.draw_rectangle_rgb = frame_mt_funcs.draw_rectangle

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
  'hflip',
  'vflip'
}

frame_mt_funcs.stamp_string_adv = function(self, str, props, userd)
  local p
  local tp = type(props)

  local codepoints = bdf.utf8_to_table(str)
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
    p.x = p.x + self:stamp_letter(p.font,codepoint,p.scale,p.x,p.y,p.r,p.g,p.b,p.lmask,p.rmask,p.tmask,p.bmask,p.hflip,p.vflip)
  end

end

frame_mt_funcs.stamp_string = function(self,f,str,scale,x,y,r,g,b,max,lmask,rmask,tmask,bmask,hflip,vflip)
  local lmask_applied = false
  if not lmask then
    lmask_applied = true
  end

  rmask = rmask or 0
  tmask = tmask or 0
  bmask = bmask or 0
  hflip = hflip or false
  vflip = vflip or false

  tmask = tmask * scale
  bmask = bmask * scale

  local codepoints = bdf.utf8_to_table(str)

  local xi = x
  for _,codepoint in ipairs(codepoints) do
    if not f.widths[codepoint] then
      codepoint = 32
    end
    if lmask and lmask >= (f.widths[codepoint] * scale) then
      lmask = lmask - (f.widths[codepoint] * scale)
      xi = xi + (f.widths[codepoint] * scale)
    else
      if max and  xi + (f.widths[codepoint] * scale) > max then
        rmask =  (f.widths[codepoint] * scale) - (max - xi)
      end
      xi = xi + self:stamp_letter(f,codepoint,scale,xi,y,r,g,b,
        lmask_applied == false and lmask or 0,
        rmask,tmask,bmask,hflip,vflip)
      if lmask_applied == false then
        lmask_applied = true
        lmask = 0
      end
    end
  end
end

frame_mt_funcs.stamp_string_hsl = function(self,f,str,scale,x,y,h,s,l,max,lmask,rmask,tmask,bmask,hflip,vflip)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:stamp_string(f,str,scale,x,y,r,g,b,max,lmask,rmask,tmask,bmask,hflip,vflip)
end

frame_mt_funcs.stamp_letter_hsl = function(self,f,codepoint,scale,x,y,h,s,l,hloffset,hroffset,ytoffset,yboffset,hflip,vflip)
  local r, g, b = hsl_to_rgb(h,s,l)
  return self:stamp_letter(f,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset,hflip,vflip)
end


