#ifndef __STACK_H__
#define __STACK_H__

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#ifndef VERSION
    #define VERSION "0.0.0"
#endif

#ifdef NDEBUG
#define Modkey Mod4Mask
#else
#define Modkey Mod1Mask
#endif

#define Max(v1, v2) (v1 > v2 ? v1 : v2)
#define Min(v1, v2) (v1 < v2 ? v1 : v2)

#define FLog(fmt, ...) do {\
    fprintf (stderr, "FATAL - [%s: %d]: " fmt "\n", __FILE__,__LINE__, ##__VA_ARGS__);\
    exit (EXIT_FAILURE);\
} while(0)

#define ELog(fmt, ...) do {\
    fprintf (stderr, "ERROR - [%s: %d]: " fmt "\n", __FILE__,__LINE__, ##__VA_ARGS__);\
} while (0)

#define ILog(fmt, ...) do {\
    fprintf (stdout, "INFO: " fmt "\n", ##__VA_ARGS__);\
} while (0)

#ifndef NDEBUG
#define DLog(fmt, ...) do {\
    fprintf (stdout, "DEBUG - [%s: %d - %s]: " fmt "\n", __FILE__,__LINE__,__FUNCTION__, ##__VA_ARGS__);\
    fflush(stdout);\
} while (0)
#else
#define DLog(fmt, ...)
#endif

#define CleanMask(mask)\
    (mask & ~(stNumLockMask|LockMask) &\
         ( ShiftMask\
         | ControlMask\
         | Mod1Mask\
         | Mod2Mask\
         | Mod3Mask\
         | Mod4Mask\
         | Mod5Mask))

/* XXX: find another way!!! */
#define ShortcutCount 32

#define DesktopCount 8

#define RootEventMask (\
          SubstructureRedirectMask\
        | SubstructureNotifyMask)\

#define FrameEvenMask (\
          ExposureMask\
        | ButtonPressMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define WindowEventMask (\
          PropertyChangeMask)

#define WindowDoNotPropagateMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask\
        | KeyPressMask\
        | KeyReleaseMask)

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)


typedef enum HAlign HAlign;
typedef enum VAlign VAlign;
typedef enum WMHints WMHints;
typedef enum WMProtocols WMProtocols;
typedef enum NetWMWindowType NetWMWindowType;
typedef enum NetWMStates NetWMStates;
typedef struct WMClass WMClass;
typedef struct WMNormals WMNormals;
typedef struct WMStrut WMStrut;
typedef struct Desktop Desktop;
typedef struct Monitor Monitor;
typedef struct Transient Transient;
typedef struct Client Client;
typedef struct Shortcut Shortcut;
typedef struct Config Config;


/*
 * Atoms
 */
enum AtomType {
    /* icccm */
    AtomWMDeleteWindow,
    AtomWMTakeFocus,
    AtomWMProtocols,

