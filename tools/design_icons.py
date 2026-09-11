"""Original deterministic 24-unit outline drawings. Repository MIT license.

These primitives, not generated PNGs, are the authoritative source. Stroke=1.5,
round caps, common 3..21 optical bounds. Rasterization supersamples at 8x.
"""
from PIL import Image, ImageDraw

DRAWINGS = {
    'TestPattern': [('box',3,3,21,21), ('circle',6,6,18,18), ('line',12,3,12,21), ('line',3,12,21,12)],
    'SolidColor': [('box',3,3,21,21), ('line',5,17,17,5), ('line',8,19,19,8), ('line',4,12,12,4)],
    'WhiteOutput': [('circle',7,7,17,17), ('line',12,2,12,5), ('line',12,19,12,22), ('line',2,12,5,12), ('line',19,12,22,12)],
    'Blackout': [('box',3,3,21,21), ('line',4,20,20,4), ('line',4,4,20,20)],
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
