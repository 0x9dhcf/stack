#include <string.h>
#include <limits.h>

#include "atoms.h"
#include "client.h"
#include "config.h"
#include "cursors.h"
#include "font.h"
#include "hints.h"
#include "log.h"
#include "monitor.h"
#include "stack.h"
#include "x11.h"

//#define DLogClient(cptr) {\
//    DLog("window:\t%ld @ (%d, %d) [%d x %d]\n"\
//         "frame:\t%ld@ (%d, %d) [%d x %d]\n"\
//         "fixed: %d"\
//         "decorated: %d" ,\
//         cptr->window, cptr->x, cptr->y, cptr->w, cptr->h,\
//         cptr->frame, cptr->fx, cptr->fy, cptr->fw, cptr->fh\
//         c->fixed, c->decorated); }
//

//void
//CloseClient(Client *c)
//{
//    if (c->deletable)
//        SendClientMessage(c, stAtoms[AtomWMDeleteWindow]);
//    else
//        XKillClient(stDisplay, c->window);
//}


static void ApplyNormalHints(Client *c);
static void Configure(Client *c);
static void GrabButtons(Client *c);
static void SaveGeometries(Client *c);
static void SynchronizeFrameGeometry(Client *c);
static void SynchronizeWindowGeometry(Client *c);
static void WriteState(Client *c);

void
MoveClientWindow(Client *c, int x, int y)
{
    if (c->states & StateFixed 
            || c->states & StateFullscreen
            || (c->states & StateMaximized) == StateMaximized)
        return;

    c->wx = x;
    c->wy = y;
    SynchronizeFrameGeometry(c);
    Configure(c);
}

void
ResizeClientWindow(Client *c, int w, int h, Bool sh)
{
    if (c->states & StateFixed 
            || c->states & StateFullscreen
            || (c->states & StateMaximized) == StateMaximized)
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
    if (c->states & StateFixed 
            || c->states & StateFullscreen
            || (c->states & StateMaximized) == StateMaximized)
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
    if (c->states & StateFixed 
            || c->states & StateFullscreen
            || (c->states & StateMaximized) == StateMaximized)
        return;

    c->fx = x;
    c->fy = y;
    SynchronizeWindowGeometry(c);
    Configure(c);
}

