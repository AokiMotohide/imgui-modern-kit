"""Original deterministic 24-unit outline drawings. Repository MIT license.

These primitives, not generated PNGs, are the authoritative source. Stroke=1.5,
round caps, common 3..21 optical bounds. Rasterization supersamples at 8x.
"""
import math
from PIL import Image, ImageDraw

DRAWINGS = {
    'TestPattern': [('box',3,3,21,21), ('circle',6,6,18,18), ('line',12,3,12,21), ('line',3,12,21,12)],
    'SolidColor': [('box',3,3,21,21), ('line',5,17,17,5), ('line',8,19,19,8), ('line',4,12,12,4)],
    'WhiteOutput': [('circle',7,7,17,17), ('line',12,2,12,5), ('line',12,19,12,22), ('line',2,12,5,12), ('line',19,12,22,12)],
    'Blackout': [('box',3,3,21,21), ('line',4,20,20,4), ('line',4,4,20,20)],
    'Usb': [('line',12,3,12,12), ('box',8,7,16,12), ('line',12,12,12,20),
            ('line',12,16,7,16,7,13), ('line',12,18,17,18,17,15),
            ('circle',5,10,9,14), ('line',10,3,12,1,14,3)],
    'Network': [('line',12,7,6,17), ('line',12,7,18,17), ('line',6,17,18,17),
                ('circle',9,3,15,9), ('circle',3,14,9,20), ('circle',15,14,21,20)],
    'Smartphone': [('box',6,2,18,22), ('line',9,5,15,5), ('line',10,19,14,19)],
    'IndustrialCamera': [('box',3,8,21,20), ('line',7,8,9,4,16,4,18,8),
                         ('circle',8,10,16,18), ('circle',10,12,14,16),
                         ('line',18,11,20,11)],
    'CommandPalette': [('box',3,5,21,19), ('line',6,9,8,11,6,13), ('line',11,14,17,14)],
    'Keyboard': [('box',3,6,21,18), ('line',6,10,7,10), ('line',10,10,11,10), ('line',14,10,15,10), ('line',18,10,18,10), ('line',7,14,17,14)],
    'Mouse': [('box',7,3,17,21), ('line',7,10,17,10), ('line',12,3,12,8)],
    'Touch': [('circle',9,3,15,9), ('line',11,15,11,7,13,7,13,13,17,12,19,15,17,21,10,21,6,15,8,13,11,16)],
    'Accessibility': [('circle',10,3,14,7), ('line',4,9,20,9), ('line',12,9,12,14,7,21), ('line',12,14,17,21)],
    'ScreenReader': [('box',3,4,17,16), ('line',7,20,13,20), ('line',10,16,10,20), ('line',18,8,21,11,18,14)],
    'HighContrast': [('circle',3,3,21,21), ('line',12,3,12,21), ('line',15,6,15,18), ('line',18,9,18,15)],
    'ReduceMotion': [('line',3,7,10,7), ('line',3,12,8,12), ('line',3,17,10,17), ('line',14,7,14,17), ('line',19,7,19,17)],
    'TextSize': [('line',3,5,15,5), ('line',9,5,9,20), ('line',15,12,21,12), ('line',18,12,18,20)],
    'RTL': [('line',5,5,20,5), ('line',9,9,20,9), ('line',4,16,20,16), ('line',8,12,4,16,8,20)],
    'ChevronUp': [('line',5,15,12,8,19,15)],
    'ChevronDown': [('line',5,9,12,16,19,9)],
    'ChevronLeft': [('line',15,5,8,12,15,19)],
    'ChevronRight': [('line',9,5,16,12,9,19)],
    'Dock': [('box',3,3,21,21), ('line',3,15,21,15), ('line',12,6,12,12), ('line',9,9,12,12,15,9)],
    'Undock': [('box',3,7,17,21), ('line',8,7,8,16,17,16), ('line',12,12,21,3), ('line',16,3,21,3,21,8)],
    'WindowMinimize': [('line',4,18,20,18)],
    'WindowMaximize': [('box',4,4,20,20)],
    'WindowRestore': [('box',3,8,16,21), ('line',8,8,8,3,21,3,21,16,16,16)],
    'Columns': [('box',3,4,21,20), ('line',9,4,9,20), ('line',15,4,15,20)],
    'Rows': [('box',3,4,21,20), ('line',3,9,21,9), ('line',3,15,21,15)],
    'ExpandAll': [('line',5,9,12,3,19,9), ('line',5,15,12,21,19,15), ('line',5,12,19,12)],
    'CollapseAll': [('line',5,3,12,9,19,3), ('line',5,21,12,15,19,21), ('line',5,12,19,12)],
    'Ungroup': [('box',3,3,10,10), ('box',14,14,21,21), ('line',15,8,21,2), ('line',3,21,9,15)],
    'SortNeutral': [('line',7,3,7,21), ('line',3,7,7,3,11,7), ('line',17,3,17,21), ('line',13,17,17,21,21,17)],
    'FilterClear': [('line',3,4,21,4,14,12,14,20,10,18,10,12,3,4), ('line',16,15,21,20), ('line',21,15,16,20)],
    'DragHandle': [('circle',7,4,9,6), ('circle',15,4,17,6), ('circle',7,11,9,13), ('circle',15,11,17,13), ('circle',7,18,9,20), ('circle',15,18,17,20)],
    'Archive': [('box',3,3,21,8), ('box',5,8,19,21), ('line',10,12,14,12)],
    'ArchiveRestore': [('box',3,3,21,8), ('line',5,8,5,21,19,21,19,8), ('line',12,18,12,11), ('line',8,15,12,11,16,15)],
    'FolderAdd': [('line',3,20,3,5,10,5,12,8,21,8,21,20,3,20), ('line',8,14,16,14), ('line',12,10,12,18)],
    'FolderMove': [('line',3,20,3,5,10,5,12,8,21,8,21,20,3,20), ('line',7,14,17,14), ('line',13,10,17,14,13,18)],
    'Replace': [('line',3,7,20,7,16,3), ('line',21,17,4,17,8,21), ('line',3,4,3,11), ('line',21,13,21,20)],
    'KeyCommand': [('box',7,7,17,17), ('circle',3,3,9,9), ('circle',15,3,21,9), ('circle',3,15,9,21), ('circle',15,15,21,21)],
    'KeyEnter': [('line',20,5,20,15,4,15), ('line',9,10,4,15,9,20)],
    'KeyEscape': [('box',3,5,21,19), ('line',15,12,7,12), ('line',10,9,7,12,10,15)],
    'KeyTab': [('line',3,7,19,7), ('line',15,3,19,7,15,11), ('line',21,3,21,11), ('line',21,17,5,17), ('line',9,13,5,17,9,21), ('line',3,13,3,21)],
}
def _arc(cx, cy, r, a0, a1, steps=18):
    """Polyline points of an arc; angles in degrees, y grows downwards."""
    points = []
    for i in range(steps + 1):
        a = math.radians(a0 + (a1 - a0) * i / steps)
        points += [round(cx + r * math.cos(a), 2), round(cy + r * math.sin(a), 2)]
    return points


