#include "stack.h"
#include "output.h"
#include "client.h"
#include "log.h"

#include <stdlib.h>

#define fforeach(o, c) for (c = o->chead; c; c = c->next)
#define bforeach(o, c) for (c = o->ctail; c; c = c->prev)
#define sfforeach(o, c, d) for (c = o->chead, d = c ? c->next : 0; c; c = d, d = c ? c->next : 0)
#define sbforeach(o, c, d) for (c = o->ctail, d = c ? c->prev : 0; c; c = d, d = c ? c->prev : 0)

//static void InsertClient(Client *c, Client *prev, Client *next);
//static void RemoveClient(Client *c);

static void PushClientFront(Output *o, Client *c);
static void PushClientBack(Output *o, Client *c);
static void RemoveClient(Output *o, Client *c);

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
    o->chead= NULL;
    o->ctail= NULL;
    o->next = NULL;
    return o;
}

void
DestroyOutput(Output *o)
{
    Client *c, *d;
    sfforeach(o, c, d) {
        RemoveClient(o, c);
        DestroyClient(c);
    }
    free(o);
    o = NULL;
}

void
AttachClientToOutput(Output *o, Client *c)
{
    c->output = o;
    PushClientFront(o, c);

    /* keep the frame inbound */
    int fx, fy;
    fx = c->x - WXOffset(c);
    fy = c->y - WYOffset(c);

    //DLog("fx: %d", fx);
    //DLog("fy: %d", fy);
    //DLog("Max(fx, o->x) + WXOffset(c): %d", Max(fx, o->x) + WXOffset(c));
    //DLog("Max(fy, o->y) + WYOffset(c): %d", Max(fy, o->y) + WYOffset(c));

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
    RemoveClient(o, c);

    //Client **p;
    //for(p = &o->clients; *p && *p != c; p = &(*p)->next);
    //*p = c->next;
}

Client *
LookupOutputClient(Output *o, Window w)
{
    Client *c;
    fforeach(o, c) {
        if (c->window == w || c->frame == w || c->topbar == w)
            return c;

        for (int i = 0; i < ButtonCount; ++i)
            if (c->buttons[i] == w)
                return c;

        for (int i = 0; i < HandleCount; ++i)
            if (c->handles[i] == w)
                return c;
    };

    return NULL;
}

Client*
NextOutputClient(Output *o, Client *c)
{
    return c->next ? c->next : o->chead;
    ///* find the first successor matching */
    //for (Client *ic = c->next; ic; ic = ic->next)
    //    if (ic->focusable)
    //        return ic;

    ///* if not found then find the first one from the head */
    //for (Client *ic = o->clients; ic && ic != c; ic = ic->next)
    //    if (ic->focusable)
    //        return ic;

    /* not found */
    return NULL;
}

Client*
PrevOutputClient(Output *o, Client *c)
{
    return c->prev ? c->prev : o->ctail;
    //Client *nf = NULL;
    //for (Client *ic = o->clients; ic && ic != c; ic = ic->next)
    //    if (ic->focusable)
    //        nf = ic;

    //if (nf)
    //    return nf;

    //for (Client *ic = c->next; ic; ic = ic->next)
    //    if (ic->focusable)
    //        nf = ic;

    //return nf;
    return NULL;
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

//void
//InsertClient(Client *c, Client *prev, Client *next) {
//    next->prev = c;
//    c->next = next;
//    c->prev = prev;
//    prev->next = c;
//}

void
RemoveClient(Output *o, Client *c)
{
    if (c->prev)
          c->prev->next = c->next;
    else
        o->chead = c->next;

    if (c->next)
        c->next->prev = c->prev;
    else
        o->ctail = c->prev;

    c->next = NULL;
    c->prev = NULL;
}

void
PushClientFront(Output *o, Client *c)
{
    if (o->chead) {
        c->next = o->chead;
        o->chead->prev = c;
    }

    if (! o->ctail)
        o->ctail = c;

    o->chead = c;
    c->prev = NULL;
}

void
PushClientBack(Output *o, Client *c)
{
    if (o->ctail) {
        c->prev = o->ctail;
        o->ctail->next = c;
    }

    if (! o->chead)
        o->chead = c;

    o->ctail = c;
    c->next = NULL;
}

