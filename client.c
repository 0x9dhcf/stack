#include <X11/Xlib.h>
#include <string.h>
#include <X11/Xatom.h>

#include "client.h"
#include "event.h"
#include "hints.h"
#include "manager.h"
#include "monitor.h"
#include "stack.h"

#define DecorationWidth (2 * config.borderWidth\
    + ButtonCount * (config.buttonSize + config.buttonGap) + 1)
#define DecorationHeight (2 * config.borderWidth +\
        config.topbarHeight + 1)

/* we always use Center and Middle, but we are ready for
 * window name right aligned */
typedef enum {
    AlignLeft,
    AlignCenter,
    AlignRight
} HAlign;

typedef enum {
    AlignTop,
    AlignMiddle,
    AlignBottom
} VAlign;


static void ApplyNormalHints(Client *c);
static void Configure(Client *c);
static void SaveGeometries(Client *c);
static void SynchronizeFrameGeometry(Client *c);
static void SynchronizeWindowGeometry(Client *c);
static void WriteText(Drawable d, const char*s, FontType ft, int color,
        HAlign ha, VAlign va, int w, int h);

void
HideClient(Client *c)
{
    /* move all windows off screen without changing anything */
    XMoveWindow(display, c->frame, -c->fw, c->fy);
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals))
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
    int mw = (c->decorated) ? DecorationWidth : 1;
    int mh = (c->decorated) ? DecorationHeight : 1;
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
    int mw = (c->decorated) ? DecorationWidth : 1;
    int mh = (c->decorated) ? DecorationHeight : 1;
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
    c->tiled = True;
    MoveResizeClientFrame(c, x, y, w, h, False);
}

void
UntileClient(Client *c)
{
    c->tiled = False;
    MoveResizeClientFrame(c, c->stx, c->sty, c->stw, c->sth, False);
}

void
MaximizeClientHorizontally(Client *c)
{
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !c->tiled) {
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
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals) && !c->tiled) {
        SaveGeometries(c);

        c->decorated = False;
        c->states |= NetWMStateFullscreen;

        MoveResizeClientWindow(c, c->monitor->x, c->monitor->y, c->monitor->w,
                c->monitor->h, False);

        SetNetWMStates(c->window, c->states);
        RaiseClient(c);
    }
}

void
RestoreClient(Client *c)
{
    /* do not restore any tiled window */
    if (c->tiled)
        return;

    if (c->states & NetWMStateFullscreen) {
        c->decorated = True;
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

    if (b && !c->active) {
        if (c->hints & HintsFocusable)
            XSetInputFocus(display, c->window, RevertToPointerRoot, CurrentTime);
        else if (c->protocols & NetWMProtocolTakeFocus)
            SendMessage(c->window, atoms[AtomWMTakeFocus]);

        c->states &= ~NetWMStateDemandsAttention;
        c->hints &= ~HintsUrgent;
        c->active = b;

        RefreshClient(c);

        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; ++i)
                XGrabButton(display, Button1, Modkey | modifiers[i], c->window,
                        False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                        GrabModeAsync, GrabModeSync, None, None);

    }

    if (!b && c->active) {
        c->active = b;
        RefreshClient(c);
        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; i++)
                XUngrabButton(display, Button1, Modkey | modifiers[i], c->window);
    }
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
RefreshClientButton(Client *c, int button, Bool hovered)
{
    int bg, fg;
    /* Select the button colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bg = config.urgentBackground;
        fg = config.urgentForeground;
    }  else if (c->active) {
        if (hovered) {
            bg = config.buttonStyles[button].activeHoveredBackground;
            fg = config.buttonStyles[button].activeHoveredForeground;
        } else {
            bg = config.buttonStyles[button].activeBackground;
            fg = config.buttonStyles[button].activeForeground;
        }
    } else {
        if (hovered) {
            bg = config.buttonStyles[button].inactiveHoveredBackground;
            fg = config.buttonStyles[button].inactiveHoveredForeground;
        } else {
            bg = config.buttonStyles[button].inactiveBackground;
            fg = config.buttonStyles[button].inactiveForeground;
        }
    }

    XSetWindowBackground(display, c->buttons[button], bg);
    XClearWindow(display, c->buttons[button]);
    WriteText(c->buttons[button], config.buttonStyles[button].icon, FontIcon,
            fg, AlignCenter, AlignMiddle, config.buttonSize,
            config.buttonSize);
}

void
RefreshClient(Client *c)
{
    int bg, fg;

    /* Do not attempt to refresh non decorated or hidden clients */
    if (!c->decorated || (c->desktop != c->monitor->activeDesktop
                && !(c->states & NetWMStateSticky)))
        return;

    /* Select the frame colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bg = config.urgentBackground;
        fg = config.urgentForeground;
    }  else if (c->active) {
        bg = c->tiled ?  config.activeTileBackground : config.activeBackground;
        fg = config.activeForeground;
    } else {
        bg = c->tiled ?  config.inactiveTileBackground : config.inactiveBackground;
        fg = config.inactiveForeground;
    }

    XSetWindowBackground(display, c->frame, bg);
    XClearWindow(display, c->frame);
    XSetWindowBackground(display, c->topbar, bg);
    XClearWindow(display, c->topbar);
    if (c->name)
        WriteText(c->topbar, c->name, FontLabel, fg, AlignCenter, AlignMiddle,
                c->ww - 2 * config.borderWidth, config.topbarHeight);

    for (int i = 0; i < ButtonCount; ++i)
        RefreshClientButton(c, i, False);
}

void
StackClientAfter(Client *c, Client *after)
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
StackClientBefore(Client *c, Client *before)
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
        RestackMonitor(c->monitor);
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
        RestackMonitor(m);

    /* finally let the pager know where we are */
    XChangeProperty(display, c->window, atoms[AtomNetWMDesktop], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char*)&c->desktop, 1);
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
    int hw, wx, wy;

    /* compute the relative window position */
    wx = c->wx - c->fx;
    wy = c->wy - c->fy;

    /* place frame and window */
    XMoveResizeWindow(display, c->frame, c->fx, c->fy, c->fw, c->fh);
    XMoveResizeWindow(display, c->window, wx, wy, c->ww, c->wh);

    /* place decoration windows */
    if (c->decorated && !c->tiled && ! (c->types & NetWMTypeNoTopbar)) {
        XMoveResizeWindow(display, c->topbar, config.borderWidth,
                config.borderWidth, c->fw - 2 * config.borderWidth,
                config.topbarHeight);
        for (int i = 0; i < ButtonCount; ++i) {
            XMoveResizeWindow(display, c->buttons[i],
                    c->fw - 2 * config.borderWidth
                    - ((i+1) * (config.buttonSize) + i * config.buttonGap),
                    (config.topbarHeight - config.buttonSize) / 2,
                    config.buttonSize, config.buttonSize);
        }
    } else if (!(c->types & NetWMTypeFixed)) {
        /* move the topbar outside the frame */
        XMoveWindow(display, c->topbar, config.borderWidth,
                -(config.borderWidth + config.topbarHeight));
    }

    /* suround frame by handles */
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals)) {
        hw = config.handleWidth;
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

    /* synchronize transients */
    Desktop *d = &c->monitor->desktops[c->desktop];
    for (Transient *t = c->transients; t; t = t->next) {
        int x, y, w, h;
        w = Min(d->ww, t->client->fw);
        h = Min(d->wh, t->client->fh);
        x = Min(Max(d->wx, c->fx + (c->fw - t->client->fw) / 2), d->wx + d->ww - w);
        y = Min(Max(d->wy, c->fy + (c->fh - t->client->fh) / 2), d->wy + d->wh - h);
        MoveResizeClientFrame(t->client, x, y, w, h, False);
    }
}

