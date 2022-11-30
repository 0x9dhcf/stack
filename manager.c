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
#include "hints.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "settings.h"
#include "x11.h"

#define RootEventMask (\
          ButtonPressMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define FrameEvenMask (\
          ExposureMask\
        | ButtonPressMask\
        | EnterWindowMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)

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

static XErrorHandler defaultErrorHandler = NULL;
static char *terminal[] = {"st", NULL};
static int lastSeenPointerX = 0;
static int lastSeenPointerY = 0;
static Time lastSeenPointerTime = 0;
static Time lastClickPointerTime = 0;
static int motionStartX = 0;
static int motionStartY = 0;
static int motionStartW = 0;
static int motionStartH = 0;
static Bool switching = 0;
static Bool running = 0;
static Window supportingWindow;
static Client *lastActiveClient = NULL;
static int WMDetectedErrorHandler(Display *d, XErrorEvent *e);

static int moveMessageType = HandleCount + 1;

Monitor *activeMonitor;
Client  *activeClient;

void
SetupWindowManager()
{
    XSetWindowAttributes wa;
    XErrorHandler h;
    Window *wins,  w0, w1, rwin, cwin;
    unsigned int nwins, mask;
    int rx, ry, wx, wy;
    KeyCode code;
    unsigned int modifiers[] = { 0, LockMask, numLockMask, numLockMask | LockMask };

    /* check for existing wm */
    h = XSetErrorHandler(WMDetectedErrorHandler);
    XSelectInput(display, root, SubstructureRedirectMask);
    XSync(display, False);
    XSetErrorHandler(h);
    XSync(display, False);

    /* Setup */
    activeMonitor = monitors;
    if (! activeMonitor)
        FLog("no monitor detected.");

    supportingWindow = XCreateSimpleWindow(display, root,
            0, 0, 1, 1, 0, 0, 0);

    XChangeProperty(display, supportingWindow,
            atoms[AtomNetSupportingWMCheck], XA_WINDOW, 32,
            PropModeReplace, (unsigned char *) &supportingWindow, 1);
    XChangeProperty(display, supportingWindow, XA_WM_NAME, XA_STRING, 8,
            PropModeReplace, (unsigned char*)"stack", 5);
    XChangeProperty(display, supportingWindow, atoms[AtomNetWMPID],
            XA_INTEGER, 32, PropModeReplace,
            (unsigned char *) &supportingWindow, 1);

    /* Configure the root window */
    XChangeProperty(display, root, atoms[AtomNetSupportingWMCheck],
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char *) &supportingWindow, 1);
    XChangeProperty(display, root, XA_WM_NAME, XA_STRING, 8,
            PropModeReplace, (unsigned char*)"stack", 5);
    XChangeProperty(display, root, atoms[AtomNetWMPID], XA_INTEGER, 32,
            PropModeReplace, (unsigned char *) &supportingWindow, 1);
    XChangeProperty(display, root, atoms[AtomNetSupported], XA_ATOM, 32,
        PropModeReplace, (unsigned char *) atoms, AtomCount);

    wa.cursor = cursors[CursorNormal];
    wa.event_mask = RootEventMask;
    XChangeWindowAttributes(display, root, CWEventMask | CWCursor, &wa);

    /* let ewmh listeners know about what is supported. */
    XChangeProperty(display, root, atoms[AtomNetSupported], XA_ATOM, 32,
            PropModeReplace, (unsigned char *) atoms , AtomCount);

    /* inform about the number of desktops */
    int desktops = DesktopCount;
    XChangeProperty(display, root, atoms[AtomNetNumberOfDesktops], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char *)&desktops, 1);

    /* use the 1st desktop of the active monitor as the current one */
    XChangeProperty(display, root, atoms[AtomNetCurrentDesktop],
            XA_CARDINAL, 32, PropModeReplace,
            (unsigned char *)&monitors->activeDesktop, 1);

    /* Reset the client list. */
    XDeleteProperty(display, root, atoms[AtomNetClientList]);

    /* manage exiting windows */
    XQueryPointer(display, root, &rwin, &cwin, &rx, &ry, &wx, &wy, &mask);

    /* to avoid unmap generated by reparenting to destroy the newly created client */
    if (XQueryTree(display, root, &w0, &w1, &wins, &nwins)) {
        for (unsigned int i = 0; i < nwins; ++i) {
            XWindowAttributes xwa;

            if (wins[i] == supportingWindow)
                continue;

            XGetWindowAttributes(display, wins[i], &xwa);
            if (!xwa.override_redirect) {
                ManageWindow(wins[i], True);
                /* reparenting does not set the e->event to root
                 * we discar events to prevent the unmap to be catched
                 * and wrongly interpreted */
                XSync(display, True);
                XSetInputFocus(display, wins[i], RevertToPointerRoot,
                        CurrentTime);
                XChangeProperty(display, root, atoms[AtomNetClientList],
                        XA_WINDOW, 32, PropModeAppend,
                        (unsigned char *) &wins[i], 1);
            }
        }
        XFree(wins);
    }

    /* grab shortcuts */
    XUngrabKey(display, AnyKey, AnyModifier, root);

    /* terminal */
    if ((code = XKeysymToKeycode(display, XK_Return)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(display, code, Modkey | ShiftMask | modifiers[j],
                    root, True, GrabModeSync, GrabModeAsync);

    /* switching XXX: hardcoded */
    if ((code = XKeysymToKeycode(display, ModkeySym)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(display, code, modifiers[j],
                    root, True, GrabModeAsync, GrabModeAsync);
    if ((code = XKeysymToKeycode(display, ModkeySym)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(display, code, ShiftMask | modifiers[j],
                    root, True, GrabModeAsync, GrabModeAsync);

    /* shortcuts */
    // XXX: (some) shortcuts could be grab on the client like buttons
    for (int i = 0; i < ShortcutCount; ++i) {
        if ((code = XKeysymToKeycode(display, settings.shortcuts[i].keysym))) {
            for (int j = 0; j < 4; ++j)
                XGrabKey(display, code,
                        settings.shortcuts[i].modifier | modifiers[j],
                        root, True, GrabModeSync, GrabModeAsync);
        }
    }

    /* finally exec autostart */
    ExecAutostartFile();
}

void
CleanupWindowManager()
{
    /* teardown */
    for (Monitor *m = monitors; m; m = m->next) {
        Client *c, *d;
        for (c = m->head, d = c ? c->next : 0; c; c = d, d = c ? c->next : 0)
            UnmanageWindow(c->window, False);
    }

    XDestroyWindow(display, supportingWindow);
    XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XSelectInput(display, root, NoEventMask);
}

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

void
ManageWindow(Window w, Bool mapped)
{
    DLog("%ld mapped: %d", w, mapped);
    Window r = None, t = None;
    int wx = 0, wy = 0 ;
    unsigned int ww, wh, d, b;
    Bool decorated;

    Client *c = malloc(sizeof(Client));
    if (!c) {
        ELog("can't allocate client.");
        return;
    }
    memset(c, 0, sizeof(Client));

    XAddToSaveSet(display, w);

    /* get info about the window */
    XGetGeometry(display, w, &r, &wx, &wy, &ww, &wh, &b, &d);
    XGetTransientForHint(display, w, &t);
    GetWMProtocols(w, &c->protocols);
    GetNetWMWindowType(w, &c->types);
    GetWMHints(w, &c->hints);
    GetNetWMStates(w, &c->states);
    GetWMNormals(w, &c->normals);
    GetWMName(w, &c->name);
    GetWMClass(w, &c->wmclass);
    GetWMStrut(w, &c->strut);
    GetMotifHints(w, &c->motifs);

    decorated = (c->types & NetWMTypeFixed
            || (c->motifs.flags == 0x2 && c->motifs.decorations == 0))
        ? False : True;
    c->isBorderVisible = decorated;
    c->isTopbarVisible = decorated;
    c->hasTopbar = decorated && !(c->types & NetWMTypeNoTopbar);
    c->hasHandles = decorated && !IsFixed(c->normals);
    c->isActive = False;
    c->isTiled = False;
    c->hovered = ButtonCount;
    c->desktop = -1;

    /* if transient for, register the client we are transient for
     * and regiter this new client as a transient of this
     * transient for client */
    if (t != None) {
        Transient *tc = malloc(sizeof(Transient));
        if (tc) {
            c->transfor = LookupClient(t);
            if (c->transfor) {
                /* attach tc to the transient for client */
                tc->client = c;
                tc->next = c->transfor->transients;
                c->transfor->transients = tc;
                /* center to the current center of the client
                 * we are transient for */
                wx = c->transfor->fx + (c->transfor->fw - ww) / 2;
                wy = c->transfor->fy + (c->transfor->fh - wh) / 2;
            } else {
                ELog("can't find transient for.");
                free(tc);
            }
        } else {
            ELog("can't allocate transient.");
        }
    }

    /* create and configure windows */
    /* we always frame the window */
    XSetWindowAttributes fattrs = {0};
    fattrs.event_mask = FrameEvenMask;
    fattrs.backing_store = WhenMapped;
    c->frame = XCreateWindow(display, root, 0, 0, 1, 1, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWEventMask | CWBackingStore, &fattrs);

    /* client */
    c->window = w;
    XReparentWindow(display, w, c->frame, 0, 0);
    XSetWindowBorderWidth(display, w, 0);
    if (c->hints & HintsFocusable && !(c->types & NetWMTypeFixed)) {
        /* allows activation on click */
        XUngrabButton(display, AnyButton, AnyModifier, c->window);
        XGrabButton(display, Button1, AnyModifier, c->window, False,
                ButtonPressMask, GrabModeSync,
                GrabModeSync, None, None);
    }

    /*
     * windows with EWMH type fixed are neither moveable,
     * resizable nor decorated. While windows fixed by ICCCM normals
     * are decorated and moveable but not resizable)
     */
    if (!(c->types & NetWMTypeFixed)) {
        long state[] = {NormalState, None};
        XChangeProperty(display, w, atoms[AtomWMState],
                atoms[AtomWMState], 32, PropModeReplace,
                (unsigned char *)state, 2);

        /* topbar */
        XSetWindowAttributes tattrs = {0};
        tattrs.event_mask = HandleEventMask;
        tattrs.cursor = cursors[CursorNormal];
        c->topbar = XCreateWindow(display, c->frame, 0, 0, 1, 1, 0,
                CopyFromParent, InputOnly, CopyFromParent,
                CWEventMask | CWCursor, &tattrs);

        /* buttons */
        XSetWindowAttributes battrs = {0};
        battrs.event_mask = ButtonEventMask;
        for (int i = 0; i < ButtonCount; ++i) {
            Window b = XCreateWindow(display, c->topbar, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOnly, CopyFromParent,
                    CWEventMask | CWCursor, &battrs);
            c->buttons[i] = b;
        }
    }

    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals)) {
        /* handles */
        XSetWindowAttributes hattrs = {0};
        hattrs.event_mask = HandleEventMask;
        for (int i = 0; i < HandleCount; ++i) {
            hattrs.cursor = cursors[CursorResizeNorthEast + i];
            Window h = XCreateWindow(display, root, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOnly, CopyFromParent,
                    CWEventMask | CWCursor, &hattrs);
            c->handles[i] = h;
        }
    }

    c->sbw = b;
    Desktop *dp = &activeMonitor->desktops[activeMonitor->activeDesktop];
    c->ww = Min(dp->ww, (int)ww);
    c->wh = Min(dp->wh, (int)wh);
    c->wx = Min(Max(dp->wx, wx), dp->wx + dp->ww - (int)ww);
    c->wy = Min(Max(dp->wy, wy), dp->wy + dp->wh - (int)wh);
    SynchronizeFrameGeometry(c);
    AttachClientToMonitor(activeMonitor, c);

    /* we can only apply structure states once attached in floating mode */
    if (! activeMonitor->desktops[activeMonitor->activeDesktop].dynamic) {
        CenterClient(c);
        if (c->states & NetWMStateMaximizedHorz)
            MaximizeClientHorizontally(c);
        if (c->states & NetWMStateMaximizedVert)
            MaximizeClientVertically(c);
        if (c->states & NetWMStateFullscreen)
            FullscreenClient(c);
    } else {
        RestackMonitor(c->monitor);
    }

    /* map */
    XMapWindow(display, c->frame);
    if (! mapped)
        XMapWindow(display, w);
    if (!(c->types & NetWMTypeFixed)) {
        XMapWindow(display, c->topbar);
        for (int i = 0; i < ButtonCount; ++i) {
            XMapWindow(display, c->buttons[i]);
        }
    }
    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals)) {
        for (int i = 0; i < HandleCount; ++i) {
            XMapWindow(display, c->handles[i]);
        }
    }

    if (!(c->types & NetWMTypeFixed)) {
        SetActiveClient(c);
        /* let anyone interrested in ewmh knows what we honor */
        SetNetWMAllowedActions(w, NetWMActionDefault);
    }

    /* update the client list */
    XChangeProperty(display, root, atoms[AtomNetClientList], XA_WINDOW,
            32, PropModeAppend, (unsigned char *) &(w), 1);
}

void
UnmanageWindow(Window w, Bool destroyed)
{
    Client *c = LookupClient(w);
    Transient *t;

    if (!c)
        return;

    DLog("%ld destroyed: %d", w, destroyed);

    /* if some transients release them */
    t = c->transients;
    while (t) {
        Transient *p = t->next;
        free(t);
        t = p;
    }

    /* if transient for someone, unregister this client */
    if (c->transfor) {
        Transient **tc;
        for (tc = &c->transfor->transients;
                *tc && (*tc)->client != c; tc = &(*tc)->next);
        *tc = (*tc)->next;

        /* we want the next active to be the transient for */
        lastActiveClient = c->transfor;
    }

    if (c == lastActiveClient)
        lastActiveClient = NULL;

    if (c == activeClient)
        activeClient = NULL;

    if (c->monitor->desktops[c->desktop].activeOnLeave == c)
        c->monitor->desktops[c->desktop].activeOnLeave = NULL;

    DetachClientFromMonitor(c->monitor, c);

    if (c->name)
        free(c->name);

    if (c->wmclass.cname)
        free(c->wmclass.cname);

    if (c->wmclass.iname)
        free(c->wmclass.iname);

    if (! destroyed) {
        //long state[] = {WithdrawnState, None};
        XGrabServer(display);
        XSetErrorHandler(DisableErrorHandler);
        XUngrabButton(display, AnyButton, AnyModifier, c->window);
        XSetWindowBorderWidth(display, c->window, c->sbw);
        XReparentWindow(display, c->window, root, c->wx, c->wy);
        //XChangeProperty(display, c->window, atoms[AtomWMState],
        //    atoms[AtomWMState], 32, PropModeReplace,
        //    (unsigned char *)state, 2);
        XSync(display, False);
        XSetErrorHandler(EnableErrorHandler);
        XUngrabServer(display);
    }

    if (!(c->types & NetWMTypeFixed)) {
        for (int i = 0; i < ButtonCount; ++i)
            XDestroyWindow(display, c->buttons[i]);
        XDestroyWindow(display, c->topbar);
    }

    if (!(c->types & NetWMTypeFixed) && ! IsFixed(c->normals))
        for (int i = 0; i < HandleCount; ++i)
            XDestroyWindow(display, c->handles[i]);

    XDestroyWindow(display, c->frame);

    if (destroyed)
        XRemoveFromSaveSet(display, c->window);

    free(c);

    /* update the client list */
    XDeleteProperty(display, root, atoms[AtomNetClientList]);
    for (Monitor *m = monitors; m; m = m->next)
        for (Client *c2 = m->head; c2; c2 = c2->next)
            XChangeProperty(display, root, atoms[AtomNetClientList],
                    XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *) &c2->window, 1);

    if (! activeClient)
        SetActiveClient(NULL);
}

