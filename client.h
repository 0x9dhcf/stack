#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <X11/Xlib.h>

#include "config.h"
#include "hints.h"
#include "stack.h"

typedef struct _Monitor Monitor;

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

enum State {
    StateNone                       = 0,
    StateAcceptFocus                = (1<<0),
    StateUrgent                     = (1<<1),
    StateTakeFocus                  = (1<<2),
    StateDeleteWindow               = (1<<3),
    StateDecorated                  = (1<<4),
    StateFixed                      = (1<<5),
    StateActive                     = (1<<6),
    StateMaximizedVertically        = (1<<7),
    StateMaximizedHorizontally      = (1<<8),
    StateMaximizedLeft              = (1<<9),
    StateMaximizedTop               = (1<<10),
    StateMaximizedRight             = (1<<11),
    StateMaximizedBottom            = (1<<12),
    StateMinimized                  = (1<<13),
    StateFullscreen                 = (1<<14),
    StateAbove                      = (1<<15),
    StateBelow                      = (1<<16),
    StateSticky                     = (1<<17),
    StateMaximized                  = (StateMaximizedVertically
                                    |  StateMaximizedHorizontally),
    StateMaximizedAny               = (StateMaximizedVertically
                                    |  StateMaximizedHorizontally
                                    |  StateMaximizedLeft
                                    |  StateMaximizedTop
                                    |  StateMaximizedRight
                                    |  StateMaximizedBottom)
};

typedef struct _Client {
    /* components */
    Window window;
    Window frame;
    Window topbar;
    Window buttons[ButtonCount];
    Window handles[HandleCount];

    /* geometries */
    int wx, wy, ww, wh;     /* window absolute geometry */
    int fx, fy, fw, fh;     /* frame absolute geometry  */
    int swx, swy, sww, swh; /* saved window geoemtry    */
    int sfx, sfy, sfw, sfh; /* saved frame geometry     */
    int sbw;                /* saved border width       */

    /* state */
    int states;

    char *name;             /* ewmh or icccm name       */
    WMClass wmclass;        /* icccm class              */
    WMNormals normals;      /* size hints               */
    WMStrut strut;          /* strut                    */

    Monitor *monitor;

    struct _Client *prev;
    struct _Client *next;
} Client;

//void CloseClient(Client *c);

Client *CreateClient(Window w);

void MoveClientWindow(Client *c, int x, int y);
void ResizeClientWindow(Client *c, int w, int h, Bool sh);
void MoveResizeClientWindow(Client *c, int x, int y, int w, int h, Bool sh);

void MoveClientFrame(Client *c, int x, int y);
void ResizeClientFrame(Client *c, int w, int h, Bool sh);
void MoveResizeClientFrame(Client *c, int x, int y, int w, int h, Bool sh);

void MaximizeClient(Client *c, int flag, Bool user);
void MinimizeClient(Client *c, Bool user);
void FullscreenClient(Client *c, Bool user);
void RestoreClient(Client *c, Bool user);
void RaiseClient(Client *c, Bool user);
void LowerClient(Client *c, Bool user);

void RefreshClient(Client *c);

void UpdateClientName(Client *c);
void UpdateClientHints(Client *c);
void UpdateClientState(Client *c);

void SetClientActive(Client *c, Bool b);

void NotifyClient(Client *c);
//void SendClientMessage(Client *c, Atom proto);

#endif
