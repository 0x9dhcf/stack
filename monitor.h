#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <X11/Xlib.h>

#include "hints.h"

#define DesktopCount 10

typedef struct _Client Client;

typedef struct _Desktop {
    int wx, wy, ww, wh; /* the working area geometry    */
    Client *activeOnLeave;
} Desktop;

typedef struct _Monitor {
    int x, y, w, h;     /* the geometry of the monitor  */

    Desktop desktops[DesktopCount];
    int activeDesktop;

    Client *chead;      /* clients list head            */
    Client *ctail;      /* clients list tail            */
    struct _Monitor *next;
} Monitor;

/* head of the the monitors list */
extern Monitor *stMonitors;

void InitializeMonitors();
void TeardownMonitors();

void AttachClientToMonitor(Monitor *m, Client *c);
void DetachClientFromMonitor(Monitor *m, Client *c);
void AddClientToDesktop(Monitor *m, Client *c, int d);
void RemoveClientFromDesktop(Monitor *m, Client *c, int d);
void SetActiveDesktop(Monitor *m, int desktop);
void Restack(Monitor *m);

#endif
