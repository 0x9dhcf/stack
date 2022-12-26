#include <X11/X.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "client.h"
#include "event.h"
#include "hints.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "settings.h"
#include "x11.h"


#define CleanMask(mask)\
    ((mask) & ~(numLockMask|LockMask) &\
         ( ShiftMask\
         | ControlMask\
         | Mod1Mask\
         | Mod2Mask\
         | Mod3Mask\
         | Mod4Mask\
         | Mod5Mask))

static void OnConfigureRequest(XConfigureRequestEvent *e);
static void OnMapRequest(XMapRequestEvent *e);
static void OnUnmapNotify(XUnmapEvent *e);
static void OnDestroyNotify(XDestroyWindowEvent *e);
static void OnExpose(XExposeEvent *e);
static void OnEnter(XCrossingEvent *e);
static void OnLeave(XCrossingEvent *e);
static void OnPropertyNotify(XPropertyEvent *e);
static void OnButtonPress(XButtonEvent *e);
static void OnButtonRelease(XButtonEvent *e);
static void OnMotionNotify(XMotionEvent *e);
static void OnMessage(XClientMessageEvent *e);
static void OnKeyPress(XKeyPressedEvent *e);
static void OnKeyRelease(XKeyReleasedEvent *e);
static void Snap(int x, int y, int w, int h, int *xp,
        int *yp, int *wp, int *hp, int snap);

static XErrorHandler defaultErrorHandler = NULL;
static char *terminal[] = {"st", NULL};
static int lastSeenPointerX = -1;
static int lastSeenPointerY = -1;
static Time lastSeenPointerTime = 0;
static Time lastClickPointerTime = 0;
static int motionStartX = 0;
static int motionStartY = 0;
static int motionStartW = 0;
static int motionStartH = 0;
static int moveMessageType = 0;
static Bool switching = 0;
static Bool running = 0;

void
StartEventLoop()
{
    int xConnection;
    fd_set fdSet;

    defaultErrorHandler = XSetErrorHandler(EnableErrorHandler);

    XSync(display, False);
    xConnection = XConnectionNumber(display);
    running = True;
    while (running) {
        struct timeval timeout = { 0, 500000 };
        FD_ZERO(&fdSet);
        FD_SET(xConnection, &fdSet);

        if (select(xConnection + 1, &fdSet, NULL, NULL, &timeout) > 0) {
            while (XPending(display)) {
                XEvent e;
                XNextEvent(display, &e);

                switch(e.type) {
                    case MapRequest:
                        OnMapRequest(&e.xmaprequest);
                    break;
                    case UnmapNotify:
                        OnUnmapNotify(&e.xunmap);
                    break;
                    case DestroyNotify:
                        OnDestroyNotify(&e.xdestroywindow);
                    break;
                    case Expose:
                        OnExpose(&e.xexpose);
                    break;
                    case ConfigureRequest:
                        OnConfigureRequest(&e.xconfigurerequest);
                    break;
                    case PropertyNotify:
                        OnPropertyNotify(&e.xproperty);
                    break;
                    case ButtonPress:
                        OnButtonPress(&e.xbutton);
                    break;
                    case ButtonRelease:
                        OnButtonRelease(&e.xbutton);
                    break;
                    case MotionNotify:
                        OnMotionNotify(&e.xmotion);
                    break;
                    case EnterNotify:
                        OnEnter(&e.xcrossing);
                    break;
                    case LeaveNotify:
                        OnLeave(&e.xcrossing);
                    break;
                    case ClientMessage:
                        OnMessage(&e.xclient);
                    break;
                    case KeyPress:
                        OnKeyPress(&e.xkey);
                    break;
                    case KeyRelease:
                        OnKeyRelease(&e.xkey);
                    break;
                }

                XFlush(display);
            }
        }
    }
}

void
StopEventLoop()
{
    running = False;
}

