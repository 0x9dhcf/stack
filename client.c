#include <X11/Xlib.h>
#include <string.h>
#include <X11/Xatom.h>

#include "client.h"
#include "hints.h"
#include "log.h"
#include "monitor.h"
#include "utils.h"
#include "x11.h"


#define DecorationWidth (2 * stConfig.borderWidth\
    + ButtonCount * (stConfig.buttonSize + stConfig.buttonGap) + 1)
#define DecorationHeight (2 * stConfig.borderWidth +\
        stConfig.topbarHeight + 1)

static void ApplyNormalHints(Client *c);
static void Configure(Client *c);
static void SaveGeometries(Client *c);
static void SynchronizeFrameGeometry(Client *c);
static void SynchronizeWindowGeometry(Client *c);

void
HideClient(Client *c)
{
    /* move all windows off screen without changing anything */
    XMoveWindow(stDisplay, c->frame, -c->fw, c->fy);
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals))
        for (int i = 0; i < HandleCount; ++i)
            XMoveWindow(stDisplay, c->handles[i], -c->fw, c->fy);
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
    SetNetWMStates(c->window, c->states);

    if (c->states & NetWMStateFullscreen) {
        c->decorated = True;
        c->states &= ~NetWMStateFullscreen;
        if (!c->tiled)
            MoveResizeClientFrame(c, c->sfx, c->sfy, c->sfw, c->sfh, False);
    } else if (c->states & NetWMStateHidden) {
        c->states &= ~NetWMStateHidden;
        if (!c->tiled)
            MoveResizeClientFrame(c, c->shx, c->shy, c->shw, c->shh, False);
    } else if (c->states & NetWMStateMaximized) {
        c->states &= ~NetWMStateMaximized;
        if (!c->tiled)
            MoveResizeClientFrame(c, c->smx, c->smy, c->smw, c->smh, False);
    } else if (c->tiled) {
        c->tiled = False;
        MoveResizeClientFrame(c, c->stx, c->sty, c->stw, c->sth, False);
    }

    SetNetWMStates(c->window, c->states);
}

void
RaiseClient(Client *c)
{
    c->states |= NetWMStateAbove;
    c->states &= ~NetWMStateBelow;
    XRaiseWindow(stDisplay, c->frame);
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals))
        for (int i = 0; i < HandleCount; ++i)
            XRaiseWindow(stDisplay, c->handles[i]);

    for (Transient *tc = c->transients; tc; tc = tc->next)
        RaiseClient(tc->client);

    SetNetWMStates(c->window, c->states);
}

void
LowerClient(Client *c)
{
    c->states |= NetWMStateBelow;
    c->states &= ~NetWMStateAbove;
    XLowerWindow(stDisplay, c->frame);
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals))
        for (int i = 0; i < HandleCount; ++i)
            XLowerWindow(stDisplay, c->handles[i]);

    for (Transient *tc = c->transients; tc; tc = tc->next)
        LowerClient(tc->client);

    SetNetWMStates(c->window, c->states);
}

void
SetClientActive(Client *c, Bool b)
{
    unsigned int modifiers[] = { 0, LockMask, stNumLockMask, stNumLockMask|LockMask };

    if (b && !c->active) {
        if (c->hints & HintsFocusable)
            XSetInputFocus(stDisplay, c->window, RevertToPointerRoot, CurrentTime);
        else if (c->protocols & NetWMProtocolTakeFocus)
            SendMessage(c->window, stAtoms[AtomWMTakeFocus]);

        c->states &= ~NetWMStateDemandsAttention;
        c->hints &= ~HintsUrgent;
        c->active = b;

        RefreshClient(c);

        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; ++i)
                XGrabButton(stDisplay, Button1, Modkey | modifiers[i], c->window,
                        False, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                        GrabModeAsync, GrabModeSync, None, None);

    }

    if (!b && c->active) {
        c->active = b;
        RefreshClient(c);
        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; i++)
                XUngrabButton(stDisplay, Button1, Modkey | modifiers[i], c->window);
    }
}

Client *
NextClient(Client *c)
{
    return c->next ? c->next : c->monitor->chead;
}

