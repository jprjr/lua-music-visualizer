stream.get_pixel = function(self,x,y)
  return self.video:get_pixel(x,y)
end

stream.draw_rectangle = function(self,x1,y1,x2,y2,r,g,b,a)
  return self.video:draw_rectangle(x1,y1,x2,y2,r,g,b,a)
end

stream.draw_rectangle_hsl = function(self,x1,y1,x2,y2,h,s,l,a)
  return self.video:draw_rectangle_hsl(x1,y1,x2,y2,h,s,l,a)
end

stream.stamp_string_adv = function(self,str,props,userd)
  return self.video:stamp_string_adv(str,props,userd)
end

stream.stamp_string = function(self,font,str,scale,x,y,r,g,b,max,lmask,rmask,tmask,bmask)
  return self.video:stamp_string(font,str,scale,x,y,r,g,b,max,lmask,rmask,tmask,bmask)
end

stream.stamp_letter = function(self,font,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset)
  return self.video:stamp_letter(font,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset)
end

stream.stamp_string_hsl = function(self,font,str,scale,x,y,h,s,l,max,lmask,rmask,tmask,bmask)
  return self.video:stamp_string_hsl(font,str,scale,x,y,h,s,l,max,lmask,rmask,tmask,bmask)
end

stream.stamp_letter_hsl = function(self,font,codepoint,scale,x,y,h,s,l,hloffset,hroffset,ytoffset,yboffset)
  return self.video:stamp_letter_hsl(font,codepoint,scale,x,y,h,s,l,hloffset,hroffset,ytoffset,yboffset)
end

stream.draw_rectangle_rgb = stream.draw_rectangle
stream.stamp_string_rgb = stream.stamp_string
stream.stamp_letter_rgb = stream.stamp_letter

stream.set_image = function(self,b)
  return self.video:set(b)
end

stream.stamp_image = function(self,b,x,y,flip,mask,alpha)
  return self.video:stamp_image(b,x,y,flip,mask,alpha)
end

stream.blend = function(self,b,alpha)
  return self.video:blend(b,alpha)
end

