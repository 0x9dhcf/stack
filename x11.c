#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>
#include <X11/Xft/Xft.h>

#include "config.h"
#include "log.h"
#include "x11.h"

static char* xatoms[] =  {
    "WM_STATE",
    "WM_DELETE_WINDOW",
    "WM_TAKE_FOCUS",
    "WM_PROTOCOLS",
    "_MOTIF_WM_HINTS",
    "_NET_SUPPORTED",
    "_NET_CLIENT_LIST",
    "_NET_CLIENT_LIST_STACKING",
    "_NET_NUMBER_OF_DESKTOPS",
    "_NET_DESKTOP_GEOMETRY",
    "_NET_DESKTOP_VIEWPORT",
    "_NET_CURRENT_DESKTOP",
    "_NET_DESKTOP_NAMES",
    "_NET_ACTIVE_WINDOW",
    "_NET_WORKAREA",
    "_NET_SUPPORTING_WM_CHECK",
    "_NET_VIRTUAL_ROOTS",
    "_NET_DESKTOP_LAYOUT",
    "_NET_SHOWING_DESKTOP",
    "_NET_CLOSE_WINDOW",
    "_NET_MOVERESIZE_WINDOW",
    "_NET_WM_MOVERESIZE",
    "_NET_RESTACK_WINDOW",
    "_NET_REQUEST_FRAME_EXTENTS",
    "_NET_WM_NAME",
    "_NET_WM_VISIBLE_NAME",
    "_NET_WM_ICON_NAME",
    "_NET_WM_VISIBLE_ICON_NAME",
    "_NET_WM_DESKTOP",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_STATE",
    "_NET_WM_ALLOWED_ACTIONS",
    "_NET_WM_STRUT",
    "_NET_WM_STRUT_PARTIAL",
    "_NET_WM_ICON_GEOMETRY",
    "_NET_WM_ICON",
    "_NET_WM_PID",
    "_NET_WM_HANDLED_ICONS",
    "_NET_WM_USER_TIME",
    "_NET_WM_USER_TIME_WINDOW",
    "_NET_FRAME_EXTENTS",
    "_NET_WM_PING",
    "_NET_WM_SYNC_REQUEST",
    "_NET_WM_SYNC_REQUEST_COUNTER",
    "_NET_WM_FULLSCREEN_MONITORS",
    "_NET_WM_FULL_PLACEMENT",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_UTILITY",
    "_NET_WM_WINDOW_TYPE_SPLASH",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
    "_NET_WM_WINDOW_TYPE_POPUP_MENU",
    "_NET_WM_WINDOW_TYPE_TOOLTIP",
    "_NET_WM_WINDOW_TYPE_NOTIFICATION",
    "_NET_WM_WINDOW_TYPE_COMBO",
    "_NET_WM_WINDOW_TYPE_DND",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_STATE_MODAL",
    "_NET_WM_STATE_STICKY",
    "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_MAXIMIZED_HORZ",
    "_NET_WM_STATE_SHADED",
    "_NET_WM_STATE_SKIP_TASKBAR",
    "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_ABOVE",
    "_NET_WM_STATE_BELOW",
    "_NET_WM_STATE_DEMANDS_ATTENTION",
    "_NET_WM_ACTION_MOVE",
    "_NET_WM_ACTION_RESIZE",
    "_NET_WM_ACTION_MINIMIZE",
    "_NET_WM_ACTION_SHADE",
    "_NET_WM_ACTION_STICK",
    "_NET_WM_ACTION_MAXIMIZE_HORZ",
    "_NET_WM_ACTION_MAXIMIZE_VERT",
    "_NET_WM_ACTION_FULLSCREEN",
    "_NET_WM_ACTION_CHANGE_DESKTOP",
    "_NET_WM_ACTION_CLOSE",
    "_NET_WM_ACTION_ABOVE",
    "_NET_WM_ACTION_BELOW"
};

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


Display         *stDisplay = NULL;
int              stScreen;
Window           stRoot;
unsigned long    stNumLockMask;
int              stXRandREventBase;
Atom             stAtoms[AtomCount];
Cursor           stCursors[CursorCount];
XftFont         *stLabelFont = NULL;
XftFont         *stIconFont = NULL;

void
InitializeX11()
{
    int ebr;
    XModifierKeymap *modmap;

    /* open the display */
    stDisplay = XOpenDisplay(0);
    if (!stDisplay)
        FLog("Can't open display.");

    /* check for extensions */
    if (!XRRQueryExtension(stDisplay, &stXRandREventBase, &ebr)) {
        XCloseDisplay(stDisplay);
        FLog("Can't load XRandR extension.");
    }

    stScreen = DefaultScreen(stDisplay);
    stRoot = RootWindow(stDisplay, stScreen);

    /* setup the num lock modifier */
    modmap = XGetModifierMapping(stDisplay);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(stDisplay, XK_Num_Lock))
                stNumLockMask = (1 << i);
    XFreeModifiermap(modmap);

    XInternAtoms(stDisplay, xatoms, AtomCount, False, stAtoms);

    for (int i = 0; i < CursorCount; ++i)
        stCursors[i] = XCreateFontCursor(stDisplay, xcursors[i]);

    stLabelFont = XftFontOpenName(stDisplay, 0, stConfig.labelFontname);
    stIconFont = XftFontOpenName(stDisplay, 0, stConfig.iconFontname);
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
    XftColorAllocName(stDisplay, DefaultVisual(stDisplay,
                DefaultScreen(stDisplay)),
                DefaultColormap(stDisplay, DefaultScreen(stDisplay)),
                name, &xftc);
    XftDrawStringUtf8(draw, &xftc, ft, x, y, (XftChar8 *)s, strlen(s));
    XftDrawDestroy(draw);
    XftColorFree(stDisplay,v, cm, &xftc);
}



void
TeardownX11()
{
    XftFontClose(stDisplay, stLabelFont);
    XftFontClose(stDisplay, stIconFont);

    for (int i = 0; i < CursorCount; ++i)
        stCursors[i] = XFreeCursor(stDisplay, stCursors[i]);

    XSync(stDisplay, False);
    XCloseDisplay(stDisplay);
}