    /* ewmh */
    AtomNetSupported,
    AtomNetClientList,
    AtomNetClientListStacking,
    AtomNetNumberOfDesktops,
    AtomNetDesktopGeometry,
    AtomNetDesktopViewport,
    AtomNetCurrentDesktop,
    AtomNetDesktopNames,
    AtomNetActiveWindow,
    AtomNetWorkarea,
    AtomNetSupportingWMCheck,
    AtomNetVirtualRoots,
    AtomNetDesktopLayout,
    AtomNetShowingDesktop,
    AtomNetCloseWindow,
    AtomNetMoveresizeWindow,
    AtomNetWMMoveresize,
    AtomNetRestackWindow,
    AtomNetRequestFrameExtents,
    AtomNetWMName,
    AtomNetWMVisibleName,
    AtomNetWMIconName,
    AtomNetWMVisibleIconName,
    AtomNetWMDesktop,
    AtomNetWMWindowType,
    AtomNetWMState,
    AtomNetWMAllowedActions,
    AtomNetWMStrut,
    AtomNetWMStrutpartial,
    AtomNetWMIconGeometry,
    AtomNetWMIcon,
    AtomNetWMPID,
    AtomNetWMHandledIcons,
    AtomNetWMUserTime,
    AtomNetWMUserTimeWindow,
    AtomNetFrameExtents,
    AtomNetWMPing,
    AtomNetWMSyncRequest,
    AtomNetWMSyncRequestCounter,
    AtomNetWMFullscreenMonitors,
    AtomNetWMFullPlacement,
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

/* 
 * cursor types
 */
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

/* 
 * Fonts 
 */
enum HAlign {
    AlignLeft,
    AlignCenter,
    AlignRight
};

enum VAlign {
    AlignTop,
    AlignMiddle,
    AlignBottom
};

/*
 * ICCCM/EWMH hints
 */
enum WMHints {
    HintsFocusable              = (1<<0),
    HintsUrgent                 = (1<<1)
};

enum WMProtocols {
    NetWMProtocolTakeFocus      = 1<<0,
    NetWMProtocolDeleteWindow   = 1<<2
};

enum NetWMWindowType {
    NetWMTypeNone               = 0,
    NetWMTypeNormal             = (1<<0),
    NetWMTypeDialog             = (1<<1),
    NetWMTypeDesktop            = (1<<2),
    NetWMTypeDock               = (1<<3),
    NetWMTypeMenu               = (1<<4),
    NetWMTypeNotification       = (1<<5),
    NetWMTypeSplash             = (1<<6),
    NetWMTypeToolbar            = (1<<7),
    NetWMTypeUtility            = (1<<8),
    NetWMTypeCombo              = (1<<9),
    NetWMTypeDnd                = (1<<10),
    NetWMTypeDropdownMenu       = (1<<11),
    NetWMTypePopupMenu          = (1<<12),
    NetWMTypeTooltip            = (1<<13),
    NetWMTypeFixed              = NetWMTypeDesktop
                                | NetWMTypeDock
                                | NetWMTypeSplash,
    NetWMTypeAny                = NetWMTypeNormal
                                | NetWMTypeDialog
                                | NetWMTypeDesktop
                                | NetWMTypeDock
                                | NetWMTypeMenu
                                | NetWMTypeNotification
                                | NetWMTypeSplash
                                | NetWMTypeToolbar
                                | NetWMTypeUtility
                                | NetWMTypeCombo
                                | NetWMTypeDnd
                                | NetWMTypeDropdownMenu
                                | NetWMTypePopupMenu
                                | NetWMTypeTooltip
};

enum NetWMStates {
    NetWMStateNone              = 0,
    NetWMStateModal             = (1<<0),
    NetWMStateSticky            = (1<<1),
    NetWMStateMaximizedVert     = (1<<2),
    NetWMStateMaximizedHorz     = (1<<3),
    NetWMStateShaded            = (1<<4),
    NetWMStateSkipTaskbar       = (1<<5),
    NetWMStateSkipPager         = (1<<6),
    NetWMStateHidden            = (1<<7),
    NetWMStateFullscreen        = (1<<8),
    NetWMStateAbove             = (1<<9),
    NetWMStateBelow             = (1<<10),
    NetWMStateDemandsAttention  = (1<<11),
    NetWMStateMaximized         = NetWMStateMaximizedHorz
                                | NetWMStateMaximizedVert,
    NetWMStateAny               = NetWMStateModal
                                | NetWMStateSticky
                                | NetWMStateMaximizedVert
                                | NetWMStateMaximizedHorz
                                | NetWMStateShaded
                                | NetWMStateSkipTaskbar
                                | NetWMStateSkipPager
                                | NetWMStateHidden
                                | NetWMStateFullscreen
                                | NetWMStateAbove
                                | NetWMStateBelow
                                | NetWMStateDemandsAttention
};

/*
typedef enum {
    NetWMActionNone          = 0,
    NetWMActionMove          = (1<<0),
    NetWMActionResize        = (1<<1),
    NetWMActionMinimize      = (1<<2),
    NetWMActionShade         = (1<<3),
    NetWMActionStick         = (1<<4),
    NetWMActionMaximizeHorz  = (1<<5),
    NetWMActionMaximizeVert  = (1<<6),
    NetWMActionFullscreen    = (1<<7),
    NetWMActionChangeDesktop = (1<<8),
    NetWMActionClose         = (1<<9),
    NetWMActionAll           = NetWMActionMove
                             | NetWMActionResize
                             | NetWMActionMinimize
                             | NetWMActionStick
                             | NetWMActionMaximizeHorz
                             | NetWMActionMaximizeVert
                             | NetWMActionFullscreen
                             | NetWMActionClose
} NetWMAllowedActions;
*/

struct WMClass {
    char *cname;
    char *iname;
};

// XXX MOVE!
#define IsFixed(n) (\
        n.minw != 0 &&\
        n.maxw != INT_MAX &&\
        n.minh != 0 &&\
        n.maxh != INT_MAX &&\
        n.minw == n.maxw &&\
        n.minh == n.maxh)

