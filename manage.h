#ifndef __MANAGE_H__
#define __MANAGE_H__

#include <X11/Xlib.h>

typedef struct _Monitor Monitor;
typedef struct _Client Client;

void ManageWindow(Window w, Bool exists);
void ForgetWindow(Window w);

Client *Lookup(Window w);

//void SetActiveMonitor(Monitor *m);
void SetActiveClient(Client *c);

#endif
