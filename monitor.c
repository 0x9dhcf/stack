#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>

#include "client.h"
#include "hints.h"
#include "log.h"
#include "macros.h"
#include "manager.h"
#include "monitor.h"
#include "settings.h"
#include "x11.h"

static Bool IsXineramaScreenUnique(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info);
static Bool IsXRandRScreenUnique(XRRCrtcInfo *unique, size_t n, XRRCrtcInfo *info);
static Bool XineramaScanMonitors();
static Bool XRandRScanMonitors();
static void StackClientFront(Monitor *m, Client *c);
static void StackClientBack(Monitor *m, Client *c);

Monitor *monitors = NULL;

Bool
SetupMonitors()
{
    Bool dirty = False;

    if (extensions & ExtentionXRandR) {
        ILog("using xrandr");
        dirty = XRandRScanMonitors();
    } else if (extensions & ExtentionXinerama) {
        ILog("using xinerama");
        dirty = XineramaScanMonitors();
    } else {
        if (!monitors) {
            monitors = malloc(sizeof(Monitor));
            if (!monitors) FLog("can't allocate monitor");
            monitors->head= NULL;
            monitors->tail= NULL;
            monitors->next = NULL;
        }
        if (monitors->w != DisplayWidth(display, DefaultScreen(display))
                || monitors->h != XDisplayHeight(display, DefaultScreen(display))) {
            monitors->x = 0;
            monitors->y = 0;
            monitors->w = DisplayWidth(display, DefaultScreen(display));
            monitors->h = XDisplayHeight(display, DefaultScreen(display));
            for (int i = 0; i < DesktopCount; ++i) {
                monitors->desktops[i].wx = monitors->x;
                monitors->desktops[i].wy = monitors->y;
                monitors->desktops[i].ww = monitors->w;
                monitors->desktops[i].wh = monitors->h;
                monitors->desktops[i].isDynamic = False;
                monitors->desktops[i].showTopbars = True;
                monitors->desktops[i].masters = settings.masters;
                monitors->desktops[i].split = settings.split;
            }
            monitors->activeDesktop = 0;
        }
    }
    for (Monitor *it = monitors; it; it = it->next)
        ILog("monitor %d: (%d, %d) [%d x %d]",
                it->id, it->x, it->y, it->w, it->h);

    if (dirty)
        SetActiveMonitor(monitors);

    return dirty;
}

void
CleanupMonitors()
{
    /* release monitors */
    Monitor *m = monitors;
    while (m) {
        Monitor *p = m->next;
        free(m);
        m = p;
    }
    monitors = NULL;
}

Monitor *
NextMonitor(Monitor *m)
{
    return m->next ? m->next : monitors;
}

Monitor *
PreviousMonitor(Monitor *m)
{
    Monitor *it = NULL;
    for (it = monitors; it && it->next != m; it = it->next);
    return it ? it : monitors;
}

Monitor *
MonitorContaining(int x, int y)
{
    for (Monitor *m = monitors; m; m = m->next) {
        if (x > m->x && x < m->x + m->w 
                && y > m->y && y < m->y + m->h) {
            return m;
        }
    }
    return NULL;
}

void
AttachClientToMonitor(Monitor *m, Client *c)
{
    c->monitor = m;
    if (c->transfor && c->transfor != m->head)
        StackClientBefore(m, c, c->transfor);
    else 
        StackClientFront(m, c);

    MoveClientToDesktop(c, m->activeDesktop);
}

void
DetachClientFromMonitor(Monitor *m, Client *c)
{
    int desktop = c->desktop;
    Desktop *d = &m->desktops[desktop];

    if (c->monitor != m)
        return;

    if (c->sprev)
          c->sprev->snext = c->snext;
    else
        m->head = c->snext;

    if (c->snext)
        c->snext->sprev = c->sprev;
    else
        m->tail = c->sprev;

    c->snext = NULL;
    c->sprev = NULL;

    if (c->strut.right || c->strut.left || c->strut.top || c->strut.bottom) {
        d->wx = m->x;
        d->wy = m->y;
        d->ww = m->w;
        d->wh = m->h;
        for (Client *mc = m->head; mc; mc = mc->snext) {
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
                    && m->desktops[m->activeDesktop].isDynamic)))
        RefreshMonitor(m);
}

