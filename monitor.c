#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "client.h"
#include "log.h"
#include "monitor.h"
#include "utils.h"
#include "x11.h"

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

    if (c->transfor && c->transfor != m->chead) {
        c->next = c->transfor;
        c->prev = c->transfor->prev;
        c->transfor->prev->next = c;
        c->transfor->prev = c;
    } else {
        PushClientFront(c);
    }

    AssignClientToDesktop(c, m->activeDesktop);
}

void
DetachClientFromMonitor(Monitor *m, Client *c)
{
    int desktop = c->desktop;
    Desktop *d = &m->desktops[desktop];

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

    if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
        d->wx = m->x;
        d->wy = m->y;
        d->ww = m->w;
        d->wh = m->h;
        for (Client *mc = m->chead; mc; mc = mc->next) {
            d->wx = Max(d->wx, m->x + mc->strut.left);
            d->wy = Max(d->wy, m->y + mc->strut.top);
            d->ww = Min(d->ww, m->w - (mc->strut.right + mc->strut.left));
            d->wh = Min(d->wh, m->h - (mc->strut.top + mc->strut.bottom));
        }
    }

    c->monitor = NULL;
    c->desktop = -1;
    if ((c->strut.right
                || c->strut.left
                || c->strut.top
                || c->strut.bottom
                || (desktop == m->activeDesktop
                    && m->desktops[m->activeDesktop].dynamic)))
        Restack(m);
}

void
SetActiveDesktop(Monitor *m, int desktop)
{
    if (desktop < 0 || desktop >= DesktopCount || m->activeDesktop == desktop)
        return;

    m->activeDesktop = desktop;

    /* assign all stickies to this desktop */
    for (Client *c = m->chead; c; c = c->next)
        if ((c->states & NetWMStateSticky) || (c->types & NetWMTypeFixed))
            AssignClientToDesktop(c, desktop);

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
    }
}
