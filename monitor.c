#include <stdlib.h>
#include <string.h>

#include <X11/extensions/Xrandr.h>

#include "atoms.h"
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
static void AddClientToDesktop(Monitor *m, Client *c, int d);
static void RemoveClientFromDesktop(Monitor *m, Client *c, int d);

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

        m->x = ci->x;
        m->y = ci->y;
        m->w = ci->width;
        m->h = ci->height;
        for (int i = 0; i < DesktopCount; ++i) {
            m->desktops[i].wx = m->x;
            m->desktops[i].wy = m->y;
            m->desktops[i].ww = m->w;
            m->desktops[i].wh = m->h;
        }
        m->activeDesktop = 0;
        m->chead= NULL;
        m->ctail= NULL;
        m->next = stMonitors;
        stMonitors = m;
        /* first found is our *primary* */
        if (!stActiveMonitor)
            stActiveMonitor = m;
        XRRFreeCrtcInfo(ci);
    }
    XRRFreeScreenResources(sr);

    /* fallback */
    if (!stMonitors) {
        stMonitors = malloc(sizeof(Monitor));
        stMonitors->x = 0;
        stMonitors->y = 0;
        stMonitors->w = DisplayWidth(stDisplay, stScreen);
        stMonitors->h = XDisplayHeight(stDisplay, stScreen);
        for (int i = 0; i < DesktopCount; ++i) {
            stMonitors->desktops[i].wx = stMonitors->x;
            stMonitors->desktops[i].wy = stMonitors->y;
            stMonitors->desktops[i].ww = stMonitors->w;
            stMonitors->desktops[i].wh = stMonitors->h;
        }
        stMonitors->activeDesktop = 0;
        stMonitors->chead= NULL;
        stMonitors->ctail= NULL;
        stMonitors->next = NULL;
    }
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace,
            (unsigned char *)&stMonitors->activeDesktop, 1);
}

void
TeardownMonitors()
{
    Monitor *m = stMonitors;
    while (m) {
        Monitor *p = m->next;
        Client *c, *d;
        sfforeach(m, c, d) {
            RemoveClient(m, c); /* should not happen */
        }
        free(m);
        m = p;
    }
}

void
AttachClientToMonitor(Monitor *m, Client *c)
{
    c->monitor = m;
    c->desktop = m->activeDesktop;
    PushClientFront(m, c);

    AddClientToDesktop(m, c, c->desktop);
    MoveResizeClientFrame(c,
            Max(c->fx, c->monitor->desktops[c->desktop].wx),
            Max(c->fy, c->monitor->desktops[c->desktop].wy),
            Min(c->fw, c->monitor->desktops[c->desktop].ww),
            Min(c->fh, c->monitor->desktops[c->desktop].wh), False);

    if ((c->strut.right || c->strut.left || c->strut.top || c->strut.bottom)
            && c->desktop == m->activeDesktop)
        Restack(m);
}

void
DetachClientFromMonitor(Monitor *m, Client *c)
{
    c->monitor = NULL;
    c->desktop = 0;
    RemoveClient(m, c);

    RemoveClientFromDesktop(m, c, c->desktop);
    if ((c->strut.right || c->strut.left || c->strut.top || c->strut.bottom)
            && c->desktop == m->activeDesktop)
        Restack(m);
}

void
SetActiveDesktop(Monitor *m, int desktop)
{
    if (desktop < 0 || desktop >= DesktopCount)
        return;

    m->activeDesktop = desktop;
    /* affect all sickies to this desktop */
    for (Client *c = m->chead; c; c = c->next) {
        if (c->states & NetWMStateSticky || c->types & NetWMTypeFixed) {
            RemoveClientFromDesktop(m, c, c->desktop);
            c->desktop = desktop;
            AddClientToDesktop(m, c, c->desktop);
        }
    }

    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktop, 1);

    Restack(m);
}

void
Restack(Monitor *m)
{
    DLog();

    Client *c;
    fforeach(m, c) {
        if (c->desktop == m->activeDesktop)
            ShowClient(c);
        else /* hide the client */
            HideClient(c);
    }
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

void
AddClientToDesktop(Monitor *m, Client *c, int d)
{
    if (d < 0 || d >= DesktopCount)
        return;

    if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
        m->desktops[d].wx = Max(m->desktops[d].wx, m->x + c->strut.left);
        m->desktops[d].wy = Max(m->desktops[d].wy, m->y + c->strut.top);
        m->desktops[d].ww = Min(m->desktops[d].ww, m->w -
                (c->strut.right + c->strut.left));
        m->desktops[d].wh = Min(m->desktops[d].wh, m->h -
                (c->strut.top + c->strut.bottom));
    }
}

void
RemoveClientFromDesktop(Monitor *m, Client *c, int d)
{
    if (d < 0 || d >= DesktopCount)
        return;

    if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
        m->desktops[d].wx = m->x;
        m->desktops[d].wy = m->y;
        m->desktops[d].ww = m->w;
        m->desktops[d].wh = m->h;
        for (Client *mc = m->chead; mc; mc = mc->next) {
            m->desktops[d].wx = Max(m->desktops[d].wx, m->x + mc->strut.left);
            m->desktops[d].wy = Max(m->desktops[d].wy, m->y + mc->strut.top);
            m->desktops[d].ww = Min(m->desktops[d].ww, m->w -
                    (mc->strut.right + mc->strut.left));
            m->desktops[d].wh = Min(m->desktops[d].wh, m->h -
                    (mc->strut.top + mc->strut.bottom));
        }
    }
}