struct WMNormals {
    int bw, bh;
    int incw, inch;
    int minw, minh;
    int maxw, maxh;
    float mina, maxa;
};

struct WMStrut {
    int top;
    int right;
    int left;
    int bottom;
};

/*
 * monitor
 */

struct Desktop {
    int wx, wy, ww, wh; /* the working area geometry    */
    Bool dynamic;
    int masters;
    float split;
    Client *activeOnLeave;
};

struct Monitor {
    int x, y, w, h;     /* the geometry of the monitor  */

    Desktop desktops[DesktopCount];
    int activeDesktop;

    Client *chead;      /* clients list head            */
    Client *ctail;      /* clients list tail            */
    struct Monitor *next;
};

/*
 * client
 */
enum Handles {
    HandleNorth,
    HandleNorthWest,
    HandleWest,
    HandleSouthWest,
    HandleSouth,
    HandleSouthEast,
    HandleEast,
    HandleNorthEast,
    HandleCount
};

enum Buttons {
    ButtonClose,
    ButtonMaximize,
    ButtonMinimize,
    ButtonCount
};

struct Transient {
    Client *client;
    struct Transient *next;
};

struct Client {
    /* components */
    Window window;
    Window frame;
    Window topbar;
    Window buttons[ButtonCount];
    Window handles[HandleCount];

    /* geometries */
    int wx, wy, ww, wh;     /* window absolute geometry                 */
    int fx, fy, fw, fh;     /* frame absolute geometry                  */
    int sfx, sfy, sfw, sfh; /* saved frame geometry post fullscreen     */
    int smx, smy, smw, smh; /* saved frame geometry post max/minimixed  */
    int stx, sty, stw, sth; /* saved frame geometry post tiling         */
    int sbw;                /* saved border width                       */

    /* statuses */
    Bool active;
    Bool decorated;
    Bool tiled;
    int desktop;

    /* icccm, ewmh*/
    char *name;
    WMClass wmclass;
    WMNormals normals;
    WMStrut strut;
    WMProtocols protocols;
    WMHints hints;
    NetWMWindowType types;
    NetWMStates states;
    Client *transfor;
    Transient *transients;

    /* internals */
    Monitor *monitor;

    Client *prev;
    Client *next;
};

/*
 * config
 */
struct Shortcut {
    unsigned long modifier;
    unsigned long keysym;
    enum {CV, CI, CC} type;
    union {
        struct { void (*f)(); } vcb;
        struct { void (*f)(int); int i; } icb;
        struct { void (*f)(Client *); } ccb;
    } cb;
};

struct Config {
    char *labelFontname;
    char *iconFontname;
    int borderWidth;
    int topbarHeight;
    int handleWidth;
    int buttonSize;
    int buttonGap;
    int activeBackground;
    int activeForeground;
    int inactiveBackground;
    int inactiveForeground;
    int urgentBackground;
    int urgentForeground;
    struct {
        char *icon;
        int activeBackground;
        int activeForeground;
        int inactiveBackground;
        int inactiveForeground;
        int activeHoveredBackground;
        int activeHoveredForeground;
        int inactiveHoveredBackground;
        int inactiveHoveredForeground;
    } buttonStyles[ButtonCount];
    int masters;
    float split;
    Shortcut shortcuts[ShortcutCount];
};

