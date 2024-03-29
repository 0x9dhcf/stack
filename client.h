#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <X11/Xlib.h>

#include "hints.h"

typedef enum Handles Handles;
typedef enum Buttons Buttons;
typedef enum NetWMActions NetWMActions;

typedef struct Client Client;
typedef struct Transient Transient;
typedef struct Monitor Monitor;

enum Handles {
    HandleNorthEast,
    HandleNorth,
    HandleNorthWest,
    HandleWest,
    HandleSouthWest,
    HandleSouth,
    HandleSouthEast,
    HandleEast,
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
    Transient *next;
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
    Bool hasTopbar;
    Bool hasHandles;
    Bool isBorderVisible;
    Bool isTopbarVisible;
    Bool isFocused;
    Bool isTiled;
    Bool isVisible;
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
    int desktop;
    int hovered;
    Client *next;
    Client *snext;
    Client *sprev;
};

extern Client *clients; 

void KillClient(Client *c);

void HideClient(Client *c);
void ShowClient(Client *c);
Bool IsClientFocusable(Client *c);
void FocusClient(Client *c, Bool b);
void RefreshClient(Client *c);
void SetClientTopbarVisible(Client *c, Bool b);
void ToggleClientTopbar(Client *c);

void SaveGeometries(Client *c);
void SynchronizeFrameGeometry(Client *c);
void SynchronizeWindowGeometry(Client *c);

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
void SnapClientLeft(Client *c);
void SnapClientRight(Client *c);
void SnapClientTop(Client *c);
void SnapClientBottom(Client *c);
void CenterClient(Client *c);
void FullscreenClient(Client *c);
void RestoreClient(Client *c);

void RaiseClient(Client *c);
void LowerClient(Client *c);

void MoveClientToDesktop(Client *c, int desktop);
void MoveClientToNextDesktop(Client *c);
void MoveClientToPreviousDesktop(Client *c);

void MoveClientToMonitor(Client *c, Monitor *m);
void MoveClientToNextMonitor(Client *c);
void MoveClientToPreviousMonitor(Client *c);

#endif /* __CLIENT_H__ */