Client *
LookupClient(Window w)
{
    for (Monitor *m = monitors; m; m = m->next) {
        for (Client *c = m->head; c; c = c->next) {
            if (c->window == w || c->frame == w || c->topbar == w)
                return c;

            for (int i = 0; i < ButtonCount; ++i)
                if (c->buttons[i] == w)
                    return c;

            for (int i = 0; i < HandleCount; ++i)
                if (c->handles[i] == w)
                    return c;
        };
    }
    return NULL;
}
void
ReloadConfig()
{
    LoadConfigFile();
    for (Monitor *m = monitors; m; m = m->next) {
        for (int i = 0; i < DesktopCount; ++i) {
            m->desktops[i].masters = settings.masters;
            m->desktops[i].split = settings.split;
        }
        for (Client *c = m->head; c; c = c->next) {
            SynchronizeWindowGeometry(c);
            Configure(c);
        }
    }
    RestackMonitor(activeMonitor);
}

void
SwitchToNextClient()
{
    Client *h = activeClient ? activeClient : activeMonitor->head;
    for (Client *c = h ? h->next ? h->next : h->monitor->head : h;
            c != h; c = c->next ? c->next : h->monitor->head) {
        if (c->desktop == h->desktop && c->types & NetWMTypeNormal) {
            if (c->states & NetWMStateHidden)
                RestoreClient(c);
            SetActiveClient(c);
            break;
        }
    }
}

