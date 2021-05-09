#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <X11/Xlib.h>

#define DesktopCount 10

typedef struct _Client Client;

typedef struct _Desktop {
    int wx, wy, ww, wh; /* the working area geometry    */
} Desktop;

typedef struct _Monitor {
    int x, y, w, h;     /* the geometry of the monitor  */

    Desktop desktops[DesktopCount];
    int activeDesktop;

    Client *chead;      /* clients list head            */
    Client *ctail;      /* clients list tail            */
    struct _Monitor *next;
} Monitor;

/* head of the list of monitors */
extern Monitor *stMonitors;

void InitializeMonitors();
void TeardownMonitors();

void AttachClientToMonitor(Monitor *m, Client *c);
void DetachClientFromMonitor(Monitor *m, Client *c);
void SetActiveDesktop(Monitor *m, int desktop);
void Restack(Monitor *m);

Client* LookupMonitorClient(Monitor *m, Window w);
Client* NextClient(Monitor *m, Client *c);
Client* PreviousClient(Monitor *m, Client *c);

#endif
