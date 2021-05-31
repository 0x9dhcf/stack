#include <string.h>

#include "stack.h"

#define WMin ( (c->decorated) ?  2 * stConfig.borderWidth +\
        ButtonCount * (stConfig.buttonSize + stConfig.buttonGap) + 1 : 1 )
#define HMin ( (c->decorated) ?  2 * stConfig.borderWidth +\
        stConfig.topbarHeight + 1 : 1 )

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
    if (w < WMin || h < HMin)
        return;

    c->fw = w;
    c->fh = h;

    if (sh)
        ApplyNormalHints(c);

    SynchronizeWindowGeometry(c);
    Configure(c);
}

void
MoveResizeClientFrame(Client *c, int x, int y, int w, int h, Bool sh)
{
    if (w < WMin || h < HMin)
        return;

    c->fx = x;
    c->fy = y;
    c->fw = w;
    c->fh = h;

    if (sh)
        ApplyNormalHints(c);

    SynchronizeWindowGeometry(c);
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
    }
}

void
RestoreClient(Client *c)
{
    if (c->states & NetWMStateFullscreen) {
        c->decorated = True;
        c->states &= ~NetWMStateFullscreen;
        if (c->tiled)
            Restack(c->monitor);
        else 
            MoveResizeClientFrame(c, c->sfx, c->sfy, c->sfw, c->sfh, False);
    } else if (c->states & (NetWMStateMaximized | NetWMStateHidden)) {
        c->states &= ~NetWMStateMaximized;
        c->states &= ~NetWMStateHidden;
        if (c->tiled)
            Restack(c->monitor);
        else
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

        if (c->states & NetWMStateHidden)
            RestoreClient(c);

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
        XDeleteProperty(stDisplay, stRoot, stAtoms[AtomNetActiveWindow]);
        if (!(c->types & NetWMTypeFixed))
            for (int i = 0; i < 4; i++)
                XUngrabButton(stDisplay, Button1, Modkey | modifiers[i], c->window);
    }
}

void
RefreshClientButton(Client *c, int button, Bool hovered)
{
    int bg, fg, x, y;
    /* select the button colors */
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

    /* do not attempt to refresh non decorated or hidden clients */
    if (!c->decorated || (c->desktop != c->monitor->activeDesktop &&
                !(c->states & NetWMStateSticky)))
        return;

    /* select the frame colors */
    if (c->states & NetWMStateDemandsAttention || c->hints & HintsUrgent) {
        bg = stConfig.urgentBackground;
        fg = stConfig.urgentForeground;
    }  else if (c->active) {
        bg = stConfig.activeBackground;
        fg = stConfig.activeForeground;
    } else {
        bg = stConfig.inactiveBackground;
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
        c->wh -= c->wh % c->normals.incw;
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
    if (c->decorated) {
        XMoveResizeWindow(stDisplay, c->topbar, stConfig.borderWidth,
                stConfig.borderWidth, c->fw - 2 * stConfig.borderWidth,
                stConfig.topbarHeight);
        for (int i = 0; i < ButtonCount; ++i) {
            XMoveResizeWindow(stDisplay, c->buttons[i],
                    c->fw - ((i+1) * (stConfig.buttonSize) + i * stConfig.buttonGap),
                    (stConfig.topbarHeight - stConfig.buttonSize) / 2,
                    stConfig.buttonSize, stConfig.buttonSize);
        }
    } else if (!(c->types & NetWMTypeFixed)){ /* move the topbar outside the frame */
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
    if (!(c->states & (NetWMStateFullscreen | NetWMStateMaximized | NetWMStateHidden))) {
        c->smx = c->fx;
        c->smy = c->fy;
        c->smw = c->fw;
        c->smh = c->fh;
    }
}

#define WXOffset(c) (c->decorated ? stConfig.borderWidth : 0)
#define WYOffset(c) (c->decorated ? stConfig.borderWidth + stConfig.topbarHeight : 0)
#define WWOffset(c) (c->decorated ? 2 * stConfig.borderWidth : 0)
#define WHOffset(c) (c->decorated ? 2 * stConfig.borderWidth + stConfig.topbarHeight: 0)

void
SynchronizeFrameGeometry(Client *c)
{
    c->fx = c->wx - WXOffset(c);
    c->fy = c->wy - WYOffset(c);
    c->fw = c->ww + WWOffset(c);
    c->fh = c->wh + WHOffset(c);
}

void
SynchronizeWindowGeometry(Client *c)
{
    c->wx = c->fx + WXOffset(c);
    c->wy = c->fy + WYOffset(c);
    c->ww = c->fw - WWOffset(c);
    c->wh = c->fh - WHOffset(c);
}

