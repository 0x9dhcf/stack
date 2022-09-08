#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <X11/Xlib.h>

#define DesktopCount 8

typedef struct Client Client;

typedef struct Desktop {
    int wx, wy, ww, wh;
    Bool dynamic;
    int masters;
    float split;
    Client *activeOnLeave;
} Desktop;

typedef struct Monitor {
    int x, y, w, h;
    Desktop desktops[DesktopCount];
    int activeDesktop;
    Client *head;
    Client *tail;
    struct Monitor *next;
} Monitor;

void InitializeMonitors();
void TeardownMonitors();
void AttachClientToMonitor(Monitor *m, Client *c);
void DetachClientFromMonitor(Monitor *m, Client *c);
void SetActiveDesktop(Monitor *m, int desktop);
void Restack(Monitor *m);
Client *NextClient(Client *c);
Client *PreviousClient(Client *c);
void MoveClientAfter(Client *c, Client *after);
void MoveClientBefore(Client *c, Client *before);
void PushClientFront(Client *c);
void PushClientBack(Client *c);
void AssignClientToDesktop(Client *c, int d);

#endif
