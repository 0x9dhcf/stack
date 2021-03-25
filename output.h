#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#define DESKTOP_COUNT 10

typedef struct _Client Client;

//typedef unsigned int Desktop;

typedef struct _Output {
    int x, y, w, h;
    Client* clients;
    struct _Output *next;
} Output;

Output* CreateOutput(int x, int y, int w, int h);
void DestroyOutput(Output *o);
void AttachClientToOutput(Output *o, Client *c);
void DetachClientFromOutput(Output *o, Client *c);

Client* FindNextFocusableClient(Output *o, Client *c);
Client* FindPrevFocusableClient(Output *o, Client *c);
//void AddClientToOutputDesktop(Output *o, Client *c, Desktop d);
//void RemoveClientFromOutputDesktop(Output *o, Client *c, Desktop d);
//void MoveClientToOutputDesktop(Output *o, Client *c, Desktop d);

#endif
