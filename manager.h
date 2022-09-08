#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <X11/Xlib.h>

typedef struct Client Client;
typedef struct Monitor Monitor;

extern Client *activeClient;
extern Monitor *activeMonitor;

void SetupWindowManager();
void CleanupWindowManager();
void ReloadConfig();
void ManageWindow(Window w, Bool mapped);
void UnmanageWindow(Window w, Bool destroyed);
Client *Lookup(Window w);
void SetActiveClient(Client *c);
void ActivateNext();
void ActivatePrev();
void MoveForward();
void MoveBackward();
void ShowDesktop(int desktop);
void MoveToDesktop(int desktop);
void ToggleDynamic();
void AddMaster(int nb);

#endif /* __MANAGER_H__ */
