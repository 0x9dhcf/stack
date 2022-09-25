#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <X11/Xlib.h>

typedef struct Client Client;
typedef struct Monitor Monitor;

extern Client *activeClient;
extern Monitor *activeMonitor;

void SetupWindowManager();
void CleanupWindowManager();
void StartEventLoop();
void StopEventLoop();
int EnableErrorHandler(Display *d, XErrorEvent *e);
int DisableErrorHandler(Display *d, XErrorEvent *e);
void ReloadConfig();
void ManageWindow(Window w, Bool mapped);
void UnmanageWindow(Window w, Bool destroyed);
Client *LookupClient(Window w);
void SetActiveClient(Client *c);
void SwitchToNextClient();
void SwitchToPreviousClient();
void SetActiveMonitor(Monitor *m);
void SwitchToNextMonitor();
void SwitchToPreviousMonitor();

#endif /* __MANAGER_H__ */
