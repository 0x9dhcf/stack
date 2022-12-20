#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <X11/Xlib.h>

typedef struct Client Client;
typedef struct Monitor Monitor;

extern Monitor *activeMonitor;
extern Client *activeClient;

void SetupWindowManager();
void CleanupWindowManager();

void ManageWindow(Window w, Bool mapped);
void UnmanageWindow(Window w, Bool destroyed);
Client *LookupClient(Window w);

void Quit();
void Reload();

void SetActiveMonitor(Monitor *m);
void ActivateNextMonitor();
void ActivatePreviousMonitor();

void SetActiveClient(Client *c);
void ActivateNextClient();
void ActivatePreviousClient();

#endif /* __MANAGER_H__ */
