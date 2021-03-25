#ifndef __STACK_H__
#define __STACK_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>

#include "config.h"

#define TagsCount 10

#define Max(v1, v2) (v1 > v2 ? v1 : v2)
#define Min(v1, v2) (v1 < v2 ? v1 : v2)
#define CleanMask(mask) (mask & ~(st_nlm|LockMask) &\
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

#ifdef NDEBUG
#define Modkey Mod4Mask
#else
#define Modkey Mod1Mask
#endif

enum AtomType {
    AtomWMDeleteWindow,
    AtomWMTakeFocus,
    AtomWMProtocols,
    AtomNetSupported,
    AtomNetClientList,
    AtomNetActiveWindow,
    AtomNetSupportingWMCheck,
    AtomNetWMName,
    AtomNetWMWindowType,
    AtomNetWMState,
    AtomNetWMPID,
    AtomNetWMWindowTypeDesktop,
    AtomNetWMWindowTypeDock,
    AtomNetWMWindowTypeToolbar,
    AtomNetWMWindowTypeMenu,
    AtomNetWMWindowTypeSplash,
    AtomNetWMWindowTypePopupMenu,
    AtomNetWMWindowTypeTooltip,
    AtomNetWMWindowTypeNotification,
    AtomNetWMStateMaximizedVert,
    AtomNetWMStateMaximizedHorz,
    AtomNetWMStateFullscreen,
    AtomNetWMStateAbove,
    AtomNetWMStateBelow,
    AtomNetWMStateDemandsAttention,
    AtomCount
};

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

extern Display*         st_dpy;                 /* display              */
extern int              st_scn;                 /* screen               */
extern Window           st_root;                /* root window          */
extern Atom             st_atm[AtomCount];      /* atoms                */
extern Cursor           st_cur[CursorCount];    /* cursors              */
extern Config           st_cfg;                 /* configuration        */
extern XftFont*         st_lft;                 /* labels font          */
extern XftFont*         st_ift;                 /* icons font           */
extern unsigned long    st_nlm;                 /* num lock mask        */
extern int              st_xeb;                 /* xrandr event base    */

void StartMainLoop();
void StopMainLoop();

void CycleForward();
void CycleBackward();

#endif
