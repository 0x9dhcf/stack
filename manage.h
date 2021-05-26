#ifndef __MANAGE_H__
#define __MANAGE_H__

#include <X11/Xlib.h>

typedef struct _Monitor Monitor;
typedef struct _Client Client;

void ManageWindow(Window w, Bool exists);
Client *Lookup(Window w);
void ForgetWindow(Window w, Bool survives);

void SetActiveClient(Client *c);
void FindNextActiveClient();

void Quit();
void ActiveNext();
void ActivePrev();
void MoveForward();
void MoveBackward();
void ShowDesktop(int desktop);
void MoveToDesktop(int desktop);
void ToggleDynamic();
void AddMaster(int nb);

#endif
