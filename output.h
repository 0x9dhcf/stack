#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <X11/Xlib.h>

typedef struct _Client Client;

typedef struct _Output {
    int x, y, w, h;
    //Client* clients;
    Client *chead;
    Client *ctail;
    struct _Output *next;
} Output;

Output* CreateOutput(int x, int y, int w, int h);
void DestroyOutput(Output *o);
void AttachClientToOutput(Output *o, Client *c);
void DetachClientFromOutput(Output *o, Client *c);

Client* LookupOutputClient(Output *o, Window w);
Client* NextOutputClient(Output *o, Client *c);
Client* PrevOutputClient(Output *o, Client *c);
//void AddClientToOutputDesktop(Output *o, Client *c, Desktop d);
//void RemoveClientFromOutputDesktop(Output *o, Client *c, Desktop d);
//void MoveClientToOutputDesktop(Output *o, Client *c, Desktop d);

#endif