void
ShowMonitorDesktop(Monitor *m, int desktop)
{
    if (desktop < 0 || desktop >= DesktopCount || m->activeDesktop == desktop)
        return;

    m->desktops[m->activeDesktop].activeOnLeave = activeClient;
    m->activeDesktop = desktop;

    /* assign all stickies to this desktop */
    for (Client *c = m->head; c; c = c->snext)
        if (c->states & NetWMStateSticky)
            MoveClientToDesktop(c, desktop);

    SetActiveClient(m->desktops[m->activeDesktop].activeOnLeave);
    RefreshMonitor(m);
    XChangeProperty(display, root, atoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktop, 1);

}

void
ShowNextMonitorDesktop(Monitor *m)
{
    ShowMonitorDesktop(m, m->activeDesktop == DesktopCount - 1 ?
            0 : m->activeDesktop + 1);
}

void
ShowPreviousMonitorDesktop(Monitor *m)
{
    ShowMonitorDesktop(m, m->activeDesktop == 0 ?
            DesktopCount -1 : m->activeDesktop - 1);
}

void
SetMonitorDesktopDynamic(Monitor *m, int desktop, Bool b)
{
    m->desktops[desktop].isDynamic = b;
    /* Restore all windows */
    if (!b)
        for (Client *c = m->head; c; c = c->snext)
            if (c->desktop == desktop)
                UntileClient(c);
    RefreshMonitor(m);
}

void
ToggleMonitorDesktopDynamic(Monitor *m, int desktop)
{
    SetMonitorDesktopDynamic(m, desktop, !m->desktops[desktop].isDynamic);
}

void
SetMonitorDynamic(Monitor *m, Bool b)
{
    for (int i = 0; i < DesktopCount; ++i)
        SetMonitorDesktopDynamic(m, i, b);
}

void
ToggleMonitorDynamic(Monitor *m)
{
    for (int i = 0; i < DesktopCount; ++i)
        ToggleMonitorDesktopDynamic(m, i);
}

void
SetMonitorDesktopMasterCount(Monitor *m, int desktop, int count)
{
    Desktop *d = &m->desktops[desktop];
    d->masters = count > 0 ? count : 0;
    if (d->isDynamic)
        RefreshMonitor(m);
}

void
IncreaseMonitorDesktopMasterCount(Monitor *m, int desktop)
{
    Desktop *d = &m->desktops[desktop];
    SetMonitorDesktopMasterCount(m, desktop, d->masters + 1);
}

void
DecreaseMonitorDesktopMasterCount(Monitor *m, int desktop)
{
    Desktop *d = &m->desktops[desktop];
    SetMonitorDesktopMasterCount(m, desktop, d->masters - 1);
}

void
SetMonitorMasterCount(Monitor *m, int count)
{
    for (int i = 0; i < DesktopCount; ++i)
        SetMonitorDesktopMasterCount(m, i, count);
}

void
IncreaseMonitorMasterCount(Monitor *m)
{
    for (int i = 0; i < DesktopCount; ++i)
        IncreaseMonitorDesktopMasterCount(m, i);
}

void DecreaseMonitorMasterCount(Monitor *m)
{
    for (int i = 0; i < DesktopCount; ++i)
        DecreaseMonitorDesktopMasterCount(m, i);
}

void
SetMonitorDesktopTopbar(Monitor *m, int desktop, Bool b)
{
    m->desktops[desktop].showTopbars = b;
    for (Client *c = m->head; c; c = c->snext) {
        if (c->desktop == desktop)
            SetClientTopbarVisible(c, b);
        RefreshClient(c);
    }
    RefreshMonitor(m);
}

