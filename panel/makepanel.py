#!/usr/bin/env python3
#
#   makepanel.py  -  Don Cross <cosinekitty@gmail.com>
#
#   Generate the SVG panel design for the VCV Rack version of Sloth Torpor
#
import sys
from svgpanel import *

def Print(message:str) -> int:
    print('makepanel.py:', message)
    return 0

def Save(panel:Panel, filename:str) -> int:
    panel.save(filename)
    return Print('Wrote: ' + filename)

PANEL_WIDTH = 4
PANEL_COLOR  = '#e6e5e5'
BORDER_COLOR = '#e6e5e5'

def GenerateMainPanel() -> int:
    svgFileName = '/home/don/github/nonlinearcircuits/res/SlothTorpor.svg'
    panel = Panel(PANEL_WIDTH)
    pl = Element('g', 'PanelLayer')
    pl.append(BorderRect(PANEL_WIDTH, PANEL_COLOR, BORDER_COLOR))
    panel.append(pl)
    return Save(panel, svgFileName)

if __name__ == '__main__':
    sys.exit(GenerateMainPanel())
