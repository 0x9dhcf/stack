#include <unistd.h>

#include <X11/XKBlib.h>

#include "atoms.h"
#include "client.h"
#include "config.h"
#include "cursors.h"
#include "events.h"
#include "font.h"
#include "hints.h"
#include "manage.h"
#include "monitor.h"
#include "stack.h"
#include "log.h"
#include "x11.h"

static char *terminal[] = {"xterm", NULL};

static int  lastSeenPointerX;
static int  lastSeenPointerY;
static Time lastSeenPointerTime;
static int  motionStartX;
static int  motionStartY;
static int  motionStartW;
static int  motionStartH;

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
static void Spawn(char **args);

void
DispatchEvent(XEvent *e)
{
    switch(e->type) {
        case MapRequest:
            OnMapRequest(&e->xmaprequest);
        break;
        case UnmapNotify:
            OnUnmapNotify(&e->xunmap);
        break;
        case DestroyNotify:
            OnDestroyNotify(&e->xdestroywindow);
        break;
        case ConfigureRequest:
            OnConfigureRequest(&e->xconfigurerequest);
        break;
        case Expose:
            OnExpose(&e->xexpose);
        break;
        case PropertyNotify:
            OnPropertyNotify(&e->xproperty);
        break;
        case ButtonPress:
            OnButtonPress(&e->xbutton);
        break;
        case ButtonRelease:
            OnButtonRelease(&e->xbutton);
        break;
        case MotionNotify:
            OnMotionNotify(&e->xmotion);
        break;
        case EnterNotify:
            OnEnter(&e->xcrossing);
        break;
        case LeaveNotify:
            OnLeave(&e->xcrossing);
        break;
        case ClientMessage:
            OnMessage(&e->xclient);
        break;
        case KeyPress:
            OnKeyPress(&e->xkey);
        break;
    }
}

void
OnConfigureRequest(XConfigureRequestEvent *e)
{
    Client *c = Lookup(e->window);

    if (!c) {
        XWindowChanges xwc;
        xwc.x = e->x;
        xwc.y = e->y;
        xwc.width = e->width;
        xwc.height = e->height;
        xwc.border_width = e->border_width;
        xwc.sibling = e->above;
        xwc.stack_mode = e->detail;
        XConfigureWindow(stDisplay, e->window, e->value_mask, &xwc);
        XSync(stDisplay, False);
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
            XSendEvent(stDisplay, e->window, False, SubstructureNotifyMask, (XEvent*)&e);
            XSync(stDisplay, False);
        }

        /* As the window is reparented, XRaiseWindow and XLowerWindow
           will generate a ConfigureRequest. */
        if (e->value_mask & CWStackMode) {
            if (e->detail == Above || e->detail == TopIf)
                RaiseClient(c);
            if (e->detail == Below || e->detail == BottomIf)
                LowerClient(c);
            SetNetWMState(c->window, c->states);
        }
    }
}

void
OnMapRequest(XMapRequestEvent *e)
{
    ManageWindow(e->window, False);
}

void
OnUnmapNotify(XUnmapEvent *e)
{
    /* ignore UnmapNotify from reparenting  */
    if (e->event != stRoot && e->event != None)
        ForgetWindow(e->window, False);
}

/* XXX: useless */
void
OnDestroyNotify(XDestroyWindowEvent *e)
{
    Client *c = Lookup(e->window);
    if (c) { ELog("Found a client??"); }
}

void
OnExpose(XExposeEvent *e)
{
    Client *c = Lookup(e->window);
    if (c && e->window == c->frame)
        RefreshClient(c);
}

