"""Generate offline Material Symbols Rounded PNGs and antialiased Roboto VLW fonts.
Symbols: Google, Apache-2.0. Roboto: Google, OFL-1.1. See adjacent licenses.
Requires Pillow, fonttools, aggdraw. No network access at generation time.
"""
from pathlib import Path
import io,struct,xml.etree.ElementTree as ET
from PIL import Image,ImageDraw,ImageFont
import aggdraw
from fontTools.pens.basePen import BasePen
from fontTools.svgLib.path import parse_path
ROOT=Path(__file__).resolve().parents[1]
ASSETS=Path(__file__).resolve().parent
NAMES=['None','Camera','Video','Folder','Gear','Grid','Palette','Temp','Touch','Info','Trash','Play','Pause','Back','Check','Prev','Next','Rewind','Reset','Stop','Storage','Format','Eject','Unit','Tune','Min','Max','Spot','Warning','Photo']
THEMES=[(229,246,255),(75,223,255),(143,174,207),(255,108,138),(7,14,28)]
class Pen(BasePen):
 def __init__(self,path,scale):super().__init__(None);self.path=path;self.s=scale
 def point(self,p):return p[0]*self.s,(p[1]+960)*self.s
 def _moveTo(self,p):self.path.moveto(*self.point(p))
 def _lineTo(self,p):self.path.lineto(*self.point(p))
 def _curveToOne(self,a,b,c):self.path.curveto(*self.point(a),*self.point(b),*self.point(c))
 def _closePath(self):self.path.close()
 def _endPath(self):pass

def icon(name,color,size):
 scale=4;mask=Image.new('L',(size*scale,size*scale));d=aggdraw.Draw(mask);path=aggdraw.Path();pen=Pen(path,size*scale/960)
 for node in ET.parse(ASSETS/'material'/f'{name}.svg').getroot().iter():
  if 'd' in node.attrib:parse_path(node.attrib['d'],pen)
 d.path(path,aggdraw.Brush(255));d.flush();mask=mask.resize((size,size),Image.Resampling.LANCZOS)
 im=Image.new('RGBA',(size,size),(*color,255));im.putalpha(mask);return im

def generate_icons():
 data=[];refs=[]
 for ni,name in enumerate(NAMES):
  themes=[]
  for ti,c in enumerate(THEMES):
   sizes=[]
   for si,size in enumerate((18,24,36)):
    if not ni:sizes.append('{nullptr,0}');continue
    raw=io.BytesIO();icon(name,c,size).save(raw,format='PNG',optimize=True);blob=raw.getvalue();ident=f'icon_{ni}_{ti}_{si}'
    data.append(f'static const uint8_t {ident}[] PROGMEM={{'+','.join(map(str,blob))+'};');sizes.append('{'+ident+','+str(len(blob))+'}')
   themes.append('{'+','.join(sizes)+'}')
  refs.append('{'+','.join(themes)+'}')
 (ROOT/'ui_icons.hpp').write_text('#pragma once\n#include <stdint.h>\n// Google Material Symbols Rounded, Apache-2.0. See ui_assets/material/LICENSE.\nnamespace modern_icon_assets {\nstruct Asset{const uint8_t* data;uint16_t size;};\n'+'\n'.join(data)+f'\nstatic const Asset icons[{len(NAMES)}][5][3]={{'+','.join(refs)+'};\n}\n')
 atlas=Image.new('RGB',(1000,300),THEMES[4]);draw=ImageDraw.Draw(atlas)
 for i,name in enumerate(NAMES[1:]):
  x=i%10*100;y=i//10*100;pic=icon(name,THEMES[1],36);atlas.paste(pic,(x+32,y+6),pic);draw.text((x+5,y+51),name,fill=THEMES[0])
 atlas.save(ASSETS/'material-atlas.png')

def font_vlw(size,weight):
 font=ImageFont.truetype(str(ASSETS/'Roboto.ttf'),size)
 axes=font.get_variation_axes();font.set_variation_by_axes([weight if b'Weight' in a['name'] else a['default'] for a in axes])
 codes=[*range(33,127),176];glyphs=[];bitmaps=[]
 ascent,descent=font.getmetrics();max_asc=0;max_desc=0
 for code in codes:
  ch=chr(code);left,top,right,bottom=font.getbbox(ch,anchor='ls');w=max(1,right-left);h=max(1,bottom-top)
  im=Image.new('L',(w,h));ImageDraw.Draw(im).text((-left,-top),ch,font=font,fill=255,anchor='ls')
  advance=round(font.getlength(ch));glyphs.append(struct.pack('>7i',code,h,w,advance,-top,left,0));bitmaps.append(im.tobytes());max_asc=max(max_asc,-top);max_desc=max(max_desc,bottom)
 blob=struct.pack('>6i',len(codes),11,max_asc+max_desc,0,max_asc,max_desc)+b''.join(glyphs)+b''.join(bitmaps)
 return blob,font

def generate_fonts():
 decl=[]
 for name,size,weight in [('caption',11,450),('body',14,500),('number',21,550)]:
  raw,font=font_vlw(size,weight);decl.append(f'static const uint8_t {name}[] PROGMEM={{'+','.join(map(str,raw))+'};')
  (ASSETS/(name+'.vlw')).write_bytes(raw)
 (ROOT/'ui_fonts.hpp').write_text('#pragma once\n#include <stdint.h>\n// Roboto, OFL-1.1; see ui_assets/Roboto-OFL.txt.\nnamespace modern_fonts {\n'+'\n'.join(decl)+'\n}\n')
 im=Image.new('RGB',(820,220),THEMES[4]);d=ImageDraw.Draw(im)
 for i,(size,weight) in enumerate([(11,450),(14,500),(21,550)]):
  _,f=font_vlw(size*2,weight);d.text((15,12+i*65),'Thermal  27.5 °C  81.5 °F  300.7 K',font=f,fill=THEMES[0])
 im.save(ASSETS/'font-preview.png')
if __name__=='__main__':generate_icons();generate_fonts();print('Generated Material icon and Roboto font assets')