void
SwitchToPreviousClient()
{
    Client *t = activeClient ? activeClient : activeMonitor->tail;
    for (Client *c = t ? t->prev ? t->prev : t->monitor->tail : t;
            c != t; c = c->prev ? c->prev : t->monitor->tail) {
        if (c->desktop == t->desktop && c->types & NetWMTypeNormal) {
            if (c->states & NetWMStateHidden)
                RestoreClient(c);
            SetActiveClient(c);
            break;
        }
    }
}

void
SetActiveMonitor(Monitor *m)
{
    if (m)
        activeMonitor = m;
    else
        activeMonitor = monitors;
    // SetMonitorActive(activeMonitor) ???
}

void
SwitchToNextMonitor()
{
    SetActiveMonitor(activeMonitor->next ? activeMonitor->next : monitors);
    SetActiveClient(NULL);
}

void
SwitchToPreviousMonitor()
{
    Monitor *m = monitors;
    for (; m && m->next != activeMonitor; m = m->next);
    SetActiveMonitor(m);
    SetActiveClient(NULL);
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
SetActiveClient(Client *c)
{
    Client *toActivate = c;

    if (activeClient && activeClient == toActivate)
        return;

    if (!toActivate) {
        /* we need to find a new one to activate */
        if (lastActiveClient
                && lastActiveClient->monitor == activeMonitor
                && lastActiveClient->desktop == activeMonitor->activeDesktop
                && !(lastActiveClient->states & NetWMStateHidden)) {
            /*
             * if the last active client is on the
             * active monitor's active desktop
             * then use it
             */
            toActivate = lastActiveClient;
        } else {
            /*
             * try to find a new one to activate, the next  normal non hidden
             * in the stack of the active monitor's active desktop
             */
            Client *head = activeClient && activeClient->monitor == activeMonitor ?
                activeClient : activeMonitor->head;
            if ((toActivate = head)) {
                do {
                    if (toActivate->desktop == activeMonitor->activeDesktop
                                && toActivate->types & NetWMTypeNormal
                                && !(toActivate->states & NetWMStateHidden))
                        break;
                    toActivate = toActivate->next ?
                        toActivate->next : toActivate->monitor->head;
                } while (toActivate != head);
            }
        }
    }

    /* the last active is now the current active (could be NULL) */
    lastActiveClient = activeClient;

    /* the current active, if exists, should no more be active */
    if (activeClient) {
        SetClientActive(activeClient, False);
        //activeClient->states |= NetWMStateBelow;
        //activeClient->states &= ~NetWMStateAbove;
        SetNetWMStates(activeClient->window, activeClient->states);
        activeClient = NULL;
    }

    // XXX: focusable client to be checked
    if (toActivate && toActivate->desktop == activeMonitor->activeDesktop
            //&& toActivate->hints & HintsFocusable
            && toActivate->types & NetWMTypeNormal
            && !(toActivate->states & NetWMStateHidden)) {
        /* if someone is to be activated do it */
        SetClientActive(toActivate, True);
        activeClient = toActivate;
        RaiseClient(activeClient);
        if (toActivate->monitor != activeMonitor)
            SetActiveMonitor(toActivate->monitor);
        XChangeProperty(display, root, atoms[AtomNetActiveWindow],
                XA_WINDOW, 32, PropModeReplace,
                (unsigned char *) &toActivate->window, 1);
    } else {
        /* otherwise let anybody know  there's no more active client */
        XDeleteProperty(display, root, atoms[AtomNetActiveWindow]);
    }
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
                && !(c->states & NetWMStateMaximized)) {
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
    if (!c)
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
        for (Monitor *m = monitors; m; m = m->next) {
            if (e->x_root > m->x && e->x_root < m->x + m->w
                    && e->y_root > m->y && e->y_root < m->y + m->h) {
                if (activeMonitor != m) {
                    SetActiveMonitor(m);
                    SetActiveClient(NULL);
                }
                break;
            }
        }
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
        if (e->window == c->topbar || (e->window == c->window && e->state == Modkey)) {
            int delay = e->time - lastClickPointerTime;
            if (delay > 150 && delay < 250) {
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
            RestackMonitor(c->monitor);
        }
    } else {
        /* we do not apply normal hints during motion but when button is released
         * to make the resizing visually smoother. Some client apply normals by
         * themselves anway (e.g gnome-terminal) */
        if (e->window == c->topbar || e->window == c->window
                || moveMessageType == HandleCount)
            MoveClientFrame(c, motionStartX + vx, motionStartY + vy);
        else if (e->window == c->handles[HandleNorth]
                || moveMessageType == HandleNorth)
            MoveResizeClientFrame(c, motionStartX, motionStartY + vy,
                    motionStartW, motionStartH - vy , False);
        else if (e->window == c->handles[HandleWest]
                || moveMessageType == HandleWest)
            ResizeClientFrame(c, motionStartW + vx, motionStartH, False);
        else if (e->window == c->handles[HandleSouth]
                || moveMessageType == HandleSouth)
            ResizeClientFrame(c, motionStartW, motionStartH + vy, False);
        else if (e->window == c->handles[HandleEast]
                || moveMessageType == HandleEast)
            MoveResizeClientFrame(c, motionStartX + vx, motionStartY,
                    motionStartW - vx, motionStartH, False);
        else if (e->window == c->handles[HandleNorthEast]
                || moveMessageType == HandleNorthEast)
            MoveResizeClientFrame(c, motionStartX + vx, motionStartY + vy,
                    motionStartW - vx, motionStartH - vy, False);
        else if (e->window == c->handles[HandleNorthWest]
                || moveMessageType == HandleNorthWest)
            MoveResizeClientFrame(c, motionStartX, motionStartY + vy,
                    motionStartW + vx, motionStartH - vy, False);
        else if (e->window == c->handles[HandleSouthWest]
                || moveMessageType == HandleSouthWest)
            ResizeClientFrame(c, motionStartW + vx, motionStartH + vy, False);
        else if (e->window == c->handles[HandleSouthEast]
                || moveMessageType == HandleSouthEast)
            MoveResizeClientFrame(c, motionStartX + vx, motionStartY,
                    motionStartW - vx, motionStartH + vy, False);
        else
          return;
    }
}

void
OnMessage(XClientMessageEvent *e)
{
    if (e->window == root) {
        if (e->message_type == atoms[AtomNetCurrentDesktop])
            ShowDesktop(activeMonitor, e->data.l[0]);

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

    //if (e->message_type == atoms[AtomNetMoveresizeWindow]) {
    //    MoveResizeClientWindow(c, e->data.l[1], e->data.l[2],
    //            e->data.l[3], e->data.l[4], True);
    //}

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

void OnLeave(XCrossingEvent *e)
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
        if (keysym == (settings.shortcuts[i].keysym) &&
                CleanMask(settings.shortcuts[i].modifier) == CleanMask(e->state)) {
            if (settings.shortcuts[i].type == CV)
                settings.shortcuts[i].cb.vcb.f();
            if (settings.shortcuts[i].type == CC && activeClient)
                settings.shortcuts[i].cb.ccb.f(activeClient);
            if (settings.shortcuts[i].type == CCI && activeClient)
                settings.shortcuts[i].cb.cicb.f(activeClient,
                        settings.shortcuts[i].cb.cicb.i);
            if (settings.shortcuts[i].type == CM && activeMonitor)
                settings.shortcuts[i].cb.mcb.f(activeMonitor);
            if (settings.shortcuts[i].type == CMI && activeMonitor)
                settings.shortcuts[i].cb.micb.f(activeMonitor,
                        settings.shortcuts[i].cb.micb.i);
        }
    }
}

void
OnKeyRelease(XKeyReleasedEvent *e)
{
    KeySym keysym;

    keysym = XkbKeycodeToKeysym(display, e->keycode, 0, 0);
    if (keysym == (ModkeySym) && activeClient && switching) {
        StackClientBefore(activeClient, activeClient->monitor->head);
        if (activeClient->isTiled)
            RestackMonitor(activeClient->monitor);
        switching = False;
    }
}

