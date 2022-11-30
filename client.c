#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <pango/pangocairo.h>

#include "client.h"
#include "hints.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "pango/pango-types.h"
#include "settings.h"
#include "x11.h"

/* icons are 40% of the button size */
#define IconScaleFactor 0.4f

static void ApplyNormalHints(Client *c);
static void SaveGeometries(Client *c);
static void GetTopbarGeometry(Client *c, int *x, int *y, int *w, int *h);
static void GetButtonGeometry(Client *c, int button, int *x, int *y, int *w, int *h);
static void SetSourceColor(cairo_t *cairo, int color);
static void DrawFrame(Client *c, cairo_t *cairo);
static void WriteTitle(Client *c, cairo_t *cairo);
static void DrawButton(Client *c, cairo_t *cairo, int button);

void
HideClient(Client *c)
{
    /* move all windows off screen without changing anything */
    XMoveWindow(display, c->frame, -c->fw, c->fy);
    if (c->hasHandles)
        for (int i = 0; i < HandleCount; ++i)
            XMoveWindow(display, c->handles[i], -c->fw, c->fy);
}

void
ShowClient(Client *c)
{
    if ((c->types & NetWMTypeFixed) || IsFixed(c->normals))
        MoveResizeClientFrame(c, c->fx, c->fy, c->fw, c->fh, False);
    else
        MoveResizeClientFrame(c,
                Max(c->fx, c->monitor->desktops[c->desktop].wx),
                Max(c->fy, c->monitor->desktops[c->desktop].wy),
                Min(c->fw, c->monitor->desktops[c->desktop].ww),
                Min(c->fh, c->monitor->desktops[c->desktop].wh), False);

    if (c->states & NetWMStateFullscreen)
        RaiseClient(c);

    RefreshClient(c);
}

void
MoveClientWindow(Client *c, int x, int y)
{
    c->wx = x;
    c->wy = y;
    SynchronizeFrameGeometry(c);
    Configure(c);
}

void
ResizeClientWindow(Client *c, int w, int h, Bool sh)
{
    if (w < 1 || h < 1)
        return;

    c->ww = w;
    c->wh = h;

    if (sh)
        ApplyNormalHints(c);

    SynchronizeFrameGeometry(c);
    Configure(c);
}

void
MoveResizeClientWindow(Client *c, int x, int y, int w, int h, Bool sh)
{
    if (w < 1 || h < 1)
        return;

    c->wx = x;
    c->wy = y;
    c->ww = w;
    c->wh = h;

    if (sh)
        ApplyNormalHints(c);

    SynchronizeFrameGeometry(c);
    Configure(c);
}

void
MoveClientFrame(Client *c, int x, int y)
{
    c->fx = x;
    c->fy = y;
    SynchronizeWindowGeometry(c);
    Configure(c);
}

void
ResizeClientFrame(Client *c, int w, int h, Bool sh)
{
    int mw, mh;
    mw = mh = 1;
    if (c->isBorderVisible) {
        mw += 2 * settings.borderWidth;
        mh += 2 * settings.borderWidth;
    }

    if (c->hasTopbar && c->isTopbarVisible) {
        mw += ButtonCount * (settings.buttonSize + settings.buttonGap);
        mh += settings.topbarHeight;;
    }

    if (w < mw || h < mh)
        return;

    c->fw = w;
    c->fh = h;

    SynchronizeWindowGeometry(c);
    if (sh) {
        ApplyNormalHints(c);
        SynchronizeFrameGeometry(c);
    }
    SynchronizeWindowGeometry(c);
    Configure(c);
}

void
MoveResizeClientFrame(Client *c, int x, int y, int w, int h, Bool sh)
{
    int mw, mh;
    mw = mh = 1;
    if (c->isBorderVisible) {
        mw += 2 * settings.borderWidth;
        mh += 2 * settings.borderWidth;
    }

    if (c->hasTopbar && c->isTopbarVisible) {
        mw += ButtonCount * (settings.buttonSize + settings.buttonGap);
        mh += settings.topbarHeight;
    }

    if (w < mw || h < mh)
        return;

    c->fx = x;
    c->fy = y;
    c->fw = w;
    c->fh = h;

    SynchronizeWindowGeometry(c);
    if (sh) {
        ApplyNormalHints(c);
        SynchronizeFrameGeometry(c);
    }
    Configure(c);
}

