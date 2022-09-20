#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <X11/Xlib.h>

#include "hints.h"

#define DesktopCount 8

typedef struct Client Client;

typedef struct Desktop {
    int wx, wy, ww, wh;
    Bool dynamic;
    Bool toolbar;
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

extern Monitor *monitors;

void SetupMonitors();
void CleanupMonitors();
void AttachClientToMonitor(Monitor *m, Client *c);
void DetachClientFromMonitor(Monitor *m, Client *c);
void SetActiveDesktop(Monitor *m, int desktop);
void RestackMonitor(Monitor *m);

#endif /* __MONITOR_H__ */
