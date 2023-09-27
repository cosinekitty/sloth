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

SLOTH_PANEL_WIDTH = 4
TRIPLE_SLOTH_PANEL_WIDTH = 8
PANEL_COLOR  = '#e6e5e5'
BORDER_COLOR = '#e6e5e5'


def SlothTitlePath(text: str, font: Font, panel: Panel) -> TextPath:
    ti = TextItem(text, font, 14.0)
    return ti.toPath(
        panel.mmWidth/2 - 0.3,
        6.0,
        HorizontalAlignment.Center,
        VerticalAlignment.Top,
        'fill:#5c3114;stroke:#5c3114;stroke-width:0.1;stroke-linejoin:bevel',
        'text_title',
        1.15,
        1.0
    )


def LogoElement(panel: Panel) -> Element:
    # dx = -40.4 moves logo flush left on a panel that is 20.32 wide
    # dx = +40.9 moves logo just off the right side of the same panel
    # Adding the two should bring the logo left side flush to the middle
    # dx = -109.5 moves the logo's right edge just off the left side of the panel
    # The difference (109.5 - 40.4) = 69.1 must be the width of the logo
    print(panel.mmWidth)
    scale = 0.25
    logoWidth = 69.1
    basisPanelWidth = 20.32
    dx = (panel.mmWidth - basisPanelWidth) / (2 * scale) - (logoWidth / 2)
    return (Element('g', 'nlc_logo', [
            Polyline('87 351.05 87 339.53 94.75 351.05 94.75 339.53', 'fill:none;stroke-linecap:round;stroke-linejoin:round;stroke-width:1.56px'),
            Polyline('94.75 339.53 94.75 351.05 102.49 351.05', 'fill:none;stroke-linecap:round;stroke-linejoin:round;stroke-width:1.56px'),
            Path('M108.79,349.13a2,2,0,0,1-1.94,1.92H103a2,2,0,0,1-1.94-1.92v-7.68a1.91,1.91,0,0,1,1.94-1.92h3.87a1.91,1.91,0,0,1,1.94,1.92', 'fill:none;stroke-linecap:round;stroke-linejoin:round;stroke-width:1.56px'),
            Path('M52.28,356.21a11.34,11.34,0,1,0,0-22.68,11.34,11.34,0,1,0,0,22.68', 'fill:none;stroke-linecap:round;stroke-linejoin:round;stroke-width:1.08px'),
            LineElement(63.89, 344.81, 81.2, 344.81, 'fill: none;stroke-linecap: round;stroke-linejoin: round;stroke-width: 1.44px'),
            LineElement(55.3, 334.01, 48.52, 342.29, 'fill: none;stroke-linecap: round;stroke-linejoin: round;stroke-width: 1.08px'),
            LineElement(56.63, 347.69, 49.86, 355.73, 'fill: none;stroke-linecap: round;stroke-linejoin: round;stroke-width: 1.08px'),
            LineElement(48.52, 342.29, 56.63, 347.69, 'fill: none;stroke-linecap: round;stroke-linejoin: round;stroke-width: 1.08px'),
            Polygon('56.99 347.45 57.24 355.01 53.73 355.85 50.22 355.73 56.99 347.45', 'fill-rule: evenodd'),
            Polygon('54.82 333.89 57.36 335.09 57.24 347.57 48.16 341.93 54.82 333.89', 'fill-rule: evenodd'),
            Polygon('56.75 349.25 62.2 349.49 61.96 350.81 60.38 352.25 56.63 355.25 56.75 349.25', 'fill-rule: evenodd'),
            Polygon('56.63 341.69 63.17 341.57 63.53 345.17 63.29 347.69 62.32 349.97 56.39 349.73 56.63 341.69', 'fill-rule: evenodd'),
            Polygon('56.99 334.61 59.41 336.17 61.84 338.93 62.8 341.21 63.05 342.05 56.27 341.93 56.99 334.61', 'fill-rule: evenodd')
        ])
        .setAttrib('transform', 'scale({:0.6g}) translate({:0.6g},135)'.format(scale, dx))
        .setAttrib('style', 'fill:#5c3114;stroke:#5c3114;')
    )