extern Display         *stDisplay;
extern int              stScreen;
extern Window           stRoot;
extern unsigned long    stNumLockMask;
extern int              stXRandREventBase;
extern Atom             stAtoms[AtomCount];
extern Cursor           stCursors[CursorCount];
extern Config           stConfig;
extern XftFont         *stLabelFont;
extern XftFont         *stIconFont;
extern Monitor         *stMonitors;
extern Monitor         *stActiveMonitor;
extern Client          *stActiveClient;
extern Bool             stRunning;

void InitializeX11();
void TeardownX11();

void InitializeAtoms();

void InitializeCursors();
void TeardownCursors();

void InitializeFonts();
void TeardownFonts();
void GetTextPosition(const char *s, XftFont *ft, HAlign ha, VAlign va, int w, int h, int *x, int *y);
void WriteText(Drawable d, const char*s, XftFont *ft, int color, int x, int y); 

void GetWMName(Window w, char **name);
void GetWMHints(Window w, WMHints *h);
void GetWMProtocols(Window w, WMProtocols *h);
void GetWMNormals(Window w, WMNormals *h);
void GetWMClass(Window w, WMClass *klass);
void GetWMStrut(Window w, WMStrut *strut);
void GetNetWMWindowType(Window w, NetWMWindowType *h);
void GetNetWMStates(Window w, NetWMStates *h);
void SetNetWMStates(Window w, NetWMStates h);
void SendMessage(Window w, Atom a);

void InitializeMonitors();
void TeardownMonitors();
void AttachClientToMonitor(Monitor *m, Client *c);
void DetachClientFromMonitor(Monitor *m, Client *c);
Client *NextClient(Monitor *m, Client *c);
Client *PreviousClient(Monitor *m, Client *c);
void MoveClientAfter(Monitor *m, Client *c, Client *after);
void MoveClientBefore(Monitor *m, Client *c, Client *before);
void AddClientToDesktop(Monitor *m, Client *c, int d);
void RemoveClientFromDesktop(Monitor *m, Client *c, int d);
void SetActiveDesktop(Monitor *m, int desktop);
void Restack(Monitor *m);

void HideClient(Client *c);
void ShowClient(Client *c);
void MoveClientWindow(Client *c, int x, int y);
void ResizeClientWindow(Client *c, int w, int h, Bool sh);
void MoveResizeClientWindow(Client *c, int x, int y, int w, int h, Bool sh);
void MoveClientFrame(Client *c, int x, int y);
void ResizeClientFrame(Client *c, int w, int h, Bool sh);
void MoveResizeClientFrame(Client *c, int x, int y, int w, int h, Bool sh);
void TileClient(Client *c, int x, int y, int w, int h);
void MaximizeClientHorizontally(Client *c);
void MaximizeClientVertically(Client *c);
void MaximizeClient(Client *c);
void MaximizeClientLeft(Client *c);
void MaximizeClientRight(Client *c);
void MaximizeClientTop(Client *c);
void MaximizeClientBottom(Client *c);
void MinimizeClient(Client *c);
void FullscreenClient(Client *c);
void RestoreClient(Client *c);
void RaiseClient(Client *c);
void LowerClient(Client *c);
void RefreshClientButton(Client *c, int button, Bool hovered);
void RefreshClient(Client *c);
void SetClientActive(Client *c, Bool b);

void DispatchEvent(XEvent *e);

void ManageWindow(Window w, Bool exists);
Client *Lookup(Window w);
void ForgetWindow(Window w, Bool survives);
void SetActiveClient(Client *c);
void FindNextActiveClient();
void Quit();
void ActivateNext();
void ActivatePrev();
void MoveForward();
void MoveBackward();
void ShowDesktop(int desktop);
void MoveToDesktop(int desktop);
void ToggleDynamic();
void AddMaster(int nb);

#endif