int
EnableErrorHandler(Display *d, XErrorEvent *e)
{
    char message[256];
    /* ignore some error */
    if (e->error_code == BadWindow
            || (e->request_code == X_SetInputFocus && e->error_code == BadMatch)
            || (e->request_code == X_PolyText8 && e->error_code == BadDrawable)
            || (e->request_code == X_PolyFillRectangle && e->error_code == BadDrawable)
            || (e->request_code == X_PolySegment && e->error_code == BadDrawable)
            || (e->request_code == X_ConfigureWindow && e->error_code == BadMatch)
            || (e->request_code == X_GrabButton && e->error_code == BadAccess)
            || (e->request_code == X_GrabKey && e->error_code == BadAccess)
            || (e->request_code == X_CopyArea && e->error_code == BadDrawable)) {
        DLog("ignored error: request code=%d, error code=%d",
            e->request_code, e->error_code);
        XGetErrorText(display, e->error_code, message, 255);
        ILog("%ld: %s (ignored)", e->resourceid, message);
        return 0;
    }
    ELog("error: request code=%d, error code=%d",
            e->request_code, e->error_code);

    return defaultErrorHandler(d, e); /* may call exit */
}

int
DisableErrorHandler(Display *d, XErrorEvent *e)
{
    (void)d;
    (void)e;
    return 0;
}

int
WMDetectedErrorHandler(Display *d, XErrorEvent *e)
{
    (void)d;
    (void)e;
    FLog("A windows manager is already running!");
    return 1; /* never reached */
}

void
OnConfigureRequest(XConfigureRequestEvent *e)
{
    Client *c = LookupClient(e->window);

    if (!c) {
        XWindowChanges xwc;
        xwc.x = e->x;
        xwc.y = e->y;
        xwc.width = e->width;
        xwc.height = e->height;
        xwc.border_width = e->border_width;
        xwc.sibling = e->above;
        xwc.stack_mode = e->detail;
        XConfigureWindow(display, e->window, e->value_mask, &xwc);
    } else {
        if (e->value_mask & CWBorderWidth)
            c->sbw = e->border_width;

        if (c->isTiled || c->states & NetWMStateMaximized) {
            XConfigureEvent ce;

            ce.type = ConfigureNotify;
            ce.display = display;
            ce.event = c->window;
            ce.window = c->window;
            ce.x = c->wx;
            ce.y = c->wy;
            ce.width = c->ww;
            ce.height = c->wh;
            ce.border_width = c->sbw;
            ce.above = None;
            ce.override_redirect = False;
            XSendEvent(display, c->window, False,
                    StructureNotifyMask, (XEvent *)&ce);
        } else if (e->value_mask & (CWX|CWY|CWWidth|CWHeight)) {
            int x, y, w, h;

            x = c->wx;
            y = c->wy;
            w = c->ww;
            h = c->wh;

            if (e->value_mask & CWX)
                x = e->x;
            if (e->value_mask & CWY)
                y = e->y;
            if (e->value_mask & CWWidth)
                w = e->width;
            if (e->value_mask & CWHeight)
                h = e->height;

            MoveResizeClientWindow(c, x, y, w, h, True);
        }

        /* as the window is reparented, XRaiseWindow and XLowerWindow
         * will generate a ConfigureRequest. */
        if (e->value_mask & CWStackMode) {
            if (e->detail == Above || e->detail == TopIf)
                RaiseClient(c);
            if (e->detail == Below || e->detail == BottomIf)
                LowerClient(c);
        }
    }
    XSync(display, False);
}

void
OnMapRequest(XMapRequestEvent *e)
{
    XWindowAttributes wa;

    if (!XGetWindowAttributes(display, e->window, &wa))
        return;

    if (wa.override_redirect) {
        XMapWindow(display, e->window);
        return;
    }

    if (!LookupClient(e->window))
        ManageWindow(e->window, False);
}

void
OnUnmapNotify(XUnmapEvent *e)
{
    /* ignore UnmapNotify from reparenting  */
    if (e->event != root && e->event != None) {
        if (e->send_event) {
            long state[] = {WithdrawnState, None};
            XChangeProperty(display, e->window, atoms[AtomWMState],
                    atoms[AtomWMState], 32, PropModeReplace,
                    (unsigned char *)state, 2);
        } else {
            UnmanageWindow(e->window, False);
        }
    }
}

