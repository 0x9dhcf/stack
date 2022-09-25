#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>

#include "client.h"
#include "config.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "stack.h"
#include "x11.h"

static Bool IsXineramaScreenUnique(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info);
static Bool IsXRandRScreenUnique(XRRCrtcInfo *unique, size_t n, XRRCrtcInfo *info);
static Bool XineramaScanMonitors();
static Bool XRandRScanMonitors();

Monitor *monitors = NULL;

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
                    m->desktops[j].dynamic = False;
                    m->desktops[j].toolbar = True;
                    m->desktops[j].masters = config.masters;
                    m->desktops[j].split = config.split;
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
                activeMonitor = monitors;

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
                    m->desktops[j].dynamic = False;
                    m->desktops[j].toolbar = True;
                    m->desktops[j].masters = config.masters;
                    m->desktops[j].split = config.split;
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
                activeMonitor = monitors;

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
SetupMonitors()
{
    Bool dirty = False;

    if (extensions & ExtentionXRandR) {
        ILog("using xrandr");
        dirty = XRandRScanMonitors();
    } else if (extensions & ExtentionXinerama) {
    //if (extensions & ExtentionXinerama) {
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
            //dirty = 1;
            monitors->x = 0;
            monitors->y = 0;
            monitors->w = DisplayWidth(display, DefaultScreen(display));
            monitors->h = XDisplayHeight(display, DefaultScreen(display));
            for (int i = 0; i < DesktopCount; ++i) {
                monitors->desktops[i].wx = monitors->x;
                monitors->desktops[i].wy = monitors->y;
                monitors->desktops[i].ww = monitors->w;
                monitors->desktops[i].wh = monitors->h;
                monitors->desktops[i].dynamic = False;
                monitors->desktops[i].toolbar = True;
                monitors->desktops[i].masters = config.masters;
                monitors->desktops[i].split = config.split;
            }
            monitors->activeDesktop = 0;
        }
    }
    for (Monitor *it = monitors; it; it = it->next)
        ILog("monitor %d: (%d, %d) [%d x %d]", it->id, it->x, it->y, it->w, it->h);

    if (dirty) {
        activeMonitor = monitors;
    }
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
    MoveClientToDesktop(c, m->activeDesktop);
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
ShowDesktop(Monitor *m, int desktop)
{
    if (desktop < 0 || desktop >= DesktopCount || m->activeDesktop == desktop)
        return;

    // XXX: we shouldn't have to refer to activeClient
    m->desktops[m->activeDesktop].activeOnLeave = activeClient;
    m->activeDesktop = desktop;

    /* assign all stickies to this desktop */
    for (Client *c = m->head; c; c = c->next)
        if ((c->states & NetWMStateSticky) || (c->types & NetWMTypeFixed))
            MoveClientToDesktop(c, desktop);

    RestackMonitor(m);
    SetActiveClient(m->desktops[m->activeDesktop].activeOnLeave);
    XChangeProperty(display, root, atoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktop, 1);
}

void
SwitchToNextDesktop(Monitor *m)
{
    ShowDesktop(m, m->activeDesktop == DesktopCount-1 ? 0 : m->activeDesktop + 1);
}

void
SwitchToPreviousDesktop(Monitor *m)
{
    ShowDesktop(m, m->activeDesktop == 0 ? DesktopCount-1 : m->activeDesktop - 1);
}

void
ToggleDynamic(Monitor *m)
{
    Bool b = m->desktops[m->activeDesktop].dynamic;
    m->desktops[m->activeDesktop].dynamic = ! b;
    /* Untile, restore all windows */
    if (b)
        for (Client *c = m->head; c; c = c->next)
            if (c->desktop == m->activeDesktop)
                UntileClient(c);
    RestackMonitor(m);
}

void
ToggleTopbar(Monitor *m)
{
    Bool b = m->desktops[m->activeDesktop].toolbar;
    m->desktops[m->activeDesktop].toolbar = ! b;
    for (Client *c = m->head; c; c = c->next) {
        if (c->desktop == m->activeDesktop)
            SetClientTopbarVisible(c, !b);
        RefreshClient(c);
    }
    RestackMonitor(m);
}

void
AddMaster(Monitor *m, int nb)
{
    if (m->desktops[m->activeDesktop].dynamic) {
        m->desktops[m->activeDesktop].masters =
            Max(m->desktops[m->activeDesktop].masters + nb, 1);
        RestackMonitor(m);
    }
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
