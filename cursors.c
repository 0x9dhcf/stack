#include <X11/cursorfont.h>

#include "stack.h"

static unsigned int xcursors[] = {
    XC_left_ptr,
    XC_fleur,
    XC_top_side,
    XC_top_right_corner,
    XC_right_side,
    XC_bottom_right_corner,
    XC_bottom_side,
    XC_bottom_left_corner,
    XC_left_side,
    XC_top_left_corner
};

Cursor stCursors[CursorCount];

void
InitializeCursors()
{
    for (int i = 0; i < CursorCount; ++i)
        stCursors[i] = XCreateFontCursor(stDisplay, xcursors[i]);
}

void
TeardownCursors()
{
    for (int i = 0; i < CursorCount; ++i)
        stCursors[i] = XFreeCursor(stDisplay, stCursors[i]);
}
