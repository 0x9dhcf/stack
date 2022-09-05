#ifndef __X11_H__
#define __X11_H__

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

enum AtomType {
    /* icccm */
    AtomWMState,
    AtomWMDeleteWindow,
    AtomWMTakeFocus,
    AtomWMProtocols,

    /* motif */
    AtomMotifWMHints,

    /* ewmh */
    AtomNetSupported,
    AtomNetClientList,
    AtomNetClientListStacking,      /* unused */
    AtomNetNumberOfDesktops,
    AtomNetDesktopGeometry,         /* unused */
    AtomNetDesktopViewport,         /* unused */
    AtomNetCurrentDesktop,
    AtomNetDesktopNames,            /* unused */
    AtomNetActiveWindow,
    AtomNetWorkarea,                /* unused */
    AtomNetSupportingWMCheck,
    AtomNetVirtualRoots,            /* unused */
    AtomNetDesktopLayout,           /* unused */
    AtomNetShowingDesktop,          /* unused */
    AtomNetCloseWindow,             /* unused */
    AtomNetMoveresizeWindow,        /* unused */
    AtomNetWMMoveresize,            /* unused */
    AtomNetRestackWindow,           /* unused */
    AtomNetRequestFrameExtents,     /* unused */
    AtomNetWMName,
    AtomNetWMVisibleName,           /* unused */
    AtomNetWMIconName,              /* unused */
    AtomNetWMVisibleIconName,       /* unused */
    AtomNetWMDesktop,               /* unused */
    AtomNetWMWindowType,
    AtomNetWMState,
    AtomNetWMAllowedActions,
    AtomNetWMStrut,                 /* unused */
    AtomNetWMStrutpartial,
    AtomNetWMIconGeometry,          /* unused */
    AtomNetWMIcon,                  /* unused */
    AtomNetWMPID,
    AtomNetWMHandledIcons,          /* unused */
    AtomNetWMUserTime,              /* unused */
    AtomNetWMUserTimeWindow,        /* unused */
    AtomNetFrameExtents,            /* unused */
    AtomNetWMPing,                  /* unused */
    AtomNetWMSyncRequest,           /* unused */
    AtomNetWMSyncRequestCounter,    /* unused */
    AtomNetWMFullscreenMonitors,    /* unused */
    AtomNetWMFullPlacement,         /* unused */
    AtomNetWMWindowTypeDesktop,
    AtomNetWMWindowTypeDock,
    AtomNetWMWindowTypeToolbar,
    AtomNetWMWindowTypeMenu,
    AtomNetWMWindowTypeUtility,
    AtomNetWMWindowTypeSplash,
    AtomNetWMWindowTypeDialog,
    AtomNetWMWindowTypeDropdownMenu,
    AtomNetWMWindowTypePopupMenu,
    AtomNetWMWindowTypeTooltip,
    AtomNetWMWindowTypeNotification,
    AtomNetWMWindowTypeCombo,
    AtomNetWMWindowTypeDnd,
    AtomNetWMWindowTypeNormal,
    AtomNetWMStateModal,
    AtomNetWMStateSticky,
    AtomNetWMStateMaximizedVert,
    AtomNetWMStateMaximizedHorz,
    AtomNetWMStateShaded,
    AtomNetWMStateSkipTaskbar,
    AtomNetWMStateSkipPager,
    AtomNetWMStateHidden,
    AtomNetWMStateFullscreen,
    AtomNetWMStateAbove,
    AtomNetWMStateBelow,
    AtomNetWMStateDemandsAttention,
    AtomNetWMActionMove,
    AtomNetWMActionResize,
    AtomNetWMActionMinimize,
    AtomNetWMActionShade,
    AtomNetWMActionStick,
    AtomNetWMActionMaximizeHorz,
    AtomNetWMActionMaximizeVert,
    AtomNetWMActionFullscreen,
    AtomNetWMActionChangeDesktop,
    AtomNetWMActionClose,
    AtomNetWMActionAbove,
    AtomNetWMActionBelow,
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

typedef enum {
    AlignLeft,
    AlignCenter,
    AlignRight
} HAlign;

typedef enum {
    AlignTop,
    AlignMiddle,
    AlignBottom
} VAlign;

extern Display *stDisplay;
extern int stScreen;
extern Window stRoot;
extern unsigned long stNumLockMask;
extern int stXRandREventBase;
extern Atom stAtoms[AtomCount];
extern Cursor stCursors[CursorCount];
extern XftFont *stLabelFont;
extern XftFont *stIconFont;
extern XErrorHandler stDefaultErrorHandler;

void InitializeX11();
void GetTextPosition(const char *s, XftFont *ft, HAlign ha, VAlign va, int w, int h, int *x, int *y);
void WriteText(Drawable d, const char*s, XftFont *ft, int color, int x, int y);
void TeardownX11();
int EventLoopErrorHandler(Display *d, XErrorEvent *e);
int DummyErrorHandler(Display *d, XErrorEvent *e);
int WMDetectedErrorHandler(Display *d, XErrorEvent *e);

#endif
