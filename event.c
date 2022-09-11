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
#include "manager.h"
#include "monitor.h"
#include "stack.h"

#define CleanMask(mask)\
    ((mask) & ~(numLockMask|LockMask) &\
         ( ShiftMask\
         | ControlMask\
         | Mod1Mask\
         | Mod2Mask\
         | Mod3Mask\
         | Mod4Mask\
         | Mod5Mask))

static XErrorHandler defaultErrorHandler = NULL;
static char *terminal[] = {"st", NULL};
static int lastSeenPointerX;
static int lastSeenPointerY;
static Time lastSeenPointerTime;
static int motionStartX;
static int motionStartY;
static int motionStartW;
static int motionStartH;
static Bool switching;
static Bool running;

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
        struct timeval timeout = { 0, 2500 };
        FD_ZERO(&fdSet);
        FD_SET(xConnection, &fdSet);

        if (select(xConnection + 1, &fdSet, NULL, NULL, &timeout) > 0) {
            while (XPending(display)) {
                // TODO: replace by an array
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
                    case ConfigureRequest:
                        OnConfigureRequest(&e.xconfigurerequest);
                    break;
                    case Expose:
                        OnExpose(&e.xexpose);
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
        DLog("%ld: %s", e->resourceid, message);
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
        XSync(display, False);
    } else {
        if (e->value_mask & CWBorderWidth)
            c->sbw = e->border_width;

        if (e->value_mask & (CWX|CWY|CWWidth|CWHeight)
                && !(c->states & NetWMStateMaximized))
                {
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

            MoveResizeClientWindow(c, x, y, w, h, False);

            XConfigureEvent xce;
            xce.event = e->window;
            xce.window = e->window;
            xce.type = ConfigureNotify;
            xce.x = c->wx;
            xce.y = c->wy;
            xce.width = c->ww;
            xce.height = c->wh;
            xce.border_width = 0;
            xce.above = None;
            xce.override_redirect = 0;
            XSendEvent(display, e->window, False, SubstructureNotifyMask, (XEvent*)&xce);
            XSync(display, False);
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
}

void
OnMapRequest(XMapRequestEvent *e)
{
    XWindowAttributes wa;

    if (!XGetWindowAttributes(display, e->window, &wa))
        return;

    if (wa.override_redirect)
        return;

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
    if (c && e->window == c->frame)
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
    Client *c = LookupClient(e->window);
    if (!c)
        return;

    lastSeenPointerX = e->x_root;
    lastSeenPointerY = e->y_root;
    motionStartX = c->fx;
    motionStartY = c->fy;
    motionStartW = c->fw;
    motionStartH = c->fh;

    if (!c->tiled) {
        if (e->window == c->topbar || (e->window == c->window && e->state == Modkey))
            XDefineCursor(display, e->window, cursors[CursorMove]);

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
}

void
OnButtonRelease(XButtonEvent *e)
{
    Client *c = LookupClient(e->window);
    if (!c)
        return;

    if (!c->tiled) {
        for (int i = 0; i < HandleCount; ++i) {
            if (e->window == c->handles[i]) {
                /* apply the size hints */
                MoveResizeClientFrame(c, c->fx, c->fy, c->fw, c->fh, True);
                break;
            }
        }
    }

    if (e->window == c->topbar || e->window == c->window)
        XDefineCursor(display, e->window, cursors[CursorNormal]);

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
            || (e->time - lastSeenPointerTime) <= (1000 / 60))
        return;

    int vx = e->x_root - lastSeenPointerX;
    int vy = e->y_root - lastSeenPointerY;

    lastSeenPointerTime = e->time;

    if (c->tiled) {
        if (e->window == c->handles[HandleWest]
                || e->window == c->handles[HandleEast]) {
            c->monitor->desktops[c->desktop].split =
                (e->x_root - c->monitor->x) / (float)c->monitor->w;
            RestackMonitor(c->monitor);
        }
    } else {
        /* we do not apply normal hints during motion but when button is released
         * to make the resizing visually smoother. Some client apply normals by
         * themselves anway (e.g gnome-terminal) */
        if (e->window == c->topbar || e->window == c->window)
            MoveClientFrame(c, motionStartX + vx, motionStartY + vy);
        else if (e->window == c->handles[HandleNorth])
            MoveResizeClientFrame(c, motionStartX, motionStartY + vy,
                    motionStartW, motionStartH - vy , False);
        else if (e->window == c->handles[HandleWest])
            ResizeClientFrame(c, motionStartW + vx, motionStartH, False);
        else if (e->window == c->handles[HandleSouth])
            ResizeClientFrame(c, motionStartW, motionStartH + vy, False);
        else if (e->window == c->handles[HandleEast])
            MoveResizeClientFrame(c, motionStartX + vx, motionStartY,
                    motionStartW - vx, motionStartH, False);
        else if (e->window == c->handles[HandleNorthEast])
            MoveResizeClientFrame(c, motionStartX + vx, motionStartY + vy,
                    motionStartW - vx, motionStartH - vy, False);
        else if (e->window == c->handles[HandleNorthWest])
            MoveResizeClientFrame(c, motionStartX, motionStartY + vy,
                    motionStartW + vx, motionStartH - vy, False);
        else if (e->window == c->handles[HandleSouthWest])
            ResizeClientFrame(c, motionStartW + vx, motionStartH + vy, False);
        else if (e->window == c->handles[HandleSouthEast])
            MoveResizeClientFrame(c, motionStartX + vx, motionStartY,
                    motionStartW - vx, motionStartH + vy, False);
        else
          return;
    }
}

void
OnMessage(XClientMessageEvent *e)
{
    /* TODO: root active window and Curent desktop
     * Especially since we have advertised supported actions
     * _NET_CLOSE_WINDOW
     * _NET_MOVERESIZE_WINDOW
     * _NET_WM_MOVERESIZE
     * _NET_RESTACK_WINDOW */

    if (e->window == root) {
        DLog("Root received: %ld",e->message_type);
        if (e->message_type == atoms[AtomNetCurrentDesktop])
            ShowDesktop(e->data.l[0]);

        /*
         * specs say message MUST be send to root but it seems
         * most pagers send to the client
         */
        if (e->message_type == atoms[AtomNetActiveWindow]) {
            Client *toActivate = LookupClient(e->window);
            if (toActivate) {
                // TODO: not on the same monitor!
                // XXX: move to setActiveClient
                //if (activeMonitor->activeDesktop != toActivate->desktop)
                //    ShowDesktop(toActivate->desktop);
                SetActiveClient(toActivate);
            }
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
        /* TODO: maximixed, minimized sticky (mainly for CSD) */
    }

    // TODO: does not work like this obviously!
    //if (e->message_type == atoms[AtomWMProtocols]
    //        && e->data.l[0] == (long)atoms[AtomWMDeleteWindow]) {
    //    XGrabServer(display);
    //    XSetErrorHandler(DummyErrorHandler);
    //    XSetCloseDownMode(display, DestroyAll);
    //    XKillClient(display, c->window);
    //    XSync(display, False);
    //    XSetErrorHandler(EventLoopErrorHandler);
    //    XUngrabServer(display);
    //}

    if (e->message_type == atoms[AtomNetActiveWindow]) {
        // TODO: not on the same monitor!
        // XXX: Move to setActiveClient
        // if (activeMonitor->activeDesktop != c->desktop)
        //         ShowDesktop(c->desktop);
        if (c->states & NetWMStateHidden)
            RestoreClient(c);
        SetActiveClient(c);
    }
}

void
OnEnter(XCrossingEvent *e)
{
    Client *c = NULL;

    if ((e->mode != NotifyNormal || e->detail == NotifyInferior) && e->window != root)
        return;

    c = LookupClient(e->window);
    if (!c)
        return;

    if (c) {
        /* TODO: should be configurable */
        if (c->tiled && e->window == c->frame)
            SetActiveClient(c);
        for (int i = 0; i < ButtonCount; ++i)
            if (e->window == c->buttons[i])
                RefreshClientButton(c, i, True);
    }
}

void OnLeave(XCrossingEvent *e)
{
    Client *c = LookupClient(e->window);

    if (c)
        for (int i = 0; i < ButtonCount; ++i)
            if (e->window == c->buttons[i])
                RefreshClientButton(c, i, False);
}

void
OnKeyPress(XKeyPressedEvent *e)
{
    KeySym keysym;

    /* keysym = XkbKeycodeToKeysym(display, e->keycode, 0, e->state & ShiftMask ? 1 : 0); */
    keysym = XkbKeycodeToKeysym(display, e->keycode, 0, 0);

    /* there is no key binding in stack (see xbindeys or such for that)
     * but it might usefull to get a terminal */
    if (keysym == (XK_Return) && CleanMask(Modkey|ShiftMask) == CleanMask(e->state)) {
        if (fork() == 0) {
            if (display)
              close(ConnectionNumber(display));
            setsid();
            execvp((char *)terminal[0], (char **)terminal);
            ELog("%s: failed", terminal[0]);
            exit(EXIT_SUCCESS);
        }
    }

    /* Warning related to config. Won't work anymore if replacing TAB by
     * something else */
    if (keysym == (XK_Tab)
            && (CleanMask(Modkey) == CleanMask(e->state)
                || CleanMask(Modkey|ShiftMask) == CleanMask(e->state)))
        switching = True;

    /* shortcuts */
    for (int i = 0; i < ShortcutCount; ++i) {
        if (keysym == (config.shortcuts[i].keysym) &&
                CleanMask(config.shortcuts[i].modifier) == CleanMask(e->state)) {
            if (config.shortcuts[i].type == CV)
                config.shortcuts[i].cb.vcb.f();
            if (config.shortcuts[i].type == CI)
                config.shortcuts[i].cb.icb.f(config.shortcuts[i].cb.icb.i);
            if (config.shortcuts[i].type == CC && activeClient)
                config.shortcuts[i].cb.ccb.f(activeClient);
        }
    }
}

void
OnKeyRelease(XKeyReleasedEvent *e)
{
    KeySym keysym;

    keysym = XkbKeycodeToKeysym(display, e->keycode, 0, 0);
    if (keysym == (ModkeySym)
            && activeClient
            && switching) {
        StackClientBefore(activeClient, activeClient->monitor->head);
        if (activeClient->tiled)
            RestackMonitor(activeClient->monitor);
        switching = False;
    }
}

