#ifndef __STACK_H__
#define __STACK_H__

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#ifndef VERSION
    #define VERSION "0.0.0"
#endif

// TODO: make it configurable
#define Modkey Mod1Mask
#define ModkeySym XK_Alt_L

#define Max(v1, v2) (v1 > v2 ? v1 : v2)
#define Min(v1, v2) (v1 < v2 ? v1 : v2)

#define FLog(fmt, ...) do {\
    fprintf (stderr, "FATAL - [%s: %d]: " fmt "\n",\
            __FILE__,__LINE__, ##__VA_ARGS__);\
    exit (EXIT_FAILURE);\
} while(0)

#define ELog(fmt, ...) do {\
    fprintf (stderr, "ERROR - [%s: %d]: " fmt "\n",\
            __FILE__,__LINE__, ##__VA_ARGS__);\
} while (0)

#define ILog(fmt, ...) do {\
    fprintf (stdout, "INFO: " fmt "\n", ##__VA_ARGS__);\
} while (0)

#ifndef NDEBUG
#define DLog(fmt, ...) do {\
    fprintf (stdout, "DEBUG - [%s: %d - %s]: " fmt "\n",\
            __FILE__,__LINE__,__FUNCTION__, ##__VA_ARGS__);\
    fflush(stdout);\
} while (0)
#else
#define DLog(fmt, ...)
#endif

typedef struct Monitor Monitor;
typedef struct Client Client;

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
    FontLabel,
    FontIcon,
    FontCount
} FontType;


#define ShortcutCount 39
typedef struct Config {
    char labelFontname[128];
    char iconFontname[128];
    int borderWidth;
    int topbarHeight;
    int handleWidth;
    int buttonSize;
    int buttonGap;
    int activeTileBackground;
    int inactiveTileBackground;
    int activeBackground;
    int activeForeground;
    int inactiveBackground;
    int inactiveForeground;
    int urgentBackground;
    int urgentForeground;
    struct {
        char icon[8];
        int activeBackground;
        int activeForeground;
        int inactiveBackground;
        int inactiveForeground;
        int activeHoveredBackground;
        int activeHoveredForeground;
        int inactiveHoveredBackground;
        int inactiveHoveredForeground;
    } buttonStyles[3];
    int decorateTiles;
    int masters;
    float split;
    struct {
        unsigned long modifier;
        unsigned long keysym;
        enum {CV, CI, CC} type;
        union {
            struct { void (*f)(); } vcb;            /* Void callback        */
            struct { void (*f)(int); int i; } icb;  /* Integer callback     */
            struct { void (*f)(Client *); } ccb;    /* Client callback      */
        } cb;
    } shortcuts[ShortcutCount];
} Config;

extern Display *display;
extern Window root;
extern unsigned long numLockMask;
extern Atom atoms[AtomCount];
extern Cursor cursors[CursorCount];
extern XftFont *fonts[FontCount];
extern Monitor *monitors;
extern Config config;

void FindConfigFile();
void FindAutostartFile();
void LoadConfigFile();
void ExecAutostartFile();

#endif