void
TileClient(Client *c, int x, int y, int w, int h)
{
    SaveGeometries(c);
    c->isTiled = True;
    MoveResizeClientFrame(c, x, y, w, h, False);
}

void
UntileClient(Client *c)
{
    c->isTiled = False;
    MoveResizeClientFrame(c, c->stx, c->sty, c->stw, c->sth, False);
}

void
MaximizeClientHorizontally(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateMaximizedHorz;
        MoveResizeClientFrame(c, c->monitor->desktops[c->desktop].wx, c->fy,
                c->monitor->desktops[c->desktop].ww, c->fh, False);
        SetNetWMStates(c->window, c->states);
    }
}

void
MaximizeClientVertically(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateMaximizedVert;
        MoveResizeClientFrame(c, c->fx, c->monitor->desktops[c->desktop].wy,
                c->fw, c->monitor->desktops[c->desktop].wh, False);
        SetNetWMStates(c->window, c->states);
    }
}

void
MaximizeClient(Client *c)
{
    MaximizeClientVertically(c);
    MaximizeClientHorizontally(c);
}

void
MaximizeClientLeft(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateMaximizedVert;
        c->states &= ~NetWMStateMaximizedHorz;
        MoveResizeClientFrame(c, c->monitor->desktops[c->desktop].wx,
                c->monitor->desktops[c->desktop].wy,
                c->monitor->desktops[c->desktop].ww / 2,
                c->monitor->desktops[c->desktop].wh, False);
        SetNetWMStates(c->window, c->states);
    }
}

void
MaximizeClientRight(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateMaximizedVert;
        c->states &= ~NetWMStateMaximizedHorz;
        MoveResizeClientFrame(c,
                c->monitor->desktops[c->desktop].wx
                    + c->monitor->desktops[c->desktop].ww / 2,
                c->monitor->desktops[c->desktop].wy,
                c->monitor->desktops[c->desktop].ww / 2,
                c->monitor->desktops[c->desktop].wh, False);
        SetNetWMStates(c->window, c->states);
    }
}

void
MaximizeClientTop(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateMaximizedHorz;
        c->states &= ~NetWMStateMaximizedVert;
        MoveResizeClientFrame(c, c->monitor->desktops[c->desktop].wx,
                c->monitor->desktops[c->desktop].wy,
                c->monitor->desktops[c->desktop].ww,
                c->monitor->desktops[c->desktop].wh / 2, False);
        SetNetWMStates(c->window, c->states);
    }
}

void
MaximizeClientBottom(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateMaximizedHorz;
        c->states &= ~NetWMStateMaximizedVert;
        MoveResizeClientFrame(c, c->monitor->desktops[c->desktop].wx,
                c->monitor->desktops[c->desktop].wy
                    + c->monitor->desktops[c->desktop].wh / 2,
                c->monitor->desktops[c->desktop].ww,
                c->monitor->desktops[c->desktop].wh / 2,
                False);
        SetNetWMStates(c->window, c->states);
    }
}

void
MinimizeClient(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !c->isTiled) {
        SaveGeometries(c);
        c->states |= NetWMStateHidden;
        MoveResizeClientFrame(c, c->monitor->x, c->monitor->y + c->monitor->h,
                c->fw, c->fw, False);
        SetNetWMStates(c->window, c->states);
    }
}

void
FullscreenClient(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->isTiled) {
        SaveGeometries(c);

        c->isBorderVisible = False;
        c->isTopbarVisible = False;
        c->states |= NetWMStateFullscreen;

        MoveResizeClientWindow(c, c->monitor->x, c->monitor->y, c->monitor->w,
                c->monitor->h, False);

        SetNetWMStates(c->window, c->states);
        RaiseClient(c);
    }
}

void
MoveClientLeftmost(Client *c)
{
    if (!(c->states & (NetWMStateMaximized | NetWMStateFullscreen))
            && !(c->types & NetWMTypeFixed) && !c->isTiled) {
        Desktop *d = &c->monitor->desktops[c->desktop];
        MoveClientFrame(c, d->wx, c->fy);
    }
}

