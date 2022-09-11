#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "client.h"
#include "monitor.h"
#include "stack.h"

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
        RestackMonitor(m);
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
RestackMonitor(Monitor *m)
{
    /* if dynamic mode is enabled re tile the desktop */
    if (m->desktops[m->activeDesktop].dynamic) {
        XEvent e;
        Client *c;
        int n = 0, mw = 0, i = 0, mx = 0, ty = 0;

        for (c = m->head; c; c = c->next)
            if (c->desktop == m->activeDesktop
                    && !(c->types & NetWMTypeFixed)
                    && !c->transfor) {
                /* restore hidden client dynamic
                 * is our window switcher */
                if (c->states & NetWMStateHidden)
                    RestoreClient(c);
                n++;
            }

        Desktop *d =  &m->desktops[m->activeDesktop];
        if (n > d->masters)
            mw = d->masters ? d->ww * d->split : 0;
        else
            mw = d->ww;

        // TODO: tile from the active!
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

