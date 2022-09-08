#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "client.h"
#include "config.h"
#include "monitor.h"
#include "stack.h"

void
InitializeMonitors()
{
    XRRScreenResources *sr;
    XRRCrtcInfo *ci;
    int i;

    sr = XRRGetScreenResources(display, root);
    for (i = 0, ci = NULL; i < sr->ncrtc; ++i) {
        ci = XRRGetCrtcInfo(display, sr, sr->crtcs[i]);
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
            m->desktops[i].masters = config.masters;
            m->desktops[i].split = config.split;
        }
        m->activeDesktop = 0;
        m->head= NULL;
        m->tail= NULL;
        m->next = monitors;
        monitors = m;
        XRRFreeCrtcInfo(ci);
    }
    XRRFreeScreenResources(sr);

    /* fallback */
    if (!monitors) {
        monitors = malloc(sizeof(Monitor));
        monitors->x = 0;
        monitors->y = 0;
        monitors->w = DisplayWidth(display, DefaultScreen(display));
        monitors->h = XDisplayHeight(display, DefaultScreen(display));
        for (int i = 0; i < DesktopCount; ++i) {
            monitors->desktops[i].wx = monitors->x;
            monitors->desktops[i].wy = monitors->y;
            monitors->desktops[i].ww = monitors->w;
            monitors->desktops[i].wh = monitors->h;
            monitors->desktops[i].masters = config.masters;
            monitors->desktops[i].split = config.split;
        }
        monitors->activeDesktop = 0;
        monitors->head= NULL;
        monitors->tail= NULL;
        monitors->next = NULL;
    }
    XChangeProperty(display, root, atoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace,
            (unsigned char *)&monitors->activeDesktop, 1);
}

void
TeardownMonitors()
{
    Monitor *m = monitors;
    while (m) {
        Monitor *p = m->next;
        free(m);
        m = p;
    }
    monitors = NULL;
}

//Monitor *
//GetMonitors()
//{
//    DAssert(monitors != NULL);
//    return monitors;
//}

void
AttachClientToMonitor(Monitor *m, Client *c)
{
    c->monitor = m;

    if (c->transfor && c->transfor != m->head) {
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
        m->head = c->next;

    if (c->next)
        c->next->prev = c->prev;
    else
        m->tail = c->prev;

    c->next = NULL;
    c->prev = NULL;

    if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
        d->wx = m->x;
        d->wy = m->y;
        d->ww = m->w;
        d->wh = m->h;
        for (Client *mc = m->head; mc; mc = mc->next) {
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
    for (Client *c = m->head; c; c = c->next)
        if ((c->states & NetWMStateSticky) || (c->types & NetWMTypeFixed))
            AssignClientToDesktop(c, desktop);

    XChangeProperty(display, root, atoms[AtomNetCurrentDesktop],
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

        for (c = m->head; c; c = c->next)
            if (c->desktop == m->activeDesktop
                    && !(c->types & NetWMTypeFixed)
                    && !c->transfor)
                n++;

        Desktop *d =  &m->desktops[m->activeDesktop];
        if (n > d->masters)
            mw = d->masters ? d->ww * d->split : 0;
        else
            mw = d->ww;

        for (c = m->head; c; c = c->next) {
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
        XSync(display, False);
        while (XCheckMaskEvent(display, EnterWindowMask, &e));
    } else {
        for (Client *c = m->head; c; c = c->next)
            if (c->desktop == m->activeDesktop)
                ShowClient(c);
            else
                HideClient(c);
    }
}

Client *
NextClient(Client *c)
{
    return c->next ? c->next : c->monitor->head;
}

Client *
PreviousClient(Client *c)
{
    return c->prev ? c->prev : c->monitor->tail;
}

void
MoveClientAfter(Client *c, Client *after)
{
    if (c == after)
        return;

    /* remove c */
    if (c->prev)
          c->prev->next = c->next;
    else
        c->monitor->head = c->next;

    /* change monitor is needed */
    c->monitor = after->monitor;

    if (c->next)
        c->next->prev = c->prev;
    else
        c->monitor->tail = c->prev;

    if (after == c->monitor->tail) {
        PushClientBack(c);
    } else {
        c->next = after->next;
        c->prev = after;
        after->next->prev = c;
        after->next = c;
    }
}

void
MoveClientBefore(Client *c, Client *before)
{
    if (c == before)
        return;

    /* remove c */
    if (c->prev)
          c->prev->next = c->next;
    else
        c->monitor->head = c->next;

    /* change monitor is needed */
    c->monitor = before->monitor;

    if (c->next)
        c->next->prev = c->prev;
    else
        c->monitor->tail = c->prev;

    if (before == c->monitor->head) {
        PushClientFront(c);
    } else {
        c->next = before;
        c->prev = before->prev;
        before->prev->next = c;
        before->prev = c;
    }
}

void
PushClientFront(Client *c)
{
    if (c->monitor->head) {
        c->next = c->monitor->head;
        c->monitor->head->prev = c;
    }

    if (! c->monitor->tail)
        c->monitor->tail = c;

    c->monitor->head = c;
    c->prev = NULL;
}

void
PushClientBack(Client *c)
{
    if (c->monitor->tail) {
        c->prev = c->monitor->tail;
        c->monitor->tail->next = c;
    }

    if (! c->monitor->head)
        c->monitor->head = c;

    c->monitor->tail = c;
    c->next = NULL;
}

void
AssignClientToDesktop(Client *c, int desktop)
{
    Monitor *m = c->monitor;
    Desktop *d = &(m->desktops[desktop]);

    if (desktop < 0 || desktop >= DesktopCount || c->desktop == desktop)
        return;

    /* remove ourself from previous desktop if any */
    if (c->desktop >= 0) {
        Desktop *pd = &m->desktops[c->desktop];
        if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
            pd->wx = m->x;
            pd->wy = m->y;
            pd->ww = m->w;
            pd->wh = m->h;
            for (Client *mc = m->head; mc; mc = mc->next) {
                if (mc != c && mc->desktop == c->desktop) {
                    pd->wx = Max(pd->wx, m->x + mc->strut.left);
                    pd->wy = Max(pd->wy, m->y + mc->strut.top);
                    pd->ww = Min(pd->ww, m->w - (mc->strut.right + mc->strut.left));
                    pd->wh = Min(pd->wh, m->h - (mc->strut.top + mc->strut.bottom));
                }
            }
        }
    }

    /* now set the new one */
    if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
        d->wx = Max(d->wx, m->x + c->strut.left);
        d->wy = Max(d->wy, m->y + c->strut.top);
        d->ww = Min(d->ww, m->w - (c->strut.right + c->strut.left));
        d->wh = Min(d->wh, m->h - (c->strut.top + c->strut.bottom));
    }

    c->desktop = desktop;
    if (c->tiled && !d->dynamic) {
        UntileClient(c);
        Restack(c->monitor);
    }

    if (!(c->types & NetWMTypeFixed))
        MoveResizeClientFrame(c,
                Max(c->fx, c->monitor->desktops[c->desktop].wx),
                Max(c->fy, c->monitor->desktops[c->desktop].wy),
                Min(c->fw, c->monitor->desktops[c->desktop].ww),
                Min(c->fh, c->monitor->desktops[c->desktop].wh), False);

    if ((c->strut.right
            || c->strut.left
            || c->strut.top
            || c->strut.bottom
            || d->dynamic))
        Restack(m);

    /* finally let the pager know where we are */
    XChangeProperty(display, c->window, atoms[AtomNetWMDesktop], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char*)&c->desktop, 1);
}
