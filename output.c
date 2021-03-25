#include "stack.h"
#include "output.h"
#include "client.h"
#include "log.h"

#include <stdlib.h>

Output*
CreateOutput(int x, int y, int w, int h)
{
    Output *o = malloc(sizeof(Output));
    if (!o)
        return NULL;

    o->x = x;
    o->y = y;
    o->w = w;
    o->h = h;
    //for (int i = 0; i < DESKTOP_COUNT; ++i) {
    //    o->desktops[i].x = x;
    //    o->desktops[i].y = y;
    //    o->desktops[i].w = w;
    //    o->desktops[i].h = h;
    //}

    //o->active = 0;
    o->clients= NULL;
    o->next = NULL;
    return o;
}

void
DestroyOutput(Output *o)
{
    DLog();
    Client *c = o->clients;
    while (c) {
        Client *p = c->next;
        DestroyClient(c);
        c = p;
    }
}

void
AttachClientToOutput(Output *o, Client *c)
{
    c->output = o;
    c->next = o->clients;
    o->clients = c;

    /* keep any frame inbound */
    int fx, fy;
    fx = c->x - WXOffset(c);
    fy = c->y - WYOffset(c);

    DLog("fx: %d", fx);
    DLog("fy: %d", fy);
    DLog("Max(fx, o->x) + WXOffset(c): %d", Max(fx, o->x) + WXOffset(c));
    DLog("Max(fy, o->y) + WYOffset(c): %d", Max(fy, o->y) + WYOffset(c));

    MoveResizeClient(c,
            Max(fx, o->x) + WXOffset(c),
            Max(fy, o->y) + WYOffset(c),
            Min(c->w, o->w),
            Min(c->h, o->h),
            False);
}

void
DetachClientFromOutput(Output *o, Client *c)
{
    c->output = NULL;
    Client **p;
    for(p = &o->clients; *p && *p != c; p = &(*p)->next);
    *p = c->next;
}

Client*
FindNextFocusableClient(Output *o, Client *c)
{
    /* find the first successor matching */
    for (Client *ic = c->next; ic; ic = ic->next)
        if (ic->focusable)
            return ic;

    /* if not found then find the first one from the head */
    for (Client *ic = o->clients; ic && ic != c; ic = ic->next)
        if (ic->focusable)
            return ic;

    /* not found */
    return NULL;
}

Client*
FindPrevFocusableClient(Output *o, Client *c)
{
    Client *nf = NULL; 
    for (Client *ic = o->clients; ic && ic != c; ic = ic->next)
        if (ic->focusable)
            nf = ic;

    if (nf)
        return nf;

    for (Client *ic = c->next; ic; ic = ic->next)
        if (ic->focusable)
            nf = ic;

    return nf;
}


//void
//AddClientToOutputDesktop(Output *o, Client *c, Desktop d)
//{
//    CLIENT_ADD_DESKTOP(c, d);
//    o->desktops[d].x = MAX(o->desktops[d].x, o->desktops[d].x + c->strut.left);
//    o->desktops[d].y = MAX(o->desktops[d].y, o->desktops[d].y + c->strut.top);
//    o->desktops[d].w = MIN(o->desktops[d].w, o->w - (c->strut.left + c->strut.right));
//    o->desktops[d].h = MIN(o->desktops[d].h, o->h - (c->strut.top + c->strut.bottom));
//    DLog("AddClientToOutputDesktop: (%d, %d) [%d x %d]",
//            o->desktops[d].x, o->desktops[d].y,
//            o->desktops[d].w, o->desktops[d].h);
//}
//
//void
//RemoveClientFromOutputDesktop(Output *o, Client *c, Desktop d)
//{
//    CLIENT_DEL_DESKTOP(c, d);
//    o->desktops[d].x = o->x;
//    o->desktops[d].y = o->y;
//    o->desktops[d].w = o->w;
//    o->desktops[d].h = o->h;
//    for (Client *p = o->clients; p; p = p->next) {
//        if (CLIENT_IS_ON_DESKTOP(p, d)) {
//            o->desktops[d].x = MAX(o->desktops[d].x, o->desktops[d].x + p->strut.left);
//            o->desktops[d].y = MAX(o->desktops[d].y, o->desktops[d].y + p->strut.top);
//            o->desktops[d].w = MIN(o->desktops[d].w, o->w - (c->strut.left + p->strut.right));
//            o->desktops[d].h = MIN(o->desktops[d].h, o->h - (c->strut.top + p->strut.bottom));
//        }
//    }
//}
//
//void
//MoveClientToOutputDesktop(Output *m, Client *c, Desktop d)
//{
//    for (int i = 0; i < DESKTOP_COUNT; ++i)
//        if (CLIENT_IS_ON_DESKTOP(c, i))
//            RemoveClientFromOutputDesktop(m, c, i);
//
//    AddClientToOutputDesktop(m, c, d);
//}