void
ToggleMonitorDesktopTopbar(Monitor *m, int desktop)
{
    SetMonitorDesktopTopbar(m, desktop, !m->desktops[desktop].showTopbars);
}

void
SetMonitorTopbar(Monitor *m, Bool b)
{
    for (int i = 0; i < DesktopCount; ++i)
        SetMonitorDesktopTopbar(m, i, b);
}

void
ToggleMonitorTopbar(Monitor *m)
{
    for (int i = 0; i < DesktopCount; ++i)
        ToggleMonitorDesktopTopbar(m, i);
}

void 
StackClientAfter(Monitor *m, Client *c, Client *after)
{
    if (after && (c == after || c->monitor != m || after->monitor != m))
        return;

    /* Remove c if linked */
    if (c->sprev || c->snext) {
        if (c->sprev)
              c->sprev->snext = c->snext;
        else
            m->head = c->snext;

        if (c->snext)
            c->snext->sprev = c->sprev;
        else
            m->tail = c->sprev;
    }

    if (after == m->tail) {
        StackClientBack(m, c);
    } else {
        c->snext = after->snext;
        c->sprev = after;
        after->snext->sprev = c;
        after->snext = c;
    }
}

void
StackClientBefore(Monitor *m, Client *c, Client *before)
{
    if (before && (c == before || c->monitor != m || before->monitor != m))
        return;

    /* remove c if linked */
    if (c->sprev || c->snext) {
        if (c->sprev)
              c->sprev->snext = c->snext;
        else
            m->head = c->snext;

        if (c->snext)
            c->snext->sprev = c->sprev;
        else
            m->tail = c->sprev;
    }

    if (before == c->monitor->head) {
        StackClientFront(m, c);
    } else {
        c->snext = before;
        c->sprev = before->sprev;
        before->sprev->snext = c;
        before->sprev = c;
    }
}

void
StackClientDown(Monitor *m, Client *c)
{
    if (c->transfor || c->monitor != m)
        return;

    Client *after = NULL;
    for (after = c->snext;
            after && (after->desktop != c->desktop
                || !(after->types & NetWMTypeNormal)
                ||  after->states & NetWMStateHidden);
            after = after->snext);

    if (after) {
        StackClientAfter(m, c, after);
        if (m->desktops[c->desktop].isDynamic)
            RefreshMonitor(c->monitor);
        return;
    }

    Client *before = NULL;
    for (before = c->monitor->head;
            before && (before->desktop != c->desktop
                || !(before->types & NetWMTypeNormal)
                ||  before->states & NetWMStateHidden);
            before = before->snext);

    if (before) {
        StackClientBefore(m, c, before);
        if (m->desktops[c->desktop].isDynamic)
            RefreshMonitor(c->monitor);
    }
}

void
StackClientUp(Monitor *m, Client *c)
{
    if (c->transfor || c->monitor != m)
        return;

    Client *before = NULL;
    for (before = c->sprev;
                    before && (before->desktop != c->desktop
                    || !(before->types & NetWMTypeNormal)
                    ||  before->states & NetWMStateHidden);
            before = before->sprev);

    if (before) {
        StackClientBefore(m, c, before);
        if (m->desktops[c->desktop].isDynamic)
            RefreshMonitor(m);
        return;
    }

    Client *after = NULL;
    for (after = c->monitor->tail;
                    after && (after->desktop != c->desktop
                    || !(after->types & NetWMTypeNormal)
                    ||  after->states & NetWMStateHidden);
            after = after->sprev);

    if (after) {
        StackClientAfter(m, c, after);
        if (m->desktops[c->desktop].isDynamic)
            RefreshMonitor(c->monitor);
    }
}