void
OnDestroyNotify(XDestroyWindowEvent *e)
{
    UnmanageWindow(e->window, True);
}

void
OnExpose(XExposeEvent *e)
{
    Client *c = LookupClient(e->window);
    if (c)
        RefreshClient(c);
}

void
OnPropertyNotify(XPropertyEvent *e)
{
    Client *c = LookupClient(e->window);
    if (!c)
        return;

    if (!c || e->window != c->window)
        return;

    if (e->atom == XA_WM_NAME || e->atom == atoms[AtomNetWMName]) {
        GetWMName(c->window, &c->name);
        RefreshClient(c);
    }

    if (e->atom == XA_WM_HINTS) {
        GetWMHints(c->window, &c->hints);
        if (c->hints & HintsUrgent) {
            RefreshClient(c);
        }
    }

    /* A Client wishing to change the state of a window MUST send a
     * _NET_WM_STATE client message to the root window
    if (e->atom == atoms[AtomNetWMState])
        UpdateClientState(c);
    */
}

void
OnButtonPress(XButtonEvent *e)
{
    Client *c = NULL;

    if (e->window == root) {
        SetActiveMonitor(MonitorContaining(e->x_root, e->y_root));
        return;
    }

    c = LookupClient(e->window);
    if (!c)
        return;

    lastSeenPointerX = e->x_root;
    lastSeenPointerY = e->y_root;
    motionStartX = c->fx;
    motionStartY = c->fy;
    motionStartW = c->fw;
    motionStartH = c->fh;

    if (!c->isTiled) {
        if (e->window == c->topbar
                || (e->window == c->window && e->state == Mod)) {
            int delay = e->time - lastClickPointerTime;
            if (delay > 150 && delay < 350) {
                if ((c->states & NetWMStateMaximized)) {
                    RestoreClient(c);
                } else {
                    MaximizeClient(c);
                }
            } else {
                XDefineCursor(display, e->window, cursors[CursorMove]);
            }
        }

        if (e->window == c->buttons[ButtonMaximize]) {
            if ((c->states & NetWMStateMaximized)) {
                RestoreClient(c);
            } else {
                MaximizeClient(c);
            }
        }

        if (e->window == c->buttons[ButtonMinimize]) {
            MinimizeClient(c);
            SetActiveClient(NULL);
        }
    }

    if (e->window == c->buttons[ButtonClose])
        KillClient(c);

    if (e->window != c->buttons[ButtonClose] && e->window != c->buttons[ButtonMinimize])
        SetActiveClient(c);

    if (e->window == c->window)
        XAllowEvents(display, ReplayPointer, CurrentTime);

    lastClickPointerTime = e->time;
}

void
OnButtonRelease(XButtonEvent *e)
{
    Client *c = LookupClient(e->window);
    if (!c)
        return;

    if (moveMessageType <= HandleCount) {
        moveMessageType = HandleCount + 1;
        XUngrabPointer(display, CurrentTime);
    }

    if (!c->isTiled) {
        for (int i = 0; i < HandleCount; ++i) {
            if (e->window == c->handles[i]) {
                /* apply the size hints */
                MoveResizeClientFrame(c, c->fx, c->fy, c->fw, c->fh, True);
                break;
            }
        }
    }

    if (e->window == c->topbar || e->window == c->window) {
        XDefineCursor(display, e->window, cursors[CursorNormal]);
        /* TODO: check monitor consistency */
    }

    if (e->window == c->window)
        XAllowEvents(display, ReplayPointer, CurrentTime);
}

