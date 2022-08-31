#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "client.h"
#include "log.h"
#include "monitor.h"
#include "utils.h"
#include "x11.h"

static void PushClientFront(Monitor *m, Client *c);
static void PushClientBack(Monitor *m, Client *c);

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
            m->desktops[i].masters = stConfig.masters;
            m->desktops[i].split = stConfig.split;
        }
        m->activeDesktop = 0;
        m->chead= NULL;
        m->ctail= NULL;
        m->next = stMonitors;
        stMonitors = m;
        // XXX TO MOVE
        // /* first found is our *primary* */
        //if (!stActiveMonitor)
        //    stActiveMonitor = m;
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
            stMonitors->desktops[i].masters = stConfig.masters;
            stMonitors->desktops[i].split = stConfig.split;
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
        free(m);
        m = p;
    }
}

void
AttachClientToMonitor(Monitor *m, Client *c)
{
    c->monitor = m;
    PushClientFront(m, c);

    AddClientToDesktop(m, c, m->activeDesktop);

    if ((c->strut.right
                || c->strut.left
                || c->strut.top
                || c->strut.bottom
                || m->desktops[m->activeDesktop].dynamic))
        Restack(m);
}

void
DetachClientFromMonitor(Monitor *m, Client *c)
{
    int desktop = c->desktop;

    if (c->monitor != m)
        return;

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

    RemoveClientFromDesktop(m, c, c->desktop);
    c->monitor = NULL;
    if ((c->strut.right
                || c->strut.left
                || c->strut.top
                || c->strut.bottom
                || (desktop == m->activeDesktop
                    && m->desktops[m->activeDesktop].dynamic)))
        Restack(m);
}

Client *
NextClient(Monitor *m, Client *c)
{
    if (c->monitor != m)
        return NULL;

    return c->next ? c->next : m->chead;
}

Client *
PreviousClient(Monitor *m, Client *c)
{
    if (c->monitor != m)
        return NULL;

    return c->prev ? c->prev : m->ctail;
}

void
MoveClientAfter(Monitor *m, Client *c, Client *after)
{
    /* remove c */
    if (c->prev)
          c->prev->next = c->next;
    else
        m->chead = c->next;

    if (c->next)
        c->next->prev = c->prev;
    else
        m->ctail = c->prev;

    if (after == m->ctail) {
        PushClientBack(m, c);
    } else {
        c->next = after->next;
        c->prev = after;
        after->next->prev = c;
        after->next = c;
    }
}

void
MoveClientBefore(Monitor *m, Client *c, Client *before)
{
    /* remove c */
    if (c->prev)
          c->prev->next = c->next;
    else
        m->chead = c->next;

    if (c->next)
        c->next->prev = c->prev;
    else
        m->ctail = c->prev;

    if (before == m->chead) {
        PushClientFront(m, c);
    } else {
        c->next = before;
        c->prev = before->prev;
        before->prev->next = c;
        before->prev = c;
    }
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

    c->desktop = d;
    if (c->tiled && !m->desktops[d].dynamic) {
        RestoreClient(c);
        Restack(c->monitor);
    }

    if (!(c->types & NetWMTypeFixed))
        MoveResizeClientFrame(c,
                Max(c->fx, c->monitor->desktops[c->desktop].wx),
                Max(c->fy, c->monitor->desktops[c->desktop].wy),
                Min(c->fw, c->monitor->desktops[c->desktop].ww),
                Min(c->fh, c->monitor->desktops[c->desktop].wh), False);
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
    c->desktop = -1;
}

void
SetActiveDesktop(Monitor *m, int desktop)
{
    if (desktop < 0 || desktop >= DesktopCount)
        return;

    m->activeDesktop = desktop;

    /* affect all stickies to this desktop */
    for (Client *c = m->chead; c; c = c->next) {
        if ((c->states & NetWMStateSticky) || (c->types & NetWMTypeFixed)) {
            RemoveClientFromDesktop(m, c, c->desktop);
            c->desktop = desktop;
            AddClientToDesktop(m, c, c->desktop);
        }
    }

    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktop, 1);
}

void
Restack(Monitor *m)
{
    /* if dynamic mode is enabled re tile the desktop */
    if (m->desktops[m->activeDesktop].dynamic) {
        XEvent e;
        Client *c;
        int n = 0, mw = 0, i = 0, mx = 0, ty = 0;

        for (c = m->chead; c; c = c->next)
            if (c->desktop == m->activeDesktop
                    && !(c->types & NetWMTypeFixed)
                    && !c->transfor)
                n++;

        Desktop *d =  &m->desktops[m->activeDesktop];
        if (n > d->masters)
            mw = d->masters ? d->ww * d->split : 0;
        else
            mw = d->ww;

        for (c = m->chead; c; c = c->next) {
            if (c->desktop == m->activeDesktop
                    && !(c->types & NetWMTypeFixed)
                    && !c->transfor) {
                if (i < d->masters) {
                    int w = (mw - mx) / (Min(n, d->masters) - i);
                    TileClient(c, d->wx + mx, d->wy, w, d->wh);
                    if (mx + c->fw < d->ww)
                        mx += c->fw;
                } else {
                    int h = (d->wh - ty) / (n - i);
                    TileClient(c, d->wx + mw, d->wy + ty, d->ww - mw, h);
                    if (ty + c->fh < d->wh)
                        ty += c->fh;
                }
                i++;
            } else if (!(c->types & NetWMTypeFixed)) {
                HideClient(c);
            }
        }
        /* Avoid having enter notify event changing active client */
        XSync(stDisplay, False);
        while (XCheckMaskEvent(stDisplay, EnterWindowMask, &e));
    } else {
        for (Client *c = m->chead; c; c = c->next)
            if (c->desktop == m->activeDesktop)
                ShowClient(c);
            else 
                HideClient(c);

        /* Make sure fullscreens are on top */
        for (Client *c = m->chead; c; c = c->next)
            if (c->desktop == m->activeDesktop
                    && (c->states & NetWMStateFullscreen))
                RaiseClient(c);
    }
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