def GenerateSlothPanel(variantName: str) -> int:
    svgFileName = '/home/don/github/nonlinearcircuits/res/Sloth{}.svg'.format(variantName)
    with Font('NotoMono-Regular.ttf') as font:
        panel = Panel(SLOTH_PANEL_WIDTH)
        pl = Element('g', 'PanelLayer')
        pl.append(BorderRect(SLOTH_PANEL_WIDTH, PANEL_COLOR, BORDER_COLOR))
        pl.append(SlothTitlePath('sloth', font, panel))
        pl.append(TextItem(variantName.lower(), font, 10.0).toPath(
            panel.mmWidth/2,
            33.3,
            HorizontalAlignment.Center,
            VerticalAlignment.Top,
            'fill:#af5a20;stroke:#90491a;stroke-width:0.05;stroke-linejoin:bevel',
            'text_variant'
        ))
        pl.append(
            Element('g', 'text_big', [
                Path('M 3.15711,98.843067 H 3.140574 L 3.089587,99.034615 H 2.9063073 V 96.890389 H 3.15711 v 0.518142 q 0,0.111621 -0.011024,0.311436 H 3.15711 q 0.14745,-0.225998 0.443728,-0.225998 0.2769855,0 0.4340817,0.206706 0.1584743,0.205327 0.1584743,0.574641 0,0.37207 -0.1584743,0.580154 -0.1584743,0.206705 -0.4340817,0.206705 -0.2852537,0 -0.443728,-0.219108 z m 0,-0.567751 q 0,0.312815 0.095085,0.446484 0.096463,0.132292 0.3059243,0.132292 0.3748261,0 0.3748261,-0.581532 0,-0.570507 -0.3775822,-0.570507 -0.2135958,0 -0.3059243,0.129535 -0.092329,0.129536 -0.092329,0.443728 z'),
                Path('m 5.2544826,96.898657 q 0.146072,0 0.146072,0.157096 0,0.07993 -0.044097,0.119889 -0.042719,0.03858 -0.1019748,0.03858 -0.14745,0 -0.14745,-0.158474 0,-0.157096 0.14745,-0.157096 z m -0.1267794,0.82131 -0.370692,-0.02894 V 97.52153 H 5.378506 v 1.316025 l 0.485069,0.02756 v 0.169499 H 4.6536584 v -0.169499 l 0.4740448,-0.02756 z'),
                Path('m 7.6274626,97.52153 v 0.155718 l -0.2700953,0.03721 q 0.089573,0.117133 0.089573,0.293522 0,0.221864 -0.14745,0.354155 -0.146072,0.130914 -0.407899,0.130914 -0.075792,0 -0.1185112,-0.0083 -0.1378037,0.07717 -0.1378037,0.183279 0,0.115755 0.221864,0.115755 h 0.2576929 q 0.2397785,0 0.3665579,0.106109 0.1267794,0.106109 0.1267794,0.303168 0,0.51952 -0.778591,0.51952 -0.3004121,0 -0.4575083,-0.111621 -0.1557182,-0.110243 -0.1557182,-0.310059 0,-0.292143 0.3307289,-0.37207 -0.1322916,-0.06477 -0.1322916,-0.212217 0,-0.15434 0.183279,-0.264583 -0.1226453,-0.05099 -0.1929252,-0.168121 -0.07028,-0.118511 -0.07028,-0.257693 0,-0.250803 0.1460719,-0.38585 0.14745,-0.136426 0.4189233,-0.136426 0.1185112,0 0.2067056,0.02756 z m -0.7331157,0.799261 q 0.3073022,0 0.3073022,-0.316948 0,-0.329351 -0.3100583,-0.329351 -0.3100584,0 -0.3100584,0.333485 0,0.312814 0.3128145,0.312814 z m 0.1350476,0.689019 h -0.259071 q -0.3169485,0 -0.3169485,0.272851 0,0.239779 0.373448,0.239779 0.5443247,0 0.5443247,-0.307303 0,-0.121267 -0.068902,-0.163986 -0.068902,-0.04134 -0.2728514,-0.04134 z')
            ])
            .setAttrib('transform', 'translate(0,-5.2916669)')
            .setAttrib('style', 'fill:#854116;stroke:#5f2f10;stroke-width:0.05;stroke-linejoin:bevel')
        )
        pl.append(
            Element('g', 'text_small', [
                Path('M 10.674858,99.121464 V 98.89271 q 0.265962,0.118511 0.509874,0.118511 0.37896,0 0.37896,-0.223242 0,-0.07855 -0.06752,-0.136426 -0.06752,-0.05788 -0.311436,-0.146072 -0.321083,-0.119889 -0.405143,-0.219108 -0.08268,-0.100596 -0.08268,-0.237022 0,-0.187413 0.152962,-0.293522 0.15434,-0.107487 0.425814,-0.107487 0.272851,0 0.508495,0.101975 l -0.08268,0.205327 q -0.250803,-0.09922 -0.439594,-0.09922 -0.325217,0 -0.325217,0.183279 0,0.08406 0.0689,0.135047 0.07028,0.04961 0.321083,0.139182 0.290766,0.106109 0.38585,0.209462 0.09646,0.101975 0.09646,0.250803 0,0.206705 -0.16123,0.325216 -0.161231,0.117133 -0.456131,0.117133 -0.341753,0 -0.516764,-0.09508 z'),
                Path('m 13.435067,99.188988 v -0.974273 q 0,-0.201193 -0.03721,-0.279741 -0.03583,-0.07855 -0.121267,-0.07855 -0.11989,0 -0.175011,0.114377 -0.05374,0.114377 -0.05374,0.383094 v 0.835091 h -0.221864 v -0.974273 q 0,-0.358289 -0.172254,-0.358289 -0.115755,0 -0.165365,0.110243 -0.04961,0.108865 -0.04961,0.438216 v 0.784103 h -0.223242 v -1.513085 h 0.175011 l 0.03721,0.203949 h 0.01378 q 0.09233,-0.23151 0.276985,-0.23151 0.224621,0 0.293522,0.250803 h 0.0083 q 0.10473,-0.250803 0.30179,-0.250803 0.176389,0 0.256315,0.128158 0.07993,0.126779 0.07993,0.424435 v 0.988053 z'),
                Path('m 15.015675,99.188988 -0.05099,-0.209462 h -0.01102 q -0.106109,0.13367 -0.21773,0.186035 -0.111621,0.05099 -0.28801,0.05099 -0.22462,0 -0.352777,-0.118511 -0.12678,-0.118511 -0.12678,-0.332107 0,-0.457508 0.702799,-0.479557 l 0.279742,-0.0096 v -0.09508 q 0,-0.325217 -0.336241,-0.325217 -0.202572,0 -0.451997,0.112999 l -0.08682,-0.188791 q 0.270095,-0.132292 0.527788,-0.132292 0.312815,0 0.451997,0.119889 0.140559,0.11989 0.140559,0.384473 v 1.036284 z m -0.06752,-0.740006 -0.223242,0.0096 q -0.270096,0.01102 -0.383095,0.08544 -0.112999,0.07304 -0.112999,0.22462 0,0.245291 0.275608,0.245291 0.202571,0 0.32246,-0.111621 0.121268,-0.112999 0.121268,-0.316949 z'),
                Path('m 16.209056,97.241821 -0.369314,-0.02894 v -0.16812 h 0.621494 v 1.947166 l 0.485069,0.02756 v 0.169499 h -1.211294 v -0.169499 l 0.474045,-0.02756 z'),
                Path('m 17.902663,97.241821 -0.369314,-0.02894 v -0.16812 h 0.621495 v 1.947166 l 0.485069,0.02756 v 0.169499 h -1.211294 v -0.169499 l 0.474044,-0.02756 z')
            ])
            .setAttrib('transform', 'translate(-0.03503753,-5.4001521)')
            .setAttrib('style', 'fill:#dc7f41;stroke:#b15b21;stroke-width:0.05;stroke-linejoin:bevel')
        )
        pl.append(LogoElement(panel))
        panel.append(pl)
        return Save(panel, svgFileName)


def GenerateTripleSlothPanel() -> int:
    svgFileName = '/home/don/github/nonlinearcircuits/res/TripleSloth.svg'
    with Font('NotoMono-Regular.ttf') as font:
        panel = Panel(TRIPLE_SLOTH_PANEL_WIDTH)
        pl = Element('g', 'PanelLayer')
        pl.append(BorderRect(TRIPLE_SLOTH_PANEL_WIDTH, PANEL_COLOR, BORDER_COLOR))
        pl.append(SlothTitlePath('triple sloth', font, panel))
        pl.append(LogoElement(panel))
        panel.append(pl)
        return Save(panel, svgFileName)


if __name__ == '__main__':
    sys.exit(
        GenerateSlothPanel("Torpor") or
        GenerateSlothPanel("Apathy") or
        GenerateSlothPanel("Inertia") or
        GenerateTripleSlothPanel()
    )
