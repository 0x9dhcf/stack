#ifndef __HINTS_H__
#define __HINTS_H__

#include <X11/Xlib.h>

#define IsFixed(n) (\
        n.minw != 0 &&\
        n.maxw != INT_MAX &&\
        n.minh != 0 &&\
        n.maxh != INT_MAX &&\
        n.minw == n.maxw &&\
        n.minh == n.maxh)

typedef enum WMHints WMHints;
typedef enum WMProtocols WMProtocols;
typedef enum NetWMWindowType NetWMWindowType;
typedef enum NetWMStates NetWMStates;
typedef enum NetWMActions NetWMActions;

typedef struct WMClass WMClass;
typedef struct WMNormals WMNormals;
typedef struct WMStrut WMStrut;
typedef struct MotifHints MotifHints;

enum WMHints{
    HintsFocusable              = (1<<0),
    HintsUrgent                 = (1<<1)
};

enum WMProtocols {
    NetWMProtocolTakeFocus      = (1<<0),
    NetWMProtocolDeleteWindow   = (1<<2)
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
    NetWMTypeActivable          = NetWMTypeNormal
                                | NetWMTypeDialog,
    NetWMTypeFixed              = NetWMTypeDesktop
                                | NetWMTypeDock
                                | NetWMTypeSplash,
    NetWMTypeNoTopbar           = NetWMTypeFixed
                                | NetWMTypeMenu
                                | NetWMTypeNotification
                                | NetWMTypeDropdownMenu
                                | NetWMTypePopupMenu
                                | NetWMTypeTooltip
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

enum NetWMActions {
    NetWMActionNone             = 0,
    NetWMActionMove             = (1<<0),
    NetWMActionResize           = (1<<2),
    NetWMActionMinimize         = (1<<3),
    NetWMActionShade            = (1<<4),
    NetWMActionStick            = (1<<5),
    NetWMActionMaximizeHorz     = (1<<6),
    NetWMActionMaximizeVert     = (1<<7),
    NetWMActionFullscreen       = (1<<8),
    NetWMActionChangeDesktop    = (1<<9),
    NetWMActionClose            = (1<<10),
    NetWMActionAbove            = (1<<11),
    NetWMActionBelow            = (1<<12),
    NetWMActionDefault          = NetWMActionMove
                                | NetWMActionResize
                                | NetWMActionMinimize
                                | NetWMActionMaximizeHorz
                                | NetWMActionMaximizeVert
                                | NetWMActionFullscreen
                                | NetWMActionChangeDesktop
                                | NetWMActionClose
                                | NetWMActionAbove
                                | NetWMActionBelow
};

struct WMClass {
    char *cname;
    char *iname;
};

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

struct MotifHints {
    long flags;
    long functions;
    long decorations;
    long input_mode;
    long state;
};

void GetWMName(Window w, char **name);
void GetWMHints(Window w, WMHints *h);
void GetWMProtocols(Window w, WMProtocols *h);
void GetWMNormals(Window w, WMNormals *h);
void GetWMClass(Window w, WMClass *klass);
void GetWMStrut(Window w, WMStrut *strut);
void GetNetWMWindowType(Window w, NetWMWindowType *h);
void GetNetWMStates(Window w, NetWMStates *h);
void SetNetWMAllowedActions(Window w, NetWMActions a);
void SetNetWMStates(Window w, NetWMStates h);
void GetMotifHints(Window w, MotifHints *h);
void SendMessage(Window w, Atom a);

#endif