# LED and live-surface icons (additions after the design-system set).
DRAWINGS.update({
    'Led': [('line',6,13,18,13), ('line',8,13,8,9,10,5,14,5,16,9,16,13), ('line',10,13,10,20), ('line',14,13,14,20)],
    'LedStrip': [('box',3,9,21,15), ('circle',6,11,8,13), ('circle',11,11,13,13), ('circle',16,11,18,13)],
    'LedMatrix': [('box',3,3,21,21)] + [('circle',x-1,y-1,x+1,y+1) for x in (8,12,16) for y in (8,12,16)],
    'LedWall': [('box',3,4,21,18), ('line',9,4,9,18), ('line',15,4,15,18), ('line',3,11,21,11), ('line',8,21,16,21)],
    'LedCabinet': [('box',3,3,21,21), ('box',6,6,18,18), ('line',12,6,12,18), ('line',6,12,18,12)],
    'Pixel': [('box',3,3,21,21), ('box',8,8,16,16), ('line',3,12,8,12), ('line',16,12,21,12)],
    'Controller': [('box',3,6,21,18), ('box',6,12,9,15), ('box',11,12,14,15), ('box',16,12,19,15), ('line',6,9,10,9)],
    'Port': [('line',5,19,5,8,8,8,8,5,16,5,16,8,19,8,19,19,5,19), ('line',8,12,8,16), ('line',12,12,12,16), ('line',16,12,16,16)],
    'Dmx': [('circle',3,3,21,21), ('circle',8,8,10,10), ('circle',14,8,16,10), ('circle',11,15,13,17), ('line',12,3,12,5)],
    'Universe': [('line',12,3,21,8,12,13,3,8,12,3), ('line',3,12,12,17,21,12), ('line',3,16,12,21,21,16)],
    'Fixture': [('line',8,4,16,4,19,14,5,14,8,4), ('line',9,14,9,19), ('line',15,14,15,19), ('line',7,20,17,20)],
    'MovingHead': [('circle',7,3,17,13), ('circle',10,6,14,10), ('line',12,13,12,17), ('box',5,17,19,21)],
    'Dimmer': [('line',9,3,9,21), ('box',5,9,13,14), ('line',16,8,19,8), ('line',16,12,19,12), ('line',16,16,19,16)],
    'Brightness': [('circle',8,8,16,16), ('line',12,3,12,5), ('line',12,19,12,21), ('line',3,12,5,12), ('line',19,12,21,12),
                   ('line',5.5,5.5,7,7), ('line',17,17,18.5,18.5), ('line',17,7,18.5,5.5), ('line',5.5,18.5,7,17)],
    'ColorTemperature': [('line',9,14,9,5,10,3,14,3,15,5,15,14), ('circle',6,13,18,21), ('line',12,8,12,16),
                         ('line',18,6,20,6), ('line',18,9,20,9), ('line',18,12,20,12)],
    'Gamma': [('line',4,3,4,20,21,20), ('line',4,20,9,19,13,16,17,11,21,3)],
    'RgbChannels': [('circle',3,3,14,14), ('circle',10,3,21,14), ('circle',6.5,9,17.5,20)],
    'WhiteChannel': [('box',3,3,21,21), ('line',6,8,9,16,12,10,15,16,18,8)],
    'Power': [('line',*_arc(12, 13, 8, -50, 230)), ('line',12,3,12,12)],
    'PowerBudget': [('line',*_arc(12, 17, 9, 180, 360)), ('line',12,17,17,11), ('line',3,17,21,17)],
    'Strobe': [('line',12,3,12,21), ('line',3,12,21,12), ('line',5.5,5.5,18.5,18.5), ('line',18.5,5.5,5.5,18.5)],
    'Chase': [('circle',3,10,7,14), ('circle',10,10,14,14), ('circle',17,10,21,14), ('line',4,18,20,18), ('line',16,15,20,18,16,21)],
    'Gradient': [('box',3,3,21,21), ('line',7,6,7,18), ('line',10,6,10,18), ('line',13.5,6,13.5,18), ('line',17.5,6,17.5,18)],
    'ColorOrder': [('line',6,4,6,13), ('line',12,4,12,13), ('line',18,4,18,13), ('line',6,18,18,18), ('line',15,15,18,18,15,21), ('line',9,15,6,18,9,21)],
    'Identify': [('circle',6,6,18,18), ('circle',10,10,14,14), ('line',12,3,12,6), ('line',12,18,12,21), ('line',3,12,6,12), ('line',18,12,21,12)],
    'Freeze': [('box',3,3,21,21), ('line',9,8,9,16), ('line',15,8,15,16)],
    'Flash': [('line',13,3,6,13,11,13,10,21,18,10,13,10,13,3)],
    'Scene': [('box',3,3,21,21), ('box',6,6,11,11), ('box',13,6,18,11), ('box',6,13,11,18), ('box',13,13,18,18)],
    'ImageButton': [('box',3,4,19,17), ('line',4,15,9,10,12,13,14,11,18,15), ('circle',13,6,16,9), ('line',15,12,15,20,17,18,19,21)],
    'ControlSurface': [('line',6,3,6,21), ('line',12,3,12,21), ('line',18,3,18,21), ('box',3.5,12,8.5,16), ('box',9.5,6,14.5,10), ('box',15.5,15,20.5,19)],
    'Fader': [('line',12,3,12,21), ('box',7,10,17,15), ('line',3,5,5,5), ('line',3,12,5,12), ('line',3,19,5,19)],
    'Knob': [('circle',7,7,17,17), ('line',12,12,15,8), ('line',*_arc(12, 12, 10, 135, 405))],
})