void
OnMotionNotify(XMotionEvent *e)
{
    Client *c = LookupClient(e->window);

    /* prevent moving type fixed, maximized or fulscreen window
     * avoid to move to often as well */
    if (!c || c->types & NetWMTypeFixed
            || c->states & (NetWMStateMaximized | NetWMStateFullscreen)
            || (e->time - lastSeenPointerTime) <= 20)
        return;

    /* update client geometry */
    int vx = e->x_root - lastSeenPointerX;
    int vy = e->y_root - lastSeenPointerY;

    lastSeenPointerTime = e->time;

    if (c->isTiled) {
        if (e->window == c->handles[HandleWest]
                || e->window == c->handles[HandleEast]) {
            c->monitor->desktops[c->desktop].split =
                (e->x_root - c->monitor->x) / (float)c->monitor->w;
            RefreshMonitor(c->monitor);
        }
    } else {
        int x = c->fx;
        int y = c->fy;
        int w = c->fw;
        int h = c->fh;
        /* we do not apply normal hints during motion but when button is released
         * to make the resizing visually smoother. Some client apply normals by
         * themselves anway (e.g gnome-terminal) */
        if (e->window == c->topbar || e->window == c->window
                || moveMessageType == HandleCount) {

            x = motionStartX + vx;
            y = motionStartY + vy;

            /* border snapping */
            Desktop *d = &c->monitor->desktops[c->desktop];
            Snap(d->wx, d->wy, d->ww, d->wh, &x, &y, &w, &h, settings.snapping);
            for (Client *it = c->monitor->head; it; it = it->snext)
                if (it != c && it->desktop == c->desktop && it->isVisible)
                    Snap(it->fx, it->fy, it->fw, it->fh,
                            &x, &y, &w, &h, settings.snapping);
        } else if (e->window == c->handles[HandleNorth]
                || moveMessageType == HandleNorth) {
            x = motionStartX;
            y = motionStartY + vy;
            w = motionStartW;
            h = motionStartH - vy;
        } else if (e->window == c->handles[HandleWest]
                || moveMessageType == HandleWest) {
            w = motionStartW + vx,
            h = motionStartH;
        } else if (e->window == c->handles[HandleSouth]
                || moveMessageType == HandleSouth) {
            w = motionStartW,
            h = motionStartH + vy;
        } else if (e->window == c->handles[HandleEast]
                || moveMessageType == HandleEast) {
            x = motionStartX + vx;
            y = motionStartY;
            w = motionStartW - vx;
            h = motionStartH;
        } else if (e->window == c->handles[HandleNorthEast]
                || moveMessageType == HandleNorthEast) {
            x = motionStartX + vx;
            y = motionStartY + vy;
            w = motionStartW - vx;
            h = motionStartH - vy;
        } else if (e->window == c->handles[HandleNorthWest]
                || moveMessageType == HandleNorthWest) {
            x = motionStartX;
            y = motionStartY + vy;
            w = motionStartW + vx;
            h = motionStartH - vy;
        } else if (e->window == c->handles[HandleSouthWest]
                || moveMessageType == HandleSouthWest) {
            w = motionStartW + vx,
            h = motionStartH + vy;
        } else if (e->window == c->handles[HandleSouthEast]
                || moveMessageType == HandleSouthEast) {
            x = motionStartX + vx;
            y = motionStartY;
            w = motionStartW - vx;
            h = motionStartH + vy;
        } else {
            return;
        }
        MoveResizeClientFrame(c, x, y, w, h, False);
    }
}

