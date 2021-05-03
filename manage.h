#ifndef __MANAGE_H__
#define __MANAGE_H__

#include <X11/Xlib.h>

typedef struct _Monitor Monitor;
typedef struct _Client Client;

extern Monitor  *stActiveMonitor;
extern Client   *stActiveClient;

void ManageWindow(Window w, Bool exists);
void ForgetWindow(Window w);
Client *Lookup(Window w);
void SetActiveClient(Client *c);
void Spawn(char **args);

void Quit();
void MaximizeVertically();
void MaximizeHorizontally();
void MaximizeLeft();
void MaximizeRight();
void MaximizeTop();
void MaximizeBottom();
void Maximize();
void Restore();
void CycleForward();
void CycleBackward();

#endif