for side in ('Left','Right','Bottom'):
    for action in ('Open','Close'):
        lines=[('box',3,3,21,21)]
        if side=='Bottom':
            lines += [('line',3,16,21,16),('line',9,10 if action=='Open' else 7,12,7 if action=='Open' else 10,15,10 if action=='Open' else 7)]
        else:
            x=8 if side=='Left' else 16
            lines += [('line',x,3,x,21)]
            center=14 if side=='Left' else 10
            direction=1 if (side=='Left')==(action=='Open') else -1
            lines += [('line',center-direction*2,8,center+direction*2,12,center-direction*2,16)]
        DRAWINGS['Panel'+side+action]=lines

def render(name):
    scale=8
    image=Image.new('RGBA',(24*scale,24*scale),(255,255,255,0))
    d=ImageDraw.Draw(image); width=12
    for primitive,*coordinates in DRAWINGS[name]:
        points=[round(x*scale) for x in coordinates]
        if primitive=='line':
            pairs=list(zip(points[::2],points[1::2])); d.line(pairs,fill='white',width=width,joint='curve')
            for x,y in pairs: d.ellipse((x-width/2,y-width/2,x+width/2,y+width/2),fill='white')
        elif primitive=='box': d.rounded_rectangle(points,radius=scale,outline='white',width=width)
        else: d.ellipse(points,outline='white',width=width)
    return image