void
RefreshMonitor(Monitor *m)
{
    /* hide clients */
    for (Client *c = m->head; c; c = c->snext)
        HideClient(c);

    /* if isDynamic mode is enabled re-tile the desktop */
    if (m->desktops[m->activeDesktop].isDynamic) {
        XEvent e;
        Client *c;
        int n = 0, mw = 0, i = 0, mx = 0, ty = 0;

        for (c = m->head; c; c = c->snext)
            if (c->desktop == m->activeDesktop
                    && !(c->types & NetWMTypeFixed)
                    && !c->transfor) {
                /* restore hidden client dynamic
                 * is our window switcher */
                if (! c->isVisible)
                    RestoreClient(c);
                n++;
            }

        Desktop *d = &m->desktops[m->activeDesktop];
        if (n > d->masters)
            mw = d->masters ? d->ww * d->split : 0;
        else
            mw = d->ww;

        for (c = m->head; c; c = c->snext) {
            if (c->desktop == m->activeDesktop
                    && !(c->types & NetWMTypeFixed)
                    && !(IsFixed(c->normals))
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
            }
        } 
        /* avoid having enter notify event changing active client */
        XSync(display, False);
        while (XCheckMaskEvent(display, EnterWindowMask, &e));
    } else {
        for (Client *c = m->tail; c; c = c->sprev)
            if (c->desktop == m->activeDesktop)
                ShowClient(c);
    }
}

Bool
IsXineramaScreenUnique(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
    while (n--)
        if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
                && unique[n].width == info->width && unique[n].height == info->height)
            return False;
    return True;
}

Bool
IsXRandRScreenUnique(XRRCrtcInfo *unique, size_t n, XRRCrtcInfo *info)
{
    while (n--)
        if (unique[n].x == info->x && unique[n].y == info->y
                && unique[n].width == info->width && unique[n].height == info->height)
            return False;
    return True;
}

Bool
XineramaScanMonitors()
{
    int i, j, n, nn;
    Bool dirty = False;
    Client *c;
    Monitor *m;
    XineramaScreenInfo *info = XineramaQueryScreens(display, &nn);
    XineramaScreenInfo *unique = NULL;

    for (n = 0, m = monitors; m; m = m->next, n++);
    /* only consider unique geometries as separate screens */
    unique = malloc(nn *  sizeof(XineramaScreenInfo));
    if (! unique) FLog("can't allocate memory for screen info");
    for (i = 0, j = 0; i < nn; i++)
        if (IsXineramaScreenUnique(unique, j, &info[i]))
            memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
    XFree(info);
    nn = j;
    if (n <= nn) { /* new monitors available */
        for (i = 0; i < (nn - n); i++) {
            Monitor *nm = malloc(sizeof(Monitor));
            if (!nm) FLog("could not allocate memory for output.");
            memset(nm, 0 , sizeof(Monitor));

            for (m = monitors; m && m->next; m = m->next);
            if (m)
                m->next = nm;
            else
                monitors = nm;
        }
        for (i = 0, m = monitors; i < nn && m; m = m->next, i++)
            if (i >= n || unique[i].x_org != m->x || unique[i].y_org !=   m->y
                    || unique[i].width != m->w || unique[i].height !=   m->h) {
                dirty = True;
                m->id = i;
                m->x = unique[i].x_org;
                m->y = unique[i].y_org;
                m->w = unique[i].width;
                m->h = unique[i].height;
                for (int j = 0; j < DesktopCount; ++j) {
                    m->desktops[j].wx = m->x;
                    m->desktops[j].wy = m->y;
                    m->desktops[j].ww = m->w;
                    m->desktops[j].wh = m->h;
                    m->desktops[j].isDynamic = False;
                    m->desktops[j].showTopbars = True;
                    m->desktops[j].masters = settings.masters;
                    m->desktops[j].split = settings.split;
                }
                m->activeDesktop = 0;
                              }
    } else { /* less monitors available nn < n */
        for (i = nn; i < n; i++) {
            for (m = monitors; m && m->next; m = m->next);
            while ((c = m->head)) {
                dirty = True;
                DetachClientFromMonitor(m, c);
                AttachClientToMonitor(monitors, c);
            }
            if (m == activeMonitor)
                SetActiveMonitor(monitors);

            if (m == monitors) {
                monitors = monitors->next;
            } else {
                Monitor *it;
                for (it = monitors; it && it->next != m; it = it->next);
                it->next = m->next;
            }
            free(m);
        }
    }
    free(unique);
    return dirty;
}

