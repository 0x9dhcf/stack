#include "font.h"
#include "X11/Xft/Xft.h"
#include "log.h"
#include "stack.h"

void
GetTextPosition(const char *s, XftFont *ft, HAlign ha, VAlign va, int w, int h, int *x, int *y)
{
    XGlyphInfo info;
    XftTextExtentsUtf8(st_dpy, ft, (FcChar8*)s, strlen(s), &info);

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
    Visual *v = DefaultVisual(st_dpy, 0);
    Colormap cm = DefaultColormap(st_dpy, 0);

    draw = XftDrawCreate(st_dpy, d, v, cm);
      
    char name[] = "#ffffff";
    snprintf(name, sizeof(name), "#%06X", color);
    XftColorAllocName (st_dpy, DefaultVisual(st_dpy, DefaultScreen(st_dpy)),
    DefaultColormap(st_dpy, DefaultScreen(st_dpy)), name, &xftc);
      
    XftDrawStringUtf8(draw, &xftc, ft, x, y, (XftChar8 *)s, strlen(s));
    XftDrawDestroy(draw);
    XftColorFree(st_dpy,v, cm, &xftc);
}