void
MoveClientRightmost(Client *c)
{
    if (!(c->states & (NetWMStateMaximized | NetWMStateFullscreen))
            && !(c->types & NetWMTypeFixed) && !c->isTiled) {
        Desktop *d = &c->monitor->desktops[c->desktop];
        MoveClientFrame(c, d->wx + d->ww - c->fw, c->fy);
    }
}

void
MoveClientTopmost(Client *c)
{
    if (!(c->states & (NetWMStateMaximized | NetWMStateFullscreen))
            && !(c->types & NetWMTypeFixed) && !c->isTiled) {
        Desktop *d = &c->monitor->desktops[c->desktop];
        MoveClientFrame(c, c->fx, d->wy);
    }
}

void
MoveClientBottommost(Client *c)
{
    if (!(c->states & (NetWMStateMaximized | NetWMStateFullscreen))
            && !(c->types & NetWMTypeFixed) && !c->isTiled) {
        Desktop *d = &c->monitor->desktops[c->desktop];
        MoveClientFrame(c, c->fx, d->wy + d->wh - c->fh);
    }
}

void
CenterClient(Client *c)
{
    if (!(c->states & (NetWMStateMaximized | NetWMStateFullscreen))
            && !(c->types & NetWMTypeFixed) && !c->isTiled) {
        Desktop *d = &c->monitor->desktops[c->desktop];
        MoveClientFrame(c, d->wx + (d->ww - c->fw) / 2,
                d->wy + (d->wh - c->fh) / 2);
    }
}

void
RestoreClient(Client *c)
{
    /* do not restore any tiled window */
    if (c->isTiled)
        return;

    if (c->states & NetWMStateFullscreen) {
        c->isBorderVisible = True;
        c->isTopbarVisible = True;
        c->states &= ~NetWMStateFullscreen;
        MoveResizeClientFrame(c, c->sfx, c->sfy, c->sfw, c->sfh, False);
    } else if (c->states & NetWMStateHidden) {
        c->states &= ~NetWMStateHidden;
        MoveResizeClientFrame(c, c->shx, c->shy, c->shw, c->shh, False);
    } else if (c->states & NetWMStateMaximized) {
        c->states &= ~NetWMStateMaximized;
        MoveResizeClientFrame(c, c->smx, c->smy, c->smw, c->smh, False);
    }

    SetNetWMStates(c->window, c->states);
}

void
RaiseClient(Client *c)
{
    c->states |= NetWMStateAbove;
    c->states &= ~NetWMStateBelow;
    XRaiseWindow(display, c->frame);
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals))
        for (int i = 0; i < HandleCount; ++i)
            XRaiseWindow(display, c->handles[i]);

    for (Transient *tc = c->transients; tc; tc = tc->next)
        RaiseClient(tc->client);

    SetNetWMStates(c->window, c->states);
}

void
LowerClient(Client *c)
{
    c->states |= NetWMStateBelow;
    c->states &= ~NetWMStateAbove;
    XLowerWindow(display, c->frame);
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals))
        for (int i = 0; i < HandleCount; ++i)
            XLowerWindow(display, c->handles[i]);

    for (Transient *tc = c->transients; tc; tc = tc->next)
        LowerClient(tc->client);

    SetNetWMStates(c->window, c->states);
}

void
SetClientActive(Client *c, Bool b)
{
    unsigned int modifiers[] = { 0, LockMask, numLockMask, numLockMask|LockMask };

    if (b && !c->isActive) {
        if (c->hints & HintsFocusable)
            XSetInputFocus(display, c->window, RevertToPointerRoot, CurrentTime);
        else if (c->protocols & NetWMProtocolTakeFocus)
            SendMessage(c->window, atoms[AtomWMTakeFocus]);

        c->states &= ~NetWMStateDemandsAttention;
        c->hints &= ~HintsUrgent;
        c->isActive = b;

        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; ++i)
                XGrabButton(display, Button1, Modkey | modifiers[i], c->window,
                        False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                        GrabModeAsync, GrabModeSync, None, None);

    }

    if (!b && c->isActive) {
        c->isActive = b;
        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; i++)
                XUngrabButton(display, Button1, Modkey | modifiers[i], c->window);
    }

    RefreshClient(c);
}