void
OnPropertyNotify(XPropertyEvent *e)
{
    Client *c = Lookup(e->window);
    if (!c)
        return;

    if (!c || e->window != c->window)
        return;

    if (e->atom == XA_WM_NAME || e->atom == stAtoms[AtomNetWMName]) {
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
    if (e->atom == stAtoms[AtomNetWMState])
        UpdateClientState(c);
    */
}

void
OnButtonPress(XButtonEvent *e)
{
    Client *c = Lookup(e->window);
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
            XDefineCursor(stDisplay, e->window, stCursors[CursorMove]);

        if (e->window == c->buttons[ButtonMaximize]) {
            if ((c->states & NetWMStateMaximized)) {
                RestoreClient(c);
            } else {
                MaximizeClient(c);
            }
        }

        if (e->window == c->buttons[ButtonMinimize]) {
            MinimizeClient(c);
            FindNextActiveClient();
        }

    }

    if (e->window == c->buttons[ButtonClose]) {
        if (c->protocols & NetWMProtocolDeleteWindow)
            SendMessage(c->window, stAtoms[AtomWMDeleteWindow]);
        else
            XKillClient(stDisplay, c->window);
    }

    if (e->window != c->buttons[ButtonClose] && e->window != c->buttons[ButtonMinimize])
        SetActiveClient(c);

    if (e->window == c->window)
        XAllowEvents(stDisplay, ReplayPointer, CurrentTime);
}

void
OnButtonRelease(XButtonEvent *e)
{
    Client *c = Lookup(e->window);
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
        XDefineCursor(stDisplay, e->window, stCursors[CursorNormal]);

    if (e->window == c->window)
        XAllowEvents(stDisplay, ReplayPointer, CurrentTime);
}

void
OnMotionNotify(XMotionEvent *e)
{
    Client *c = Lookup(e->window);

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
            Restack(c->monitor);
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
    /* TODO: root active window and Cureent desktop */
    Client *c = Lookup(e->window);
    if (!c)
        return;

    if (e->message_type == stAtoms[AtomNetWMState]) {
        if (e->data.l[1] == (long)stAtoms[AtomNetWMStateFullscreen]
                || e->data.l[2] == (long)stAtoms[AtomNetWMStateFullscreen]) {
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
        if (e->data.l[1] == (long)stAtoms[AtomNetWMStateDemandsAttention]
                || e->data.l[2] == (long)stAtoms[AtomNetWMStateDemandsAttention]) {
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

        /* TODO: maximixed, minimized sticky (mainly for CSD) */
    }

    if (e->message_type == stAtoms[AtomNetActiveWindow])
        XSetInputFocus(stDisplay, c->window, RevertToPointerRoot, CurrentTime);
}

void
OnEnter(XCrossingEvent *e)
{
    Client *c = Lookup(e->window);

    if (c)
        for (int i = 0; i < ButtonCount; ++i)
            if (e->window == c->buttons[i])
                RefreshClientButton(c, i, True);
}

void OnLeave(XCrossingEvent *e)
{
    Client *c = Lookup(e->window);

    if (c)
        for (int i = 0; i < ButtonCount; ++i)
            if (e->window == c->buttons[i])
                RefreshClientButton(c, i, False);
}

void
OnKeyPress(XKeyPressedEvent *e)
{
    KeySym keysym;
    /* keysym = XkbKeycodeToKeysym(stDisplay, e->keycode, 0, e->state & ShiftMask ? 1 : 0); */
    keysym = XkbKeycodeToKeysym(stDisplay, e->keycode, 0, 0);

    /* there is no key binding in stack (see xbindeys or such for that)
     * , but might usefull to get a terminal */ 
    if (keysym == (XK_Return) && CleanMask(Modkey|ShiftMask) == CleanMask(e->state))
        Spawn((char**)terminal);

    /* shortcuts */
    for (int i = 0; i < ShortcutCount; ++i) {
        Shortcut sc = stConfig.shortcuts[i];
        if (keysym == (sc.keysym) &&
                CleanMask(sc.modifier) == CleanMask(e->state)) {
            if (sc.type == CV) sc.cb.vcb.f();
            if (sc.type == CI) sc.cb.icb.f(sc.cb.icb.i);
            if (sc.type == CC) sc.cb.ccb.f(stActiveClient);
            break;
        }
    }
}

void
Spawn(char **args)
{
    if (fork() == 0) {
        if (stDisplay)
              close(ConnectionNumber(stDisplay));
        setsid();
        execvp((char *)args[0], (char **)args);
        ELog("%s: failed", args[0]);
        exit(EXIT_SUCCESS);
    }
}
