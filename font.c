#include "X11/Xft/Xft.h"

#include "config.h"
#include "font.h"
#include "log.h"
#include "x11.h"

XftFont *stLabelFont = NULL;
XftFont *stIconFont = NULL;

/* XXX config should be loaded */
void
InitializeFonts()
{
    stLabelFont = XftFontOpenName(stDisplay, 0, stConfig.labelFontname);
    stIconFont = XftFontOpenName(stDisplay, 0, stConfig.iconFontname);
}

void
TeardownFonts()
{
    XftFontClose(stDisplay, stLabelFont);
    XftFontClose(stDisplay, stIconFont);
}

void
GetTextPosition(const char *s, XftFont *ft, HAlign ha, VAlign va, int w, int h, int *x, int *y)
{
    XGlyphInfo info;
    XftTextExtentsUtf8(stDisplay, ft, (FcChar8*)s, strlen(s), &info);

    switch ((int)ha) {
        case AlignLeft:
            *x = 0;
            break;
        case AlignCenter:
            *x = (w - (info.xOff >= info.width ? info.xOff : info.width)) / 2;
            break;
        case AlignRight:
            *x = w - (info.xOff >= info.width ? info.xOff : info.width);
            break;
    }
    switch ((int)va) {
        case AlignTop:
            *y = h + ft->ascent + ft->descent - ft->descent + info.yOff;
            break;
        case AlignMiddle:
            *y = (h + ft->ascent + ft->descent) / 2 - ft->descent + info.yOff;
            break;
        case AlignBottom:
            *y = 0;
            break;
    }
}

void
WriteText(Drawable d, const char*s, XftFont *ft, int color, int x, int y)
{
    XftColor xftc;
    XftDraw *draw;
    Visual *v = DefaultVisual(stDisplay, 0);
    Colormap cm = DefaultColormap(stDisplay, 0);

    draw = XftDrawCreate(stDisplay, d, v, cm);
      
    char name[] = "#ffffff";
    snprintf(name, sizeof(name), "#%06X", color);
    XftColorAllocName (stDisplay, DefaultVisual(stDisplay, DefaultScreen(stDisplay)),
        DefaultColormap(stDisplay, DefaultScreen(stDisplay)), name, &xftc);
      
    XftDrawStringUtf8(draw, &xftc, ft, x, y, (XftChar8 *)s, strlen(s));
    XftDrawDestroy(draw);
    XftColorFree(stDisplay,v, cm, &xftc);
}


