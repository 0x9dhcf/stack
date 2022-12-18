#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <X11/Xlib.h>

#define DesktopCount 8

typedef struct Monitor Monitor;
typedef struct Desktop Desktop;
typedef struct Client Client;

struct Desktop {
    int wx, wy, ww, wh;
    Bool isDynamic;
    Bool showTopbars;
    int masters;
    float split;
    Client *activeOnLeave;
};

struct Monitor {
    int id;
    int x, y, w, h;
    Desktop desktops[DesktopCount];
    int activeDesktop;
    Client *head;
    Client *tail;
    Monitor *next;
};

Bool SetupMonitors();
void CleanupMonitors();

Monitor *MainMonitor();
Monitor *NextMonitor(Monitor *m);
Monitor *PreviousMonitor(Monitor *m);
Monitor *MonitorContaining(int x, int y);

void AttachClientToMonitor(Monitor *m, Client *c);
void DetachClientFromMonitor(Monitor *m, Client *c);

void ShowMonitorDesktop(Monitor *m, int desktop);
void ShowNextMonitorDesktop(Monitor *m);
void ShowPreviousMonitorDesktop(Monitor *m);

void SetMonitorDesktopDynamic(Monitor *m, int desktop, Bool b);
void ToggleMonitorDesktopDynamic(Monitor *m, int desktop);
void SetMonitorDynamic(Monitor *m, Bool b);
void ToggleMonitorDynamic(Monitor *m);

void SetMonitorDesktopMasterCount(Monitor *m, int desktop, int count);
void IncreaseMonitorDesktopMasterCount(Monitor *m, int desktop);
void DecreaseMonitorDesktopMasterCount(Monitor *m, int desktop);
void SetMonitorMasterCount(Monitor *m, int count);
void IncreaseMonitorMasterCount(Monitor *m);
void DecreaseMonitorMasterCount(Monitor *m);

void SetMonitorDesktopTopbar(Monitor *m, int desktop, Bool b);
void ToggleMonitorDesktopTopbar(Monitor *m, int desktop);
void SetMonitorTopbar(Monitor *m, Bool b);
void ToggleMonitorTopbar(Monitor *m);

void StackClientAfter(Monitor *m, Client *c, Client *after);
void StackClientBefore(Monitor *m, Client *c, Client *before);
void StackClientDown(Monitor *m, Client *c);
void StackClientUp(Monitor *m, Client *c);
void StackClientFront(Monitor *m, Client *c);
void StackClientBack(Monitor *m, Client *c);

void RefreshMonitor(Monitor *m);

#endif /* __MONITOR_H__ */
