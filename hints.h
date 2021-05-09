#ifndef __HINTS_H__
#define __HINTS_H__

#include <X11/Xlib.h>

typedef enum _WMHints {
    HintsFocusable      = (1<<0),
    HintsUrgent         = (1<<1)
} WMHints;

typedef enum _WMProtocols {
    NetWMProtocolTakeFocus      = 1<<0,
    NetWMProtocolDeleteWindow   = 1<<2
} WMProtocols;

typedef enum _NetWMWindowType {
    NetWMTypeNone           = 0,
    NetWMTypeNormal         = (1<<0),
    NetWMTypeDialog         = (1<<1),
    NetWMTypeDesktop        = (1<<2),
    NetWMTypeDock           = (1<<3),
    NetWMTypeMenu           = (1<<4),
    NetWMTypeNotification   = (1<<5),
    NetWMTypeSplash         = (1<<6),
    NetWMTypeToolbar        = (1<<7),
    NetWMTypeUtility        = (1<<8),
    NetWMTypeCombo          = (1<<9),
    NetWMTypeDnd            = (1<<10),
    NetWMTypeDropdownMenu   = (1<<11),
    NetWMTypePopupMenu      = (1<<12),
    NetWMTypeTooltip        = (1<<13),
    NetWMTypeFixed          = NetWMTypeDesktop
                            | NetWMTypeDock
                            | NetWMTypeSplash
} NetWMWindowType;

typedef enum _NetWMState {
    NetWMStateNone             = 0,
    NetWMStateModal            = (1<<0),
    NetWMStateSticky           = (1<<1),
    NetWMStateMaximizedVert    = (1<<2),
    NetWMStateMaximizedHorz    = (1<<3),
    NetWMStateShaded           = (1<<4),
    NetWMStateSkipTaskbar      = (1<<5),
    NetWMStateSkipPager        = (1<<6),
    NetWMStateHidden           = (1<<7),
    NetWMStateFullscreen       = (1<<8),
    NetWMStateAbove            = (1<<9),
    NetWMStateBelow            = (1<<10),
    NetWMStateDemandsAttention = (1<<11),
    NetWMStateMaximized        = NetWMStateMaximizedHorz
                               | NetWMStateMaximizedVert
} NetWMState;

typedef enum _NetWMAllowedActions {
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

typedef struct _WMClass {
    char *cname;
    char *iname;
} WMClass;

typedef struct _WMNormals {
    int bw, bh;
    int incw, inch;
    int minw, minh;
    int maxw, maxh;
    float mina, maxa;
} WMNormals;

typedef struct _WMStrut {
    int top;
    int right;
    int left;
    int bottom;
} WMStrut;

void GetWMName(Window w, char **name);

void GetWMHints(Window w, WMHints *h);

void GetWMProtocols(Window w, WMProtocols *h);

void GetWMNormals(Window w, WMNormals *h);

void GetWMClass(Window w, WMClass *klass);

void GetWMStrut(Window w, WMStrut *strut);

void GetNetWMWindowType(Window w, NetWMWindowType *h);

void GetNetWMState(Window w, NetWMState *h);

void SetNetWMState(Window w, NetWMState h);

void SendMessage(Window w, Atom a);

#endif