void
SetClientTopbarVisible(Client *c, Bool b)
{
    c->isTopbarVisible = b;
    SynchronizeWindowGeometry(c);
    Configure(c);
}

void
ToggleClientTopbar(Client *c)
{
    SetClientTopbarVisible(c, !c->isTopbarVisible);
}

void
KillClient(Client *c)
{
    if (c->protocols & NetWMProtocolDeleteWindow) {
        SendMessage(c->window, atoms[AtomWMDeleteWindow]);
    } else {
        XGrabServer(display);
        XSetErrorHandler(DisableErrorHandler);
        XSetCloseDownMode(display, DestroyAll);
        XKillClient(display, c->window);
        XSync(display, False);
        XSetErrorHandler(EnableErrorHandler);
        XUngrabServer(display);
    }
}

void
RefreshClient(Client *c)
{
    Visual *v;
    cairo_surface_t *surface;  
    cairo_t *cairo; 

    /* do not attempt to refresh non decorated or hidden clients */
    if ((!c->isTopbarVisible && !c->isBorderVisible)
            || (c->desktop != c->monitor->activeDesktop
                && !(c->states & NetWMStateSticky)))
        return;

    v = DefaultVisual(display, DefaultScreen(display));
    surface = cairo_xlib_surface_create(display, c->frame, v, c->fw, c->fh);
    cairo = cairo_create(surface);
    cairo_set_line_cap(cairo, CAIRO_LINE_CAP_ROUND);

    DrawFrame(c, cairo);

    if (c->hasTopbar && c->isTopbarVisible) {
        WriteTitle(c, cairo);
        for (int i = 0; i < ButtonCount; ++i) {
            DrawButton(c, cairo, i);
        }
    }

    cairo_destroy(cairo);
    cairo_surface_destroy(surface);
}

