#ifndef __X11_H__
#define __X11_H__

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

typedef enum AtomType AtomType;
typedef enum CursorType CursorType;
typedef enum Extention Extension;

enum AtomType {
    /* icccm */
    AtomWMState,
    AtomWMChangeState,
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
    AtomNetCloseWindow,
    AtomNetMoveresizeWindow,        /* unused */
    AtomNetWMMoveresize,
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
    CursorResizeNorthEast,
    CursorResizeNorth,
    CursorResizeNorthWest,
    CursorResizeWest,
    CursorResizeSouthWest,
    CursorResizeSouth,
    CursorResizeSouthEast,
    CursorResizeEast,
    CursorCount
};

enum Extention {
    ExtentionNone       = 0,
    ExtentionXRandR     = (1 << 1),
    ExtentionXinerama   = (1 << 2)
};

extern Display *display;
extern int extensions;
extern Window root;
extern unsigned long numLockMask;
extern Atom atoms[AtomCount];
extern Cursor cursors[CursorCount];

void SetupX11();
void CleanupX11();

#endif /* __X11_H__ */
