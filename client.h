#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "X11/XKBlib.h"
#include "X11/Xlib.h"
#include "stack.h"

/* window position and size offsets */
#define WXOffset(c) (c->decorated ? st_cfg.border_width : 0)
#define WYOffset(c) (c->decorated ? st_cfg.border_width + st_cfg.topbar_height : 0)
#define WWOffset(c) (c->decorated ? 2 * st_cfg.border_width : 0)
#define WHOffset(c) (c->decorated ? 2 * st_cfg.border_width + st_cfg.topbar_height: 0)

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
    // ButtonIconify, No iconify yet
    ButtonCount
};

typedef struct _Output Output;

typedef struct _Client {
    /* components */
    Window window;
    Window frame;
    Window topbar;
    Window buttons[ButtonCount];
    Window handles[HandleCount];

    /* geometries */
    int x, y, w, h;         /* current geometry                 */
    int mx, my, mw, mh;     /* pre maximization geometry        */
    //int hx, hy, hw, hh;   /* pre minimisation geometry        */
    int fx, fy, fw, fh;     /* pre fullscreen geometry          */
    int px, py, pw, ph;     /* pre pointer motion geometry      */
    int sbw;                /* saved border width               */

    /* state */
    Bool decorated;
    Bool fdecorated;        /* pre fullscreen decoration status */
    Bool active;
    Bool urgent;
    //Bool minimized;
    Bool vmaximixed;
    Bool hmaximixed;
    Bool fullscreen;

    /* ewmh */
    char *name;
    Bool focusable;
    Bool deletable;
    Bool fixed;
    char *class;
    char *instance;

    /* normal hints */
    int bw, bh;
    int incw, inch;
    int minw, minh;
    int maxw, maxh;
    float mina, maxa;

    /* strut */
    int top;
    int right;
    int left;
    int bottom;

    /* internal */
    int tags;
    int lx, ly;         /* last seen pointer                */
    Time lt;            /* last time for pointer motion     */
    Output *output;
    struct _Client *prev;
    struct _Client *next;
} Client;

Client *CreateClient(Window w);
void DestroyClient(Client *c);

void CloseClient(Client *c);
void MoveClient(Client *c, int x, int y);
void ResizeClient(Client *c, int w, int h, Bool sh);
void MoveResizeClient(Client *c, int x, int y, int w, int h, Bool sh);
void MaximizeClientHorizontally(Client *c);
void MaximizeClientVertically(Client *c);
void MaximizeClient(Client *c);
void MinimizeClient(Client *c);
//void FullscreenClient(Client *c); No need to be public
void RaiseClient(Client *c);
void LowerClient(Client *c);
void RestoreClient(Client *c);
void SetClientActive(Client *c, Bool b);
//void FocusClient(Client *c, Bool b);

void OnClientExpose(Client *c, XExposeEvent *e);
void OnClientEnter(Client *c, XCrossingEvent *e);
void OnClientLeave(Client *c, XCrossingEvent *e);
void OnClientConfigureRequest(Client *c, XConfigureRequestEvent *e);
void OnClientPropertyNotify(Client *c, XPropertyEvent *e);
//void OnClientFocusIn(Client *c, XFocusInEvent *e);
//void OnClientFocusOut(Client *c, XFocusOutEvent *e);
void OnClientButtonPress(Client *c, XButtonEvent *e);
void OnClientButtonRelease(Client *c, XButtonEvent *e);
void OnClientMotionNotify(Client *c, XMotionEvent *e);
void OnClientMessage(Client *c, XClientMessageEvent *e);


#endif
