#ifndef __MANAGE_H__
#define __MANAGE_H__

#include <X11/Xlib.h>

typedef struct _Monitor Monitor;
typedef struct _Client Client;

void ManageWindow(Window w, Bool exists);
Client *Lookup(Window w);
void ForgetWindow(Window w, Bool survives);

void SetActiveClient(Client *c);
void FindNextActiveClient(); /* find a new home for the active */

void Quit();
void MaximizeActiveClientVertically();
void MaximizeActiveClientHorizontally();
void MaximizeActiveClientLeft();
void MaximizeActiveClientRight();
void MaximizeActiveClientTop();
void MaximizeActiveClientBottom();
void MaximizeActiveClient();
void RestoreActiveClient();
void CycleActiveMonitorForward();
void CycleActiveMonitorBackward();
void ShowActiveMonitorDesktop(int desktop);
void MoveActiveClientToDesktop(int desktop);

#endif
