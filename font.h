#ifndef __FONT_H__
#define __FONT_H__

#include <X11/Xft/Xft.h>

typedef enum _HAlign {
    AlignLeft,
    AlignCenter,
    AlignRight
} HAlign;

typedef enum _VAlign {
    AlignTop,
    AlignMiddle,
    AlignBottom
} VAlign;

/* XXX might not need to be global */
extern XftFont *stLabelFont;
extern XftFont *stIconFont;

void InitializeFonts();
void TeardownFonts();
void GetTextPosition(const char *s, XftFont *ft, HAlign ha, VAlign va, int w, int h, int *x, int *y);
void WriteText(Drawable d, const char*s, XftFont *ft, int color, int x, int y); 

#endif