Client *
PreviousClient(Client *c)
{
    return c->prev ? c->prev : c->monitor->ctail;
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
        c->monitor->chead = c->next;

    /* change monitor is needed */
    c->monitor = after->monitor;

    if (c->next)
        c->next->prev = c->prev;
    else
        c->monitor->ctail = c->prev;

    if (after == c->monitor->ctail) {
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
        c->monitor->chead = c->next;

    /* change monitor is needed */
    c->monitor = before->monitor;

    if (c->next)
        c->next->prev = c->prev;
    else
        c->monitor->ctail = c->prev;

    if (before == c->monitor->chead) {
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
    if (c->monitor->chead) {
        c->next = c->monitor->chead;
        c->monitor->chead->prev = c;
    }

    if (! c->monitor->ctail)
        c->monitor->ctail = c;

    c->monitor->chead = c;
    c->prev = NULL;
}

void
PushClientBack(Client *c)
{
    if (c->monitor->ctail) {
        c->prev = c->monitor->ctail;
        c->monitor->ctail->next = c;
    }

    if (! c->monitor->chead)
        c->monitor->chead = c;

    c->monitor->ctail = c;
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
            for (Client *mc = m->chead; mc; mc = mc->next) {
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
        RestoreClient(c);
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
    XChangeProperty(stDisplay, c->window, stAtoms[AtomNetWMDesktop], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char*)&c->desktop, 1);
}

void
KillClient(Client *c)
{
    if (c->protocols & NetWMProtocolDeleteWindow) {
        SendMessage(c->window, stAtoms[AtomWMDeleteWindow]);
    } else {
        XGrabServer(stDisplay);
        XSetErrorHandler(DummyErrorHandler);
        XSetCloseDownMode(stDisplay, DestroyAll);
        XKillClient(stDisplay, c->window);
        XSync(stDisplay, False);
        XSetErrorHandler(EventLoopErrorHandler);
        XUngrabServer(stDisplay);
    }
}


void
RefreshClientButton(Client *c, int button, Bool hovered)
{
    int bg, fg, x, y;
    /* Select the button colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bg = stConfig.urgentBackground;
        fg = stConfig.urgentForeground;
    }  else if (c->active) {
        if (hovered) {
            bg = stConfig.buttonStyles[button].activeHoveredBackground;
            fg = stConfig.buttonStyles[button].activeHoveredForeground;
        } else {
            bg = stConfig.buttonStyles[button].activeBackground;
            fg = stConfig.buttonStyles[button].activeForeground;
        }
    } else {
        if (hovered) {
            bg = stConfig.buttonStyles[button].inactiveHoveredBackground;
            fg = stConfig.buttonStyles[button].inactiveHoveredForeground;
        } else {
            bg = stConfig.buttonStyles[button].inactiveBackground;
            fg = stConfig.buttonStyles[button].inactiveForeground;
        }
    }

    XSetWindowBackground(stDisplay, c->buttons[button], bg);
    XClearWindow(stDisplay, c->buttons[button]);
    GetTextPosition(stConfig.buttonStyles[button].icon, stIconFont,
            AlignCenter, AlignMiddle, stConfig.buttonSize,
            stConfig.buttonSize, &x, &y);
    WriteText(c->buttons[button], stConfig.buttonStyles[button].icon,
            stIconFont, fg, x, y);
}

void
RefreshClient(Client *c)
{
    int bg, fg, /*bbg, bfg,*/ x, y;

    /* Do not attempt to refresh non decorated or hidden clients */
    if (!c->decorated || (c->desktop != c->monitor->activeDesktop
                && !(c->states & NetWMStateSticky)))
        return;

    /* Select the frame colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bg = stConfig.urgentBackground;
        fg = stConfig.urgentForeground;
    }  else if (c->active) {
        bg = c->tiled ?  stConfig.activeTileBackground : stConfig.activeBackground;
        fg = stConfig.activeForeground;
    } else {
        bg = c->tiled ?  stConfig.inactiveTileBackground : stConfig.inactiveBackground;
        fg = stConfig.inactiveForeground;
    }

    XSetWindowBackground(stDisplay, c->frame, bg);
    XClearWindow(stDisplay, c->frame);
    XSetWindowBackground(stDisplay, c->topbar, bg);
    XClearWindow(stDisplay, c->topbar);
    if (c->name) {
        GetTextPosition(c->name, stLabelFont, AlignCenter, AlignMiddle,
                c->ww - 2 * stConfig.borderWidth, stConfig.topbarHeight, &x, &y);
        WriteText(c->topbar, c->name, stLabelFont, fg, x, y);
    }

    for (int i = 0; i < ButtonCount; ++i)
        RefreshClientButton(c, i, False);
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
    XMoveResizeWindow(stDisplay, c->frame, c->fx, c->fy, c->fw, c->fh);
    XMoveResizeWindow(stDisplay, c->window, wx, wy, c->ww, c->wh);

    /* place decoration windows */
    if (c->decorated && !c->tiled && ! (c->types & NetWMTypeNoTopbar)) {
        XMoveResizeWindow(stDisplay, c->topbar, stConfig.borderWidth,
                stConfig.borderWidth, c->fw - 2 * stConfig.borderWidth,
                stConfig.topbarHeight);
        for (int i = 0; i < ButtonCount; ++i) {
            XMoveResizeWindow(stDisplay, c->buttons[i],
                    c->fw - 2 * stConfig.borderWidth
                    - ((i+1) * (stConfig.buttonSize) + i * stConfig.buttonGap),
                    (stConfig.topbarHeight - stConfig.buttonSize) / 2,
                    stConfig.buttonSize, stConfig.buttonSize);
        }
    } else if (!(c->types & NetWMTypeFixed)) {
        /* move the topbar outside the frame */
        XMoveWindow(stDisplay, c->topbar, stConfig.borderWidth,
                -(stConfig.borderWidth + stConfig.topbarHeight));
    }

    /* suround frame by handles */
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals)) {
        hw = stConfig.handleWidth;
        XMoveResizeWindow(stDisplay, c->handles[HandleNorth],
                c->fx, c->fy - hw, c->fw, hw);
        XMoveResizeWindow(stDisplay, c->handles[HandleNorthWest],
                c->fx + c->fw, c->fy - hw, hw, hw);
        XMoveResizeWindow(stDisplay, c->handles[HandleWest],
                c->fx + c->fw, c->fy, hw, c->fh);
        XMoveResizeWindow(stDisplay, c->handles[HandleSouthWest],
                c->fx + c->fw, c->fy + c->fh, hw, hw);
        XMoveResizeWindow(stDisplay, c->handles[HandleSouth],
                c->fx, c->fy + c->fh, c->fw, hw);
        XMoveResizeWindow(stDisplay, c->handles[HandleSouthEast],
                c->fx - hw, c->fy + c->fh, hw, hw);
        XMoveResizeWindow(stDisplay, c->handles[HandleEast],
                c->fx - hw, c->fy, hw, c->fh);
        XMoveResizeWindow(stDisplay, c->handles[HandleNorthEast],
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

#define WXOffset(c) (c->decorated ? stConfig.borderWidth : 0)
#define WYOffset(c) (c->decorated ? stConfig.borderWidth + stConfig.topbarHeight : 0)
#define WWOffset(c) (c->decorated ? 2 * stConfig.borderWidth : 0)
#define WHOffset(c) (c->decorated ? 2 * stConfig.borderWidth + stConfig.topbarHeight: 0)

void
SynchronizeFrameGeometry(Client *c)
{
    if (c->tiled || (c->types & NetWMTypeNoTopbar)) {
        c->fx = c->wx - stConfig.borderWidth;
        c->fy = c->wy - stConfig.borderWidth;
        c->fw = c->ww + 2 * stConfig.borderWidth;
        c->fh = c->wh + 2 * stConfig.borderWidth;
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
        c->wx = c->fx + stConfig.borderWidth;
        c->wy = c->fy + stConfig.borderWidth;
        c->ww = c->fw - 2 * stConfig.borderWidth;
        c->wh = c->fh - 2 * stConfig.borderWidth;
    } else {
        c->wx = c->fx + WXOffset(c);
        c->wy = c->fy + WYOffset(c);
        c->ww = c->fw - WWOffset(c);
        c->wh = c->fh - WHOffset(c);
    }
}
