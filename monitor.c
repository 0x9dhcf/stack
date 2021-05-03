#include <stdlib.h>
#include <string.h>

#include <X11/extensions/Xrandr.h>

#include "client.h"
#include "log.h"
#include "manage.h"
#include "monitor.h"
#include "x11.h"

#define fforeach(o, c) for (c = o->chead; c; c = c->next)
#define bforeach(o, c) for (c = o->ctail; c; c = c->prev)
#define sfforeach(o, c, d) for (c = o->chead, d = c ? c->next : 0; c; c = d, d = c ? c->next : 0)
#define sbforeach(o, c, d) for (c = o->ctail, d = c ? c->prev : 0; c; c = d, d = c ? c->prev : 0)

static void PushClientFront(Monitor *m, Client *c);
static void PushClientBack(Monitor *m, Client *c);
static void RemoveClient(Monitor *m, Client *c);

Monitor *stMonitors = NULL;

void
InitializeMonitors()
{
    XRRScreenResources *sr;
    XRRCrtcInfo *ci;
    int i;

    sr = XRRGetScreenResources(stDisplay, stRoot);
    for (i = 0, ci = NULL; i < sr->ncrtc; ++i) {
        ci = XRRGetCrtcInfo(stDisplay, sr, sr->crtcs[i]);
        if (ci == NULL)
            continue;
        if (ci->noutput == 0) {
            XRRFreeCrtcInfo(ci);
            continue;
        }

        Monitor *m = malloc(sizeof(Monitor));
        if (!m) {
            ELog("could not allocate memory for output.");
            continue;
        }
        memset(m, 0 , sizeof(Monitor));

        m->x = m->wx = ci->x;
        m->y = m->wy = ci->y;
        m->w = m->ww = ci->width;
        m->h = m->wh = ci->height;
        m->chead= NULL;
        m->ctail= NULL;
        m->next = stMonitors;
        stMonitors = m;
        /* first found is our *primary* */
        if (!stActiveMonitor)
            stActiveMonitor = m;
        XRRFreeCrtcInfo(ci);
        DLog("monitor: (%d, %d) [%d x %d]", m->x, m->y, m->w, m->h);
    }
    XRRFreeScreenResources(sr);

    /* fallback */
    if (!stMonitors) {
        stMonitors = malloc(sizeof(Monitor));
        stMonitors->x = stMonitors->wx = 0;
        stMonitors->y = stMonitors->wy = 0;
        stMonitors->w = stMonitors->ww = DisplayWidth(stDisplay, stScreen);
        stMonitors->h = stMonitors->wh = XDisplayHeight(stDisplay, stScreen);
        stMonitors->chead= NULL;
        stMonitors->ctail= NULL;
        stMonitors->next = NULL;
    }
}

void
TeardownMonitors()
{
    Monitor *m = stMonitors;
    while (m) {
        Monitor *p = m->next;
        Client *c, *d;
        sfforeach(m, c, d) {
            RemoveClient(m, c);
            // XXX DestroyClient(c);
        }
        free(m);
        m = p;
    }
}

void
AttachClientToMonitor(Monitor *m, Client *c)
{
    c->monitor = m;
    PushClientFront(m, c);

    DLog("working area: (%d, %d) [%d x %d]", m->wx, m->wy, m->ww, m->wh);
    DLog("strut : %d, %d %d,  %d", c->strut.left, c->strut.top, c->strut.right, c->strut.bottom);
    /* compute the new working area */
    m->wx = Max(m->wx, m->x + c->strut.left);
    m->wy = Max(m->wy, m->y + c->strut.top);
    m->ww = Min(m->ww, m->w - (c->strut.right + c->strut.left));
    m->wh = Min(m->wh, m->h - (c->strut.top + c->strut.bottom));

    DLog("working area: (%d, %d) [%d x %d]", m->wx, m->wy, m->ww, m->wh);

    /* keep the frame inbound */
    MoveResizeClientFrame(c,
            Max(c->fx, m->wx),
            Max(c->fy, m->wy),
            Min(c->fw, m->ww),
            Min(c->fh, m->wh),
            False);
}

void
DetachClientFromMonitor(Monitor *m, Client *c)
{
    c->monitor = NULL;
    RemoveClient(m, c);
}

Client *
LookupMonitorClient(Monitor *m, Window w)
{
    Client *c;
    fforeach(m, c) {
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
NextClient(Monitor *m, Client *c)
{
    return c->next ? c->next : m->chead;
}

Client*
PreviousClient(Monitor *m, Client *c)
{
    return c->prev ? c->prev : m->ctail;
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

void
RemoveClient(Monitor *m, Client *c)
{
    if (c->prev)
          c->prev->next = c->next;
    else
        m->chead = c->next;

    if (c->next)
        c->next->prev = c->prev;
    else
        m->ctail = c->prev;

    c->next = NULL;
    c->prev = NULL;
}

void
PushClientFront(Monitor *m, Client *c)
{
    if (m->chead) {
        c->next = m->chead;
        m->chead->prev = c;
    }

    if (! m->ctail)
        m->ctail = c;

    m->chead = c;
    c->prev = NULL;
}

void
PushClientBack(Monitor *m, Client *c)
{
    if (m->ctail) {
        c->prev = m->ctail;
        m->ctail->next = c;
    }

    if (! m->chead)
        m->chead = c;

    m->ctail = c;
    c->next = NULL;
}