void
StackClientAfter(Client *c, Client *after)
{
    if (c == after)
        return;

    /* Remove c */
    if (c->prev)
          c->prev->next = c->next;
    else
        c->monitor->head = c->next;

    /* Change monitor is needed */
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
StackClientBefore(Client *c, Client *before)
{
    if (c == before)
        return;

    /* Remove c */
    if (c->prev)
          c->prev->next = c->next;
    else
        c->monitor->head = c->next;

    /* Change monitor is needed */
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
StackClientDown(Client *c)
{
    if (c->transfor)
        return;

    // XXX: WHY??
    if (c->monitor->desktops[c->monitor->activeDesktop].dynamic) {

        Client *after = NULL;
        for (after = c->next;
                        after && (after->desktop != c->desktop
                        || !(after->types & NetWMTypeNormal)
                        ||  after->states & NetWMStateHidden);
                after = after->next);

        if (after) {
            StackClientAfter(c, after);
            RestackMonitor(c->monitor);
            return;
        }

        Client *before = NULL;
        for (before = c->monitor->head;
                        before && (before->desktop != c->desktop
                        || !(before->types & NetWMTypeNormal)
                        ||  before->states & NetWMStateHidden);
                before = before->next);

        if (before) {
            StackClientBefore(c, before);
            RestackMonitor(c->monitor);
        }
    }
}

void
StackClientUp(Client *c)
{
    if (c->transfor)
        return;

    if (c->monitor->desktops[c->monitor->activeDesktop].dynamic) {

        Client *before = NULL;
        for (before = c->prev;
                        before && (before->desktop != c->desktop
                        || !(before->types & NetWMTypeNormal)
                        ||  before->states & NetWMStateHidden);
                before = before->prev);

        if (before) {
            StackClientBefore(c, before);
            RestackMonitor(activeMonitor);
            return;
        }

        Client *after = NULL;
        for (after = c->monitor->tail;
                        after && (after->desktop != c->desktop
                        || !(after->types & NetWMTypeNormal)
                        ||  after->states & NetWMStateHidden);
                after = after->prev);

        if (after) {
            StackClientAfter(c, after);
            RestackMonitor(c->monitor);
        }
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
MoveClientToDesktop(Client *c, int desktop)
{
    int from = c->desktop;
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

    c->isTopbarVisible = d->toolbar;
    c->desktop = desktop;
    if (c->isTiled && !d->dynamic) {
        UntileClient(c);
        RestackMonitor(c->monitor);
    }

    if (!(c->types & NetWMTypeFixed))
        MoveResizeClientFrame(c,
                Max(c->fx, c->monitor->desktops[c->desktop].wx),
                Max(c->fy, c->monitor->desktops[c->desktop].wy),
                Min(c->fw, c->monitor->desktops[c->desktop].ww),
                Min(c->fh, c->monitor->desktops[c->desktop].wh), False);

    if (from != -1) { /* it's probably a new window not affected yet */
        SetActiveClient(NULL);
        RestackMonitor(m);
    }

    /* finally let the pager know where we are */
    XChangeProperty(display, c->window, atoms[AtomNetWMDesktop], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char*)&c->desktop, 1);
}

void
MoveClientToNextDesktop(Client *c)
{
    MoveClientToDesktop(c, c->desktop + 1);
}

void
MoveClientToPreviousDesktop(Client *c)
{
    MoveClientToDesktop(c, c->desktop - 1);
}

void
MoveClientToMonitor(Client *c, Monitor *m)
{
    int x, y;
    x = c->fx - c->monitor->desktops[c->desktop].wx + m->desktops[m->activeDesktop].wx;
    y = c->fy - c->monitor->desktops[c->desktop].wy + m->desktops[m->activeDesktop].wy;
    DetachClientFromMonitor(c->monitor, c);
    AttachClientToMonitor(m, c);
    MoveClientFrame(c, x, y);
    SetActiveClient(NULL);
}

void
MoveClientToNextMonitor(Client *c)
{
    if (c->monitor->next)
        MoveClientToMonitor(c, c->monitor->next);
}

void
MoveClientToPreviousMonitor(Client *c)
{
    Monitor *m = NULL;
    for (m = monitors; m && m->next != c->monitor; m = m->next);
    if (m)
        MoveClientToMonitor(c, m);
}

void
ApplyNormalHints(Client *c)
{
    int baseismin = (c->normals.bw == c->normals.minw)
        && (c->normals.bh == c->normals.minh);

    /* temporarily remove base dimensions, ICCCM 4.1.2.3 */
    if (!baseismin) {
        c->ww -= c->normals.bw;
        c->wh -= c->normals.bh;
    }

    /* adjust for aspect limits */
    if (c->normals.mina && c->normals.maxa) {
        if (c->normals.maxa < (float)c->ww / c->wh)
            c->ww = c->wh * c->normals.maxa;
        else if (c->normals.mina < (float)c->wh / c->ww)
            c->wh = c->ww * c->normals.mina;
    }

    if (c->normals.incw && c->normals.inch) {
        /* remove base dimensions for increment */
        if (baseismin) {
            c->ww -= c->normals.bw;
            c->wh -= c->normals.bh;
        }

        /* adjust for increment value */
        c->ww -= c->ww % c->normals.incw;
        c->wh -= c->wh % c->normals.inch;

        /* restore base dimensions */
        c->ww += c->normals.bw;
        c->wh += c->normals.bh;
    }

    /* adjust for min max width/height */
    c->ww = c->ww < c->normals.minw ? c->normals.minw : c->ww;
    c->wh = c->wh < c->normals.minh ? c->normals.minh : c->wh;
    c->ww = c->ww > c->normals.maxw ? c->normals.maxw : c->ww;
    c->wh = c->wh > c->normals.maxh ? c->normals.maxh : c->wh;
}

void
Configure(Client *c)
{
    int wx, wy, bw;
    XEvent ce;

    /* what is the border width */
    bw = c->isBorderVisible ? settings.borderWidth : 0;

    /* compute the relative window's position */
    wx = c->wx - c->fx;
    wy = c->wy - c->fy;

    /* place frame and window */
    XMoveResizeWindow(display, c->frame, c->fx, c->fy, c->fw, c->fh);
    XMoveResizeWindow(display, c->window, wx, wy, c->ww, c->wh);

    /* place the topbar */
    if (c->hasTopbar) {
        if (c->isTopbarVisible) {
            int x, y, w, h;
            GetTopbarGeometry(c, &x, &y, &w, &h);
            XMoveResizeWindow(display, c->topbar, x, y, w, h);
            for (int i = 0; i < ButtonCount; ++i) {
                GetButtonGeometry(c, i, &x, &y, &w, &h);
                XMoveResizeWindow(display, c->buttons[i], x, y, w, h); 
            }
        } else {
            /* move the topbar outside the frame */
            XMoveWindow(display, c->topbar, bw, -(bw + settings.topbarHeight));
        }
    }

    /* suround frame by handles */
    if (c->hasHandles) {
        int hw = settings.handleWidth;
        XMoveResizeWindow(display, c->handles[HandleNorth],
                c->fx, c->fy - hw, c->fw, hw);
        XMoveResizeWindow(display, c->handles[HandleNorthWest],
                c->fx + c->fw, c->fy - hw, hw, hw);
        XMoveResizeWindow(display, c->handles[HandleWest],
                c->fx + c->fw, c->fy, hw, c->fh);
        XMoveResizeWindow(display, c->handles[HandleSouthWest],
                c->fx + c->fw, c->fy + c->fh, hw, hw);
        XMoveResizeWindow(display, c->handles[HandleSouth],
                c->fx, c->fy + c->fh, c->fw, hw);
        XMoveResizeWindow(display, c->handles[HandleSouthEast],
                c->fx - hw, c->fy + c->fh, hw, hw);
        XMoveResizeWindow(display, c->handles[HandleEast],
                c->fx - hw, c->fy, hw, c->fh);
        XMoveResizeWindow(display, c->handles[HandleNorthEast],
                c->fx - hw, c->fy - hw, hw, hw);
    }
    RefreshClient(c);

    /* synchronize transients */
    Desktop *d = &c->monitor->desktops[c->desktop];
    for (Transient *t = c->transients; t; t = t->next) {
        int x, y, w, h;
        w = Min(d->ww, t->client->fw);
        h = Min(d->wh, t->client->fh);
        x = Min(Max(d->wx, c->fx + (c->fw - t->client->fw) / 2), d->wx + d->ww - w);
        y = Min(Max(d->wy, c->fy + (c->fh - t->client->fh) / 2), d->wy + d->wh - h);
        MoveResizeClientFrame(t->client, x, y, w, h, False);
        RefreshClient(t->client);
    }

    /* let anybody knows about the changes */
    memset(&ce, 0, sizeof(ce));
    ce.xconfigure.display = display;
    ce.xconfigure.type = ConfigureNotify;
    ce.xconfigure.event = c->window;
    ce.xconfigure.window = c->window;
    ce.xconfigure.x = c->wx;
    ce.xconfigure.y = c->wy;
    ce.xconfigure.width = c->ww;
    ce.xconfigure.height = c->wh;
    ce.xconfigure.border_width = c->isBorderVisible ? settings.borderWidth : 0;
    ce.xconfigure.above = None;
    ce.xconfigure.override_redirect = False;
    XSendEvent(display, c->window, False, StructureNotifyMask, &ce);
    XSync(display, False);
}

void
SynchronizeFrameGeometry(Client *c)
{
    int bw = c->isBorderVisible ? settings.borderWidth : 0;
    int th = (c->hasTopbar && c->isTopbarVisible) ? settings.topbarHeight : 0;
    c->fx = c->wx - bw;
    c->fy = c->wy - bw - th;
    c->fw = c->ww + 2 * bw;
    c->fh = c->wh + 2 * bw + th;
}

void
SynchronizeWindowGeometry(Client *c)
{
    int bw = c->isBorderVisible ? settings.borderWidth : 0;
    int th = (c->hasTopbar && c->isTopbarVisible) ? settings.topbarHeight : 0;
    c->wx = c->fx + bw;
    c->wy = c->fy + bw + th;
    c->ww = c->fw - 2 * bw;
    c->wh = c->fh - 2 * bw - th;
}

void
SaveGeometries(Client *c)
{
    if (!c->isTiled) {
        c->stx = c->fx;
        c->sty = c->fy;
        c->stw = c->fw;
        c->sth = c->fh;
    }
    if (!(c->states & NetWMStateFullscreen)) {
        c->sfx = c->fx;
        c->sfy = c->fy;
        c->sfw = c->fw;
        c->sfh = c->fh;
    }
    if (!(c->states & NetWMStateMaximized)) {
        c->smx = c->fx;
        c->smy = c->fy;
        c->smw = c->fw;
        c->smh = c->fh;
    }
    if (!(c->states & NetWMStateHidden)) {
        c->shx = c->fx;
        c->shy = c->fy;
        c->shw = c->fw;
        c->shh = c->fh;
    }
}

void
GetTopbarGeometry(Client *c, int *x, int *y, int *w, int *h)
{
    int bw = c->isBorderVisible ? settings.borderWidth : 0;
    *x = bw;
    *y = bw;
    *w = c->fw - 2 * bw;
    *h = settings.topbarHeight;
}

void
GetButtonGeometry(Client *c, int button, int *x, int *y, int *w, int *h)
{
    int bw = c->isBorderVisible ? settings.borderWidth : 0;
    int bs = settings.buttonSize;
    int bg = settings.buttonGap;
    int bh = settings.topbarHeight;
    *x = c->fw - 2 * bw - ((button+1) * (bs) + (button+1) * bg);
    *y = bw + (bh - bs) / 2;
    *w = bs;
    *h = bs;
}

void
SetSourceColor(cairo_t *cairo, int color)
{
    double r = ((unsigned char)(color>>16))/255.0;
    double g = ((unsigned char)(color>>8))/255.0;
    double b = ((unsigned char)color)/255.0;
    cairo_set_source_rgb(cairo, r, g, b);
}

void
WriteTitle(Client *c, cairo_t *cairo)
{
    int fg, bw, x, y, w, h;
    PangoLayout *layout;
    PangoFontDescription *desc;

    /* select the frame colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        fg = settings.urgentForeground;
    }  else if (c->isActive) {
        fg = settings.activeForeground;
    } else {
        fg = settings.inactiveForeground;
    }
        
    /* what is the border width */
    bw = c->isBorderVisible ? settings.borderWidth : 0;

    /* label */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_text(layout, c->name, -1);
    desc = pango_font_description_from_string(settings.labelFontname);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    pango_layout_get_size(layout, &w, &h);
    x = (c->fw - (double)w/PANGO_SCALE) / 2.0;
    y = bw + (settings.topbarHeight - (double)h/PANGO_SCALE) / 2.0;
    cairo_move_to(cairo, x, y);
    SetSourceColor(cairo, fg);
    pango_cairo_show_layout(cairo, layout);
    g_object_unref(layout);
}

void
DrawFrame(Client *c, cairo_t *cairo)
{
    int bg, bc, bw;
    /* select the frame colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bg = settings.urgentBackground;
        bc = settings.urgentBorder;
    }  else if (c->isActive) {
        bg = settings.activeBackground;
        bc = c->isTiled ? settings.activeTileBackground
                : settings.activeBorder;
    } else {
        bg = settings.inactiveBackground;
        bc = c->isTiled ? settings.inactiveTileBackground
                : settings.inactiveBorder;
    }

    /* what is the border width */
    bw = c->isBorderVisible ? settings.borderWidth : 0;

    /* background */
    SetSourceColor(cairo, bg);
    cairo_rectangle(cairo, bw, bw, c->fw - 2 * bw, c->fh - 2 * bw);  
    cairo_fill(cairo);

    /* border */
    if (c->isBorderVisible) {
        cairo_set_line_width(cairo, 2 * bw);
        SetSourceColor(cairo, bc);
        cairo_rectangle(cairo, 0, 0, c->fw, c->fh);  
        cairo_stroke(cairo);
    }
}

void
DrawButton(Client *c, cairo_t *cairo, int button)
{
    int bbg, bfg, bbc;
    int x, y, w, h;

    /* select the button colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bbg = settings.urgentBackground;
        bfg = settings.urgentForeground;
        bbc = settings.urgentForeground;
    }  else if (c->isActive) {
        if (button == c->hovered) {
            bbg = settings.buttonStyles[button].activeHoveredBackground;
            bfg = settings.buttonStyles[button].activeHoveredForeground;
            bbc = settings.buttonStyles[button].inactiveHoveredBorder;
        } else {
            bbg = settings.buttonStyles[button].activeBackground;
            bfg = settings.buttonStyles[button].activeForeground;
            bbc = settings.buttonStyles[button].activeBorder;
        }
    } else {
        if (button == c->hovered) {
            bbg = settings.buttonStyles[button].inactiveHoveredBackground;
            bfg = settings.buttonStyles[button].inactiveHoveredForeground;
            bbc = settings.buttonStyles[button].inactiveHoveredBorder;
        } else {
            bbg = settings.buttonStyles[button].inactiveBackground;
            bfg = settings.buttonStyles[button].inactiveForeground;
            bbc = settings.buttonStyles[button].inactiveBorder;
        }
    }
    
    /* draw buttons */
    GetButtonGeometry(c, button, &x, &y, &w, &h);

    if (settings.buttonShape == ButtonCirle) {
        /* button background */
        SetSourceColor(cairo, bbg);
        cairo_arc(cairo, x+w/2.0, y+h/2.0, w/2.0, 0, 2*M_PI);
        cairo_fill(cairo);
        SetSourceColor(cairo, bbc);

        /* button border */
        cairo_arc(cairo, x+w/2.0, y+h/2.0, w/2.0, 0, 2*M_PI);
        cairo_set_line_width (cairo, 0.5);
        cairo_stroke (cairo);
    } else {
        /* button background */
        SetSourceColor(cairo, bbg);
        cairo_rectangle(cairo, x, y, w, h);
        cairo_fill(cairo);
        SetSourceColor(cairo, bbc);

        /* button border */
        cairo_rectangle(cairo, x, y, w, h);
        cairo_set_line_width (cairo, 0.5);
        cairo_stroke (cairo);
    }

    /* button icon */
    SetSourceColor(cairo, bfg);
    if (strlen(settings.buttonStyles[button].icon)) {
        int tw, th;
        double dx, dy, dw, dh;
        PangoLayout *layout;
        PangoFontDescription *desc;

        layout = pango_cairo_create_layout(cairo);
        pango_layout_set_text(layout, settings.buttonStyles[button].icon, -1);
        desc = pango_font_description_from_string(settings.iconFontname);
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        pango_layout_get_size(layout, &tw, &th);
        dw = (double)tw/PANGO_SCALE;
        dh = (double)th/PANGO_SCALE;
        dx = (double)x + ((double)w - dw) / 2.0;
        dy = (double)y + ((double)h - dh) / 2.0;
        cairo_move_to(cairo, dx, dy);
        pango_cairo_show_layout(cairo, layout);
        g_object_unref(layout);
    } else {
        cairo_set_line_width (cairo, 1.5);
        if (button == ButtonClose) {
            cairo_save(cairo);
            cairo_translate(cairo, x+w/2.0, y+h/2.0);
            cairo_scale(cairo, IconScaleFactor, IconScaleFactor);
            cairo_move_to(cairo, -w/2.0, -h/2.0);
            cairo_line_to (cairo, w/2.0, h/2.0);
            cairo_move_to (cairo, w/2.0, -h/2.0);
            cairo_line_to (cairo, -w/2.0, h/2.0);
            cairo_restore(cairo);
            cairo_stroke (cairo);
        }
        if (button == ButtonMaximize) {
            cairo_save(cairo);
            cairo_translate(cairo, x+w/2.0, y+h/2.0);
            cairo_scale(cairo, IconScaleFactor, IconScaleFactor);
            cairo_rectangle(cairo, -w/2.0, -h/2.0, w, h);
            cairo_restore(cairo);
            cairo_stroke (cairo);
        }
        if (button == ButtonMinimize) {
            cairo_save(cairo);
            cairo_translate(cairo, x+w/2.0, y+h/2.0);
            cairo_scale(cairo, IconScaleFactor, IconScaleFactor);
            cairo_move_to (cairo, -w/2.0, 0);
            cairo_line_to (cairo, w/2.0, 0 );
            cairo_restore(cairo);
            cairo_stroke (cairo);
        }
    }
}