void
ResizeClientFrame(Client *c, int w, int h, Bool sh)
{
    if (c->states & StateFixed 
            || c->states & StateFullscreen
            || (c->states & StateMaximized) == StateMaximized)
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
    if (c->states & StateFixed 
            || c->states & StateFullscreen
            || (c->states & StateMaximized) == StateMaximized)
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
MaximizeClient(Client *c, int flag, Bool user)
{
    if (c->states & StateFixed || (c->states & flag) == flag)
        return;

    /* if not maximized or fullscreen save the geometries */
    if (!(c->states & (StateFullscreen | StateMaximizedAny)))
        SaveGeometries(c);

    /* clear the state flag to allow move rezize */
    c->states &= ~StateMaximizedAny;

    //DLog("flag: %d", flag);
    //DLog("working area: (%d, %d) [%d x %d]", c->monitor->wx, c->monitor->wy,
    //        c->monitor->ww, c->monitor->wh);

    switch (flag) {
        case StateMaximized:
            MoveResizeClientFrame(c, c->monitor->wx, c->monitor->wy,
                    c->monitor->ww, c->monitor->wh, False);
        break;
        case StateMaximizedHorizontally:
            MoveResizeClientFrame(c, c->monitor->wx, c->fy,
                    c->monitor->ww, c->fh, False);
        break;
        case StateMaximizedVertically:
            MoveResizeClientFrame(c, c->fx, c->monitor->wy,
                    c->fw, c->monitor->wh, False);
        break;
        case StateMaximizedLeft:
            MoveResizeClientFrame(c, c->monitor->wx, c->monitor->wy,
                    c->monitor->ww / 2, c->monitor->wh, False);
        break;
        case StateMaximizedRight:
            MoveResizeClientFrame(c, c->monitor->wx + c->monitor->ww /2, 
                    c->monitor->wy, c->monitor->ww / 2, c->monitor->wh, False);
        break;
        case StateMaximizedTop:
            MoveResizeClientFrame(c, c->monitor->wx, c->monitor->wy,
                    c->monitor->ww, c->monitor->wh / 2, False);
        break;
        case StateMaximizedBottom:
            MoveResizeClientFrame(c, c->monitor->wx,
                    c->monitor->wy + c->monitor->wh / 2,
                    c->monitor->ww, c->monitor->wh / 2, False);
        break;
    }

    c->states |= flag;
    if (user)
        WriteState(c);
}

void
MinimizeClient(Client *c, Bool user)
{
    if (c->states & StateFixed || c->states & StateMinimized)
        return;

    /* clear the state flag to allow move rezize */
    c->states &= ~StateMaximizedAny;

    /* if not maximized of fullscreen save the geometries */
    if (!(c->states & (StateFullscreen | StateMaximizedAny)))
        SaveGeometries(c);

    MoveResizeClientFrame(c, c->monitor->x, c->monitor->y + c->monitor->h,
            c->fw, c->fw, False);

    c->states |= StateMinimized;
    if (user)
        WriteState(c);
}

void
FullscreenClient(Client *c, Bool user)
{
    int st;

    if (c->states & StateFixed || c->states & StateFullscreen)
        return;

    /* clear the state flag to allow move rezize */
    st = c->states;
    c->states &= ~StateMaximizedAny;
    c->states &= ~StateDecorated;
    c->states |= StateFullscreen;

    MoveResizeClientWindow(c, c->monitor->x, c->monitor->y, c->monitor->w,
            c->monitor->h, False);

    c->states |= st;
    if (user)
        WriteState(c);
}

/* XXX can't see when user won't be tru */
void
RestoreClient(Client *c, Bool user)
{
    if (c->states & StateFullscreen) {
        c->states |= StateDecorated;
        c->states &= ~StateFullscreen;

        if ((c->states & StateMaximizedAny) == StateMaximizedAny) {
            MaximizeClient(c, c->states, user);
        } else {
            c->states &= ~StateMaximizedAny;
            MoveResizeClientFrame(c, c->sfx, c->sfy, c->sfw, c->sfh, False);
        }

        if (user)
            WriteState(c);
    } else {
        c->states &= ~StateMaximizedAny;
        MoveResizeClientFrame(c, c->sfx, c->sfy, c->sfw, c->sfh, False);
        if (user)
            WriteState(c);
    }
}

void
RaiseClient(Client *c, Bool user)
{
    c->states |= StateAbove;
    c->states &= ~StateBelow;
    XRaiseWindow(stDisplay, c->frame);
    for (int i = 0; i < HandleCount; ++i)
        XRaiseWindow(stDisplay, c->handles[i]);

    if (user)
        WriteState(c);
}

void
LowerClient(Client *c, Bool user)
{
    c->states |= StateBelow;
    c->states &= ~StateAbove;
    XLowerWindow(stDisplay, c->frame);
    for (int i = 0; i < HandleCount; ++i)
        XLowerWindow(stDisplay, c->handles[i]);

    if (user)
        WriteState(c);
}

void
UpdateClientName(Client *c)
{
    GetWMName(c->window, &c->name);
    RefreshClient(c);
}

void
UpdateClientHints(Client *c)
{
    int h;
    GetWMHints(c->window, &h);

    if (h & HintsFocusable)
        c->states |= StateAcceptFocus;

    if (h & HintsUrgent) {
        c->states |= StateUrgent;
        RefreshClient(c);
    }
}

void
UpdateClientState(Client *c)
{
    int h;

    GetNetWMState(c->window, &h);
    if ((h & NetWMStateMaximized)  == NetWMStateMaximized) {
        MaximizeClient(c, StateMaximized, False);
    } else if (h & NetWMStateMaximizedHorz) {
        MaximizeClient(c, StateMaximizedHorizontally, False);
    } else if (h & NetWMStateMaximizedVert) {
        MaximizeClient(c, StateMaximizedVertically, False);
    } 

    if (h & NetWMStateHidden)
        MinimizeClient(c, False);

    if (h & NetWMStateFullscreen)
        FullscreenClient(c, False);

    if (h & NetWMStateAbove)
        RaiseClient(c, False);

    if (h & NetWMStateBelow)
        LowerClient(c, False);

    if (h & NetWMStateDemandsAttention) {
        c->states |= StateUrgent;
        RefreshClient(c);
    }

    if (h & NetWMStateSticky)
        c->states |= StateSticky;
}

void
SetClientActive(Client *c, Bool b)
{
    if (b) {
        if (c->states & StateAcceptFocus) {
            XSetInputFocus(stDisplay, c->window, RevertToPointerRoot, CurrentTime);
        //    SendClientMessage(c, stAtoms[AtomWMTakeFocus]);
        }
        c->states &= ~StateUrgent;
        c->states |= StateActive;
        RefreshClient(c);
        RaiseClient(c, True);
    } else {
        c->states &= ~StateActive;
        RefreshClient(c);
        XDeleteProperty(stDisplay, stRoot, stAtoms[AtomNetActiveWindow]);
    }
    WriteState(c);
    NotifyClient(c);
    GrabButtons(c);
}

/*
 * static implemetations
 */
void
ApplyNormalHints(Client *c)
{
    DLog("(%d, %d) [%d x %d]", c->wx, c->wy, c->ww, c->wh);
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
    DLog("(%d, %d) [%d x %d]", c->wx, c->wy, c->ww, c->wh);
}

void
Configure(Client *c)
{
    //DLog("frame (absolute)\t: (%d, %d) [%d x %d]", c->fx, c->fy, c->fw, c->fh);
    //DLog("window (absolute)\t: (%d, %d) [%d x %d]", c->wx, c->wy, c->ww, c->wh);
    int hw, wx, wy;

    /* compute the relative window position */
    wx = c->wx - c->fx;
    wy = c->wy - c->fy;

    /* place frame and window */
    XMoveResizeWindow(stDisplay, c->frame, c->fx, c->fy, c->fw, c->fh);
    XMoveResizeWindow(stDisplay, c->window, wx, wy, c->ww, c->wh);

    /* place decoration windows */
    if (c->states & StateDecorated) {
        XMoveResizeWindow(stDisplay, c->topbar, stConfig.borderWidth,
                stConfig.borderWidth, c->fw, stConfig.topbarHeight);
        for (int i = 0; i < ButtonCount; ++i) {
            XMoveResizeWindow(stDisplay, c->buttons[i],
                    c->fw - (i + 1) * (stConfig.buttonSize + stConfig.buttonGap),
                    (stConfig.topbarHeight - stConfig.buttonSize) / 2,
                    stConfig.buttonSize, stConfig.buttonSize);
        }
    } else { /* move the topbar outside the frame */
        XMoveWindow(stDisplay, c->topbar, stConfig.borderWidth,
                -(stConfig.borderWidth + stConfig.topbarHeight));
    }


    /* suround frame by handles */
    hw = stConfig.handleWidth;
    XMoveResizeWindow(stDisplay, c->handles[HandleNorth], c->fx, c->fy - hw, c->fw, hw);
    XMoveResizeWindow(stDisplay, c->handles[HandleNorthWest], c->fx + c->fw, c->fy - hw, hw, hw);
    XMoveResizeWindow(stDisplay, c->handles[HandleWest], c->fx + c->fw, c->fy, hw, c->fh);
    XMoveResizeWindow(stDisplay, c->handles[HandleSouthWest], c->fx + c->fw, c->fy + c->fh, hw, hw);
    XMoveResizeWindow(stDisplay, c->handles[HandleSouth], c->fx, c->fy + c->fh, c->fw, hw);
    XMoveResizeWindow(stDisplay, c->handles[HandleSouthEast], c->fx - hw, c->fy + c->fh, hw, hw);
    XMoveResizeWindow(stDisplay, c->handles[HandleEast], c->fx - hw, c->fy, hw, c->fh);
    XMoveResizeWindow(stDisplay, c->handles[HandleNorthEast], c->fx - hw, c->fy - hw, hw, hw);

    RefreshClient(c);
    NotifyClient(c);
}

void
NotifyClient(Client *c)
{
    XConfigureEvent e;
    e.event = c->window;
    e.window = c->window;
    e.type = ConfigureNotify;
    e.x = c->wx;
    e.y = c->wy;
    e.width = c->ww;
    e.height = c->wh;
    e.border_width = 0;
    e.above = None;
    e.override_redirect = 0;

    XSendEvent(stDisplay, c->window, False, SubstructureNotifyMask, (XEvent*)&e);
}
//
//void
//SendClientMessage(Client *c, Atom proto)
//{
//    XClientMessageEvent  cm;
//
//    (void)memset(&cm, 0, sizeof(cm));
//    cm.type = ClientMessage;
//    cm.window = c->window;
//    cm.message_type = stAtoms[AtomWMProtocols];
//    cm.format = 32;
//    cm.data.l[0] = proto;
//    cm.data.l[1] = CurrentTime;
//
//    XSendEvent(stDisplay, c->window, False, NoEventMask, (XEvent *)&cm);
//}

void
RefreshClient(Client *c)
{
    int bg, fg, bbg, bfg, x, y;

    if (!(c->states & StateDecorated))
        return;

    /* select the colors */
    if (c->states & StateUrgent) {
        bg = bbg = stConfig.urgentBackground;
        fg = bfg = stConfig.urgentForeground;
    } else if (c->states & StateActive) {
        bg = stConfig.activeBackground;
        fg = stConfig.activeForeground;
        bbg = stConfig.activeButtonBackground;
        bfg = stConfig.activeButtonForeground;
    } else {
        bg = stConfig.inactiveBackground;
        fg = stConfig.inactiveForeground;
        bbg = stConfig.inactiveButtonBackground;
        bfg = stConfig.inactiveButtonForeground;
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
    for (int i = 0; i < ButtonCount; ++i) {
        XSetWindowBackground(stDisplay, c->buttons[i], bbg);
        XClearWindow(stDisplay, c->buttons[i]);
        GetTextPosition(stConfig.buttonIcons[i], stIconFont,
                AlignCenter, AlignMiddle, stConfig.buttonSize,
                stConfig.buttonSize, &x, &y);
        WriteText(c->buttons[i], stConfig.buttonIcons[i], stIconFont, bfg, x, y);
    }
}

void
GrabButtons(Client *c)
{
    unsigned int modifiers[] = { 0, LockMask, stNumLockMask, stNumLockMask|LockMask };

    XUngrabButton(stDisplay, AnyButton, AnyModifier, c->window);
    if (!(c->states & StateActive)) {
        XGrabButton(stDisplay, AnyButton, AnyModifier, c->window, False,
                ButtonPressMask | ButtonReleaseMask, GrabModeSync,
                GrabModeSync, None, None);
    } else {
        for (int i = 0; i < 4; i++)
            XGrabButton(stDisplay, Button1, Modkey | modifiers[i], c->window, False,
                    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                    GrabModeAsync, GrabModeSync, None, None);
    }
}

void
SaveGeometries(Client *c)
{
    DLog();
    /* XXX saved window geometry is never used */
    c->swx = c->wx;
    c->swy = c->wy;
    c->sww = c->ww;
    c->swh = c->wh;

    c->sfx = c->fx;
    c->sfy = c->fy;
    c->sfw = c->fw;
    c->sfh = c->fh;
}

#define WXOffset(c) (c->states & StateDecorated ? stConfig.borderWidth : 0)
#define WYOffset(c) (c->states & StateDecorated ? stConfig.borderWidth + stConfig.topbarHeight : 0)
#define WWOffset(c) (c->states & StateDecorated ? 2 * stConfig.borderWidth : 0)
#define WHOffset(c) (c->states & StateDecorated ? 2 * stConfig.borderWidth + stConfig.topbarHeight: 0)

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

void
WriteState(Client *c) {
    int netWMFlag = NetWMStateNone;

    if (c->states & StateMaximizedVertically)
        netWMFlag |= NetWMStateMaximizedVert;
    if (c->states & StateMaximizedHorizontally)
        netWMFlag |= NetWMStateMaximizedHorz;
    if (c->states & StateMinimized)
        netWMFlag |= NetWMStateHidden;
    if (c->states & StateFullscreen)
        netWMFlag |= NetWMStateFullscreen;
    if (c->states & StateAbove)
        netWMFlag |= NetWMStateAbove;
    if (c->states & StateBelow)
        netWMFlag |= NetWMStateBelow;
    if (c->states & StateUrgent)
        netWMFlag |= NetWMStateDemandsAttention;

    SetNetWMState(c->window, netWMFlag);

    NotifyClient(c);
}