void
OnMessage(XClientMessageEvent *e)
{
    if (e->window == root) {
        if (e->message_type == atoms[AtomNetCurrentDesktop])
            ShowMonitorDesktop(activeMonitor, e->data.l[0]);

        /*
         * specs say message MUST be send to root but it seems
         * most pagers send to the client
         */
        if (e->message_type == atoms[AtomNetActiveWindow]) {
            Client *toActivate = LookupClient(e->window);
            if (toActivate)
                SetActiveClient(toActivate);
        }
        return;
    }

    Client *c = LookupClient(e->window);
    if (!c)
        return;

    if (e->message_type == atoms[AtomNetWMState]) {
        if (e->data.l[1] == (long)atoms[AtomNetWMStateFullscreen]
                || e->data.l[2] == (long)atoms[AtomNetWMStateFullscreen]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                RestoreClient(c);
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                FullscreenClient(c);
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & NetWMStateFullscreen)
                    RestoreClient(c);
                else
                    FullscreenClient(c);
            }
        }

        if (e->data.l[1] == (long)atoms[AtomNetWMStateDemandsAttention]
                || e->data.l[2] == (long)atoms[AtomNetWMStateDemandsAttention]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                c->states &= ~NetWMStateDemandsAttention;
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                c->states |= NetWMStateDemandsAttention;
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & NetWMStateDemandsAttention)
                    c->states &= ~NetWMStateDemandsAttention;
                else
                    c->states |= NetWMStateDemandsAttention;
            }
            RefreshClient(c);
        }

        if (e->data.l[1] == (long)atoms[AtomNetWMStateMaximizedHorz]
                || e->data.l[2] == (long)atoms[AtomNetWMStateMaximizedHorz]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                RestoreClient(c);
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                MaximizeClientHorizontally(c);
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & NetWMStateMaximizedHorz)
                    RestoreClient(c);
                else
                    MaximizeClientHorizontally(c);
            }
        }

        if (e->data.l[1] == (long)atoms[AtomNetWMStateMaximizedVert]
                || e->data.l[2] == (long)atoms[AtomNetWMStateMaximizedVert]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                RestoreClient(c);
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                MaximizeClientVertically(c);
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & NetWMStateMaximizedVert)
                    RestoreClient(c);
                else
                    MaximizeClientVertically(c);
            }
        }

        if (e->data.l[1] == (long)atoms[AtomNetWMStateHidden]
                || e->data.l[2] == (long)atoms[AtomNetWMStateHidden]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                RestoreClient(c);
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                MinimizeClient(c);
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & NetWMStateHidden)
                    RestoreClient(c);
                else
                    MinimizeClient(c);
            }
        }
    }

    /* XXX: we should keep track of icccm WMState (Iconic and Normal States)
     * but we use ewmh hidden state for now */
    if (e->message_type == atoms[AtomWMChangeState]) {
        if (c->states & NetWMStateHidden)
            RestoreClient(c);
        else
            MinimizeClient(c);
    }

    if (e->message_type == atoms[AtomNetActiveWindow]) {
        if (c->states & NetWMStateHidden)
            RestoreClient(c);
        SetActiveClient(c);
    }

    if (e->message_type == atoms[AtomNetCloseWindow]) {
        KillClient(c);
        SetActiveClient(NULL);
    }

    if (e->message_type == atoms[AtomNetWMActionMinimize]) {
        MinimizeClient(c);
        SetActiveClient(c);
    }

    if (e->message_type == atoms[AtomNetWMMoveresize]) {
        lastSeenPointerX = e->data.l[0];
        lastSeenPointerY = e->data.l[1];
        motionStartX = c->fx;
        motionStartY = c->fy;
        motionStartW = c->fw;
        motionStartH = c->fh;
        moveMessageType = e->data.l[2];
        if (moveMessageType <= HandleCount)
            XGrabPointer(display, c->frame, False, ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeSync, None, cursors[CursorMove],CurrentTime);
    }
}

void
OnEnter(XCrossingEvent *e)
{
    Client *c = NULL;

    if ((e->mode != NotifyNormal || e->detail == NotifyInferior) && e->window != root)
        return;

    c = LookupClient(e->window);

    if (c) {
        if (e->window == c->frame
                && (settings.focusFollowsPointer
                    ||  c->isTiled
                    || c->monitor != activeMonitor))
            SetActiveClient(c);

       for (int i = 0; i < ButtonCount; ++i) {
           if (e->window == c->buttons[i]) {
                c->hovered = i;
                RefreshClient(c);
                break;
           }
       }
    }
}

void
OnLeave(XCrossingEvent *e)
{
    Client *c = LookupClient(e->window);

    if (c) {
        for (int i = 0; i < ButtonCount; ++i) {
            if (e->window == c->buttons[i]) {
                c->hovered = ButtonCount;
                RefreshClient(c);
                break;
            }
        }
    }
}