void
SaveGeometries(Client *c)
{
    if (!c->tiled) {
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

#define WXOffset(c) (c->decorated ? config.borderWidth : 0)
#define WYOffset(c) (c->decorated ? config.borderWidth + config.topbarHeight : 0)
#define WWOffset(c) (c->decorated ? 2 * config.borderWidth : 0)
#define WHOffset(c) (c->decorated ? 2 * config.borderWidth + config.topbarHeight: 0)

void
SynchronizeFrameGeometry(Client *c)
{
    if (c->tiled || (c->types & NetWMTypeNoTopbar)) {
        c->fx = c->wx - config.borderWidth;
        c->fy = c->wy - config.borderWidth;
        c->fw = c->ww + 2 * config.borderWidth;
        c->fh = c->wh + 2 * config.borderWidth;
    } else {
        c->fx = c->wx - WXOffset(c);
        c->fy = c->wy - WYOffset(c);
        c->fw = c->ww + WWOffset(c);
        c->fh = c->wh + WHOffset(c);
    }
}

void
SynchronizeWindowGeometry(Client *c)
{
    if (c->tiled || (c->types & NetWMTypeNoTopbar)) {
        c->wx = c->fx + config.borderWidth;
        c->wy = c->fy + config.borderWidth;
        c->ww = c->fw - 2 * config.borderWidth;
        c->wh = c->fh - 2 * config.borderWidth;
    } else {
        c->wx = c->fx + WXOffset(c);
        c->wy = c->fy + WYOffset(c);
        c->ww = c->fw - WWOffset(c);
        c->wh = c->fh - WHOffset(c);
    }
}

void
WriteText(Drawable d, const char*s, FontType ft, int color,
        HAlign ha, VAlign va, int w, int h)
{
    int x, y;
    XGlyphInfo info;
    XftColor xftc;
    XftDraw *draw;
    Visual *v;
    Colormap cm;
    XftFont *f;

    f = fonts[ft];
    XftTextExtentsUtf8(display, f, (FcChar8*)s, strlen(s), &info);
    x = y = 0;
    switch ((int)ha) {
        case AlignCenter:
            x = (w - (info.xOff >= info.width ? info.xOff : info.width)) / 2;
            break;
        case AlignRight:
            x = w - (info.xOff >= info.width ? info.xOff : info.width);
            break;
    }
    switch ((int)va) {
        case AlignTop:
            y = h + f->ascent + f->descent - f->descent + info.yOff;
            break;
        case AlignMiddle:
            y = (h + f->ascent + f->descent) / 2 - f->descent + info.yOff;
            break;
    }

    v = DefaultVisual(display, 0);
    cm = DefaultColormap(display, 0);
    draw = XftDrawCreate(display, d, v, cm);

    char name[] = "#ffffff";
    snprintf(name, sizeof(name), "#%06X", color);
    XftColorAllocName(display, DefaultVisual(display,
                DefaultScreen(display)),
                DefaultColormap(display, DefaultScreen(display)),
                name, &xftc);
    XftDrawStringUtf8(draw, &xftc, f, x, y, (XftChar8 *)s, strlen(s));
    XftDrawDestroy(draw);
    XftColorFree(display,v, cm, &xftc);
}
