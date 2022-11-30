#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>

#include "log.h"
#include "settings.h"
#include "x11.h"

static char *atomNames[] =  {
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

static unsigned int cursorIds[] = {
    XC_left_ptr,
    XC_fleur,
    XC_top_left_corner,
    XC_top_side,
    XC_top_right_corner,
    XC_right_side,
    XC_bottom_right_corner,
    XC_bottom_side,
    XC_bottom_left_corner,
    XC_left_side
};

Display *display;
int extensions;
Window root;
unsigned long numLockMask;
Atom atoms[AtomCount];
Cursor cursors[CursorCount];

void
SetupX11()
{
    int ebr;
    int xreb;
    XModifierKeymap *modmap;

    /* open the display */
    display = XOpenDisplay(0);
    if (! display)
        FLog("Can't open display.");

    /* check for extensions */
    extensions = ExtentionNone;
    if (XRRQueryExtension(display, &xreb, &ebr))
        extensions |= ExtentionXRandR;
    if (XineramaQueryExtension(display, &xreb, &ebr))
        extensions |= ExtentionXinerama;

    /* get the root window */
    root = RootWindow(display, DefaultScreen(display));

    /* setup the num lock modifier */
    modmap = XGetModifierMapping(display);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(display, XK_Num_Lock))
                numLockMask = (1 << i);
    XFreeModifiermap(modmap);

    /* initialize atoms */
    XInternAtoms(display, atomNames, AtomCount, False, atoms);

    /* initialize cursors */
    for (int i = 0; i < CursorCount; ++i)
        cursors[i] = XCreateFontCursor(display, cursorIds[i]);
}

void
CleanupX11()
{
    /* release x11 resources */
    for (int i = 0; i < CursorCount; ++i)
        cursors[i] = XFreeCursor(display, cursors[i]);

    XSync(display, False);
    XCloseDisplay(display);
}