void
OnKeyPress(XKeyPressedEvent *e)
{
    KeySym keysym;

    /* keysym = XkbKeycodeToKeysym(display, e->keycode, 0, e->state & ShiftMask ? 1 : 0); */
    keysym = XkbKeycodeToKeysym(display, e->keycode, 0, 0);

    /* there is no key binding in stack (see xbindeys or such for that)
     * but it might usefull to get a terminal */
    if (keysym == (XK_Return)
            && CleanMask(ModCtrlShift) == CleanMask(e->state)) {
        if (fork() == 0) {
            if (display)
              close(ConnectionNumber(display));
            setsid();
            execvp((char *)terminal[0], (char **)terminal);
            ELog("%s: failed", terminal[0]);
            exit(EXIT_SUCCESS);
        }
    }

    /* XXX: warning related to config. Won't work anymore if replacing TAB by
     * something else */
    if (keysym == (XK_Tab)
            && (CleanMask(Mod) == CleanMask(e->state)
                || CleanMask(ModShift) == CleanMask(e->state)))
        switching = True;

    /* shortcuts */
    for (int i = 0; i < ShortcutCount; ++i) {
        struct Shortcut *s = &settings.shortcuts[i];
        if (keysym == (s->keysym)
                && CleanMask(s->modifier) == CleanMask(e->state)) {
            if (s->type == VCB)
                s->cb.v_cb.f();
            if (s->type == CCB && activeClient)
                s->cb.c_cb.f(activeClient);
            if (s->type == MCB && activeMonitor)
                s->cb.m_cb.f(activeMonitor);
            if (s->type == CICB && activeClient)
                s->cb.ci_cb.f(activeClient, s->cb.ci_cb.i);
            if (s->type == MICB && activeMonitor)
                s->cb.mi_cb.f(activeMonitor, s->cb.mi_cb.i);
            if (s->type == MDCB && activeMonitor)
                s->cb.md_cb.f(activeMonitor, activeMonitor->activeDesktop);
            if (s->type == MCCB && activeClient)
                s->cb.mc_cb.f(activeClient->monitor, activeClient);
        }
    }
}

void
OnKeyRelease(XKeyReleasedEvent *e)
{
    KeySym keysym;

    keysym = XkbKeycodeToKeysym(display, e->keycode, 0, 0);
    if (keysym == (ModSym) && activeClient && switching) {
        StackClientBefore(activeMonitor, activeClient, activeMonitor->head);
        RefreshMonitor(activeClient->monitor);
        switching = False;
    }
}

void
Snap(int x, int y, int w, int h, int *xp,
        int *yp, int *wp, int *hp, int snap)
{
    /* boundaries */
    int l1 = x;
    int l2 = *xp;
    int r1 = x + w;
    int r2 = *xp + *wp;
    int t1 = y;
    int t2 = *yp;
    int b1 = y + h;
    int b2 = *yp + *hp;

    /* gaps */
    int ll = l1 - l2;
    int lr = l1 - r2;
    int rr = r1 - r2;
    int rl = r1 - l2;
    int tt = t1 - t2;
    int tb = t1 - b2;
    int bb = b1 - b2;
    int bt = b1 - t2;
    int all = abs(ll);
    int alr = abs(lr);
    int arl = abs(rl);
    int arr = abs(rr);
    int att = abs(tt);
    int atb = abs(tb);
    int abb = abs(bb);
    int abt = abs(bt);

    /* included */
    if (l1 - snap < l2 && r1 + snap > r2 && t1 - snap < t2 && b1 + snap > b2) {
        if (all < snap || arr < snap) {
            if (all > arr) *xp += rr; else *xp += ll;
        }
        if (att < snap || abb < snap) {
            if (att > abb) *yp += bb; else *yp += tt;
        }
    }

    /* disjoint aligned horizontally */
    if (((t1 < t2 && b1 > t2) || (t1 < b2 && b1 > b2))
            || ((t1 > t2) && (b1 < b2))
            || ((t1 < t2) && (b1 > b2))) {
        if (alr < snap) {
            *xp += lr; 
        }
        if (arl < snap) {
            *xp += rl; 
        }
    }

    /* disjoint aligned vertically */
    if (((l1 < l2 && r1 > l2) || (l1 < r2 && r1 > r2))
            || ((l1 > l2) && (r1 < r2))
            || ((l1 < l2) && (r1 > r2))) {
        if (atb < snap) {
            *yp += tb; 
        }
        if (abt < snap) {
            *yp += bt; 
        }
    }
}