Bool
XRandRScanMonitors()
{
    int i, j, n, nn;
    Bool dirty = False;
    Client *c;
    Monitor *m;
    XRRScreenResources *sr;
    XRRCrtcInfo *unique = NULL;

    /* scan for monitors */
    sr = XRRGetScreenResources(display, root);

    for (n = 0, m = monitors; m; m = m->next, n++);
    /* only consider unique geometries as separate screens */
    unique = malloc(sr->ncrtc * sizeof(XRRCrtcInfo));
    if (! unique) FLog("can't allocate memory for screen info");
    for (i = 0, j = 0; i < sr->ncrtc; i++) {
        XRRCrtcInfo *ci = XRRGetCrtcInfo(display, sr, sr->crtcs[i]);
        if (ci != NULL && ci->noutput != 0 && IsXRandRScreenUnique(unique, j, ci))
            memcpy(&unique[j++], ci, sizeof(XRRCrtcInfo));
        XRRFreeCrtcInfo(ci);
    }
    XRRFreeScreenResources(sr);

    nn = j;
    if (n <= nn) { /* new monitors available */
        for (i = 0; i < (nn - n); i++) {
            Monitor *nm = malloc(sizeof(Monitor));
            if (!nm) FLog("could not allocate memory for output.");
            memset(nm, 0 , sizeof(Monitor));

            for (m = monitors; m && m->next; m = m->next);
            if (m)
                m->next = nm;
            else
                monitors = nm;
        }
        for (i = 0, m = monitors; i < nn && m; m = m->next, i++)
            if (i >= n || unique[i].x != m->x || unique[i].y !=   m->y
                    || (int)unique[i].width != m->w || (int)unique[i].height !=   m->h) {
                dirty = True;
                m->id = i;
                m->x = unique[i].x;
                m->y = unique[i].y;
                m->w = (int)unique[i].width;
                m->h = (int)unique[i].height;
                for (int j = 0; j < DesktopCount; ++j) {
                    m->desktops[j].wx = m->x;
                    m->desktops[j].wy = m->y;
                    m->desktops[j].ww = m->w;
                    m->desktops[j].wh = m->h;
                    m->desktops[j].isDynamic = False;
                    m->desktops[j].showTopbars = True;
                    m->desktops[j].masters = settings.masters;
                    m->desktops[j].split = settings.split;
                }
                m->activeDesktop = 0;
                              }
    } else { /* less monitors available nn < n */
        for (i = nn; i < n; i++) {
            for (m = monitors; m && m->next; m = m->next);
            while ((c = m->head)) {
                dirty = True;
                DetachClientFromMonitor(m, c);
                AttachClientToMonitor(monitors, c);
            }
            if (m == activeMonitor)
                SetActiveMonitor(monitors);

            if (m == monitors) {
                monitors = monitors->next;
            } else {
                Monitor *it;
                for (it = monitors; it && it->next != m; it = it->next);
                it->next = m->next;
            }
            free(m);
        }
    }
    free(unique);
    return dirty;
}

void
StackClientFront(Monitor *m, Client *c)
{
    if (c->monitor != m)
        return;

    if (m->head) {
        c->snext = m->head;
        m->head->sprev = c;
    }

    if (! m->tail)
        m->tail = c;

    m->head = c;
    c->sprev = NULL;
}

void
StackClientBack(Monitor *m, Client *c)
{
    if (c->monitor != m)
        return;

    if (m->tail) {
        c->sprev = m->tail;
        m->tail->snext = c;
    }

    if (! m->head)
        m->head = c;

    m->tail = c;
    c->snext = NULL;
}

