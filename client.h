#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "hints.h"

typedef struct Monitor Monitor;
typedef struct Transient Transient;
typedef struct Client Client;

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
    Window window;
    Window frame;
    Window topbar;
    Window buttons[ButtonCount];
    Window handles[HandleCount];
    int wx, wy, ww, wh;     /* Window absolute geometry                 */
    int fx, fy, fw, fh;     /* Frame absolute geometry                  */
    int sfx, sfy, sfw, sfh; /* Saved frame geometry ante fullscreen     */
    int smx, smy, smw, smh; /* Saved frame geometry ante max/minimixed  */
    int shx, shy, shw, shh; /* saved frame geometry ante minimixed      */
    int stx, sty, stw, sth; /* Saved frame geometry ante tiling         */
    int sbw;                /* Saved border width                       */
    Bool active;
    Bool decorated;
    Bool tiled;
    int desktop;
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
    MotifHints motifs;
    Monitor *monitor;
    Client *prev;
    Client *next;
};

void HideClient(Client *c);
void ShowClient(Client *c);
void MoveClientWindow(Client *c, int x, int y);
void ResizeClientWindow(Client *c, int w, int h, Bool sh);
void MoveResizeClientWindow(Client *c, int x, int y, int w, int h, Bool sh);
void MoveClientFrame(Client *c, int x, int y);
void ResizeClientFrame(Client *c, int w, int h, Bool sh);
void MoveResizeClientFrame(Client *c, int x, int y, int w, int h, Bool sh);
void TileClient(Client *c, int x, int y, int w, int h);
void UntileClient(Client *c);
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
void KillClient(Client *c);

#endif
