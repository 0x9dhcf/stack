#ifndef __CURSORS_H__
#define __CURSORS_H__

#include <X11/Xutil.h>

enum CursorType {
    CursorNormal,
    CursorMove,
    CursorResizeNorth,
    CursorResizeNorthWest,
    CursorResizeWest,
    CursorResizeSouthWest,
    CursorResizeSouth,
    CursorResizeSouthEast,
    CursorResizeEast,
    CursorResizeNorthEast,
    CursorCount
};

extern Cursor stCursors[CursorCount];

void InitializeCursors();
void TeardownCursors();

#endif
