#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <X11/Xlib.h>

#include "config.h"

typedef struct Client Client;

typedef struct Desktop {
    int wx, wy, ww, wh; /* The working area geometry    */
    Bool dynamic;
    int masters;
    float split;
    Client *activeOnLeave;
} Desktop;

typedef struct Monitor {
    int x, y, w, h;     /* The geometry of the monitor  */

    Desktop desktops[DesktopCount];
    int activeDesktop;

    Client *chead;      /* Clients list head            */
    Client *ctail;      /* Clients list tail            */
    struct Monitor *next;
} Monitor;

extern Monitor *stMonitors;

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

#endif
