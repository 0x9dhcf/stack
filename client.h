#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <X11/Xlib.h>

#include "hints.h"

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

typedef struct _Transient {
    struct _Client *client;
    struct _Transient *next;
} Transient;

typedef struct _Client {
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

    /* properties */
    //Bool activable;
    //Bool moveable;
    //Bool resizable;
    //Bool decorable;
    //Bool fixed;

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
    NetWMState states;
    struct _Client *transfor;
    Transient *transients;

    /* internals */
    Monitor *monitor;

    struct _Client *prev;
    struct _Client *next;
} Client;


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

#endif
