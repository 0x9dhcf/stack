#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "client.h"
#include "config.h"
#include "log.h"
#include "monitor.h"
#include "stack.h"
#include "utils.h"
#include "x11.h"

#define RootEventMask (SubstructureRedirectMask | SubstructureNotifyMask)

#define FrameEvenMask (\
          ExposureMask\
        | ButtonPressMask\
        | EnterWindowMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define WindowEventMask (PropertyChangeMask)

#define WindowDoNotPropagateMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask\
        | KeyPressMask\
        | KeyReleaseMask)

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
    (mask & ~(stNumLockMask|LockMask) &\
         ( ShiftMask\
         | ControlMask\
         | Mod1Mask\
         | Mod2Mask\
         | Mod3Mask\
         | Mod4Mask\
         | Mod5Mask))

static char    *terminal[] = {"uxterm", NULL};
static int      lastSeenPointerX;
static int      lastSeenPointerY;
static Time     lastSeenPointerTime;
static int      motionStartX;
static int      motionStartY;
static int      motionStartW;
static int      motionStartH;
static Monitor *stActiveMonitor;
static Client  *stActiveClient;
static Bool     stRunning;

static int      WMDetectedErrorHandler(Display *d, XErrorEvent *e);
static int      EventLoopErrorHandler(Display *d, XErrorEvent *e);
static void     ManageWindow(Window w, Bool exists);
static Client  *Lookup(Window w);
static void     ForgetWindow(Window w, Bool survives);
static void     SetActiveClient(Client *c);
static void     FindNextActiveClient();
static void     OnConfigureRequest(XConfigureRequestEvent *e);
static void     OnMapRequest(XMapRequestEvent *e);
static void     OnUnmapNotify(XUnmapEvent *e);
static void     OnDestroyNotify(XDestroyWindowEvent *e);
static void     OnExpose(XExposeEvent *e);
static void     OnEnter(XCrossingEvent *e);
static void     OnLeave(XCrossingEvent *e);
static void     OnPropertyNotify(XPropertyEvent *e);
static void     OnButtonPress(XButtonEvent *e);
static void     OnButtonRelease(XButtonEvent *e);
static void     OnMotionNotify(XMotionEvent *e);
static void     OnMessage(XClientMessageEvent *e);
static void     OnKeyPress(XKeyPressedEvent *e);
static void     Spawn(char **args);

static XErrorHandler defaultErrorHandler;
static Window supportingWindow;
static Client *lastActiveClient = NULL;

int
WMDetectedErrorHandler(Display *d, XErrorEvent *e)
{
    (void)d;
    (void)e;
    FLog("A windows manager is already running!");
    return 1; /* never reached */
}

int
EventLoopErrorHandler(Display *d, XErrorEvent *e)
{
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
        return 0;
    }
    ELog("error: request code=%d, error code=%d",
            e->request_code, e->error_code);
    return defaultErrorHandler(d, e); /* may call exit */
}

void
Start()
{
    int xConnection;
    fd_set fdSet;

    XSetWindowAttributes wa;
    Window *wins,  w0, w1, rwin, cwin;
    unsigned int nwins, mask;
    int rx, ry, wx, wy;
    KeyCode code;
    unsigned int modifiers[] = { 0, LockMask, stNumLockMask, stNumLockMask | LockMask };

    /* check for existing wm */
    XErrorHandler xerror = XSetErrorHandler(WMDetectedErrorHandler);
    XSelectInput(stDisplay, stRoot, SubstructureRedirectMask);
    XSync(stDisplay, False);
    XSetErrorHandler(xerror);
    XSync(stDisplay, False);

    /* Setup */
    if (! stMonitors)
        FLog("no monitor detected.");

    stActiveMonitor = stMonitors;

    defaultErrorHandler = XSetErrorHandler(EventLoopErrorHandler);

    supportingWindow = XCreateSimpleWindow(stDisplay, stRoot,
            0, 0, 1, 1, 0, 0, 0);

    XChangeProperty(stDisplay, supportingWindow,
            stAtoms[AtomNetSupportingWMCheck], XA_WINDOW, 32,
            PropModeReplace, (unsigned char *) &supportingWindow, 1);
    XChangeProperty(stDisplay, supportingWindow, XA_WM_NAME, XA_STRING, 8,
            PropModeReplace, (unsigned char*)"stack", 5);
    XChangeProperty(stDisplay, supportingWindow, stAtoms[AtomNetWMPID],
            XA_INTEGER, 32, PropModeReplace,
            (unsigned char *) &supportingWindow, 1);

    /* Configure the root window */
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetSupportingWMCheck],
            XA_WINDOW, 32, PropModeReplace,
            (unsigned char *) &supportingWindow, 1);
    XChangeProperty(stDisplay, stRoot, XA_WM_NAME, XA_STRING, 8,
            PropModeReplace, (unsigned char*)"stack", 5);
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetWMPID], XA_INTEGER, 32,
            PropModeReplace, (unsigned char *) &supportingWindow, 1);
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetSupported], XA_ATOM, 32,
        PropModeReplace, (unsigned char *) stAtoms, AtomCount);

    wa.cursor = stCursors[CursorNormal];
    wa.event_mask = RootEventMask;
    XChangeWindowAttributes(stDisplay, stRoot, CWEventMask | CWCursor, &wa);

    /* Let ewmh listeners know about what is supported. */
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetSupported], XA_ATOM, 32,
            PropModeReplace, (unsigned char *) stAtoms , AtomCount);

    /* inform about the number of desktops */
    int desktops = DesktopCount;
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetNumberOfDesktops], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char *)&desktops, 1);

    /* Reset the client list. */
    XDeleteProperty(stDisplay, stRoot, stAtoms[AtomNetClientList]);

    /* manage exiting windows */
    XQueryPointer(stDisplay, stRoot, &rwin, &cwin, &rx, &ry, &wx, &wy, &mask);

    /* to avoid unmap generated by reparenting to destroy the newly created client */
    if (XQueryTree(stDisplay, stRoot, &w0, &w1, &wins, &nwins)) {
        for (unsigned int i = 0; i < nwins; ++i) {
            XWindowAttributes xwa;

            if (wins[i] == supportingWindow)
                continue;

            XGetWindowAttributes(stDisplay, wins[i], &xwa);
            if (!xwa.override_redirect) {
                ManageWindow(wins[i], True);
                /* reparenting does not set the e->event to root
                 * we discar events to prevent the unmap to be catched
                 * and wrongly interpreted */
                XSync(stDisplay, True);
                XSetInputFocus(stDisplay, wins[i], RevertToPointerRoot,
                        CurrentTime);
                XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetClientList],
                        XA_WINDOW, 32, PropModeAppend,
                        (unsigned char *) &wins[i], 1);
            }
        }
        XFree(wins);
    }

    /* grab shortcuts */
    XUngrabKey(stDisplay, AnyKey, AnyModifier, stRoot);

    if ((code = XKeysymToKeycode(stDisplay, XK_Return)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(stDisplay, code, Modkey | ShiftMask | modifiers[j],
                    stRoot, True, GrabModeSync, GrabModeAsync);
    if ((code = XKeysymToKeycode(stDisplay, XK_p)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(stDisplay, code, Modkey | modifiers[j],
                    stRoot, True, GrabModeSync, GrabModeAsync);

    for (int i = 0; i < ShortcutCount; ++i) {
        Shortcut sc = stConfig.shortcuts[i];
        if ((code = XKeysymToKeycode(stDisplay, sc.keysym))) {
            for (int j = 0; j < 4; ++j)
                XGrabKey(stDisplay, code,
                        sc.modifier | modifiers[j],
                        stRoot, True, GrabModeSync, GrabModeAsync);
        }
    }

    /* Main loop */
    XSync(stDisplay, False);

    xConnection = XConnectionNumber(stDisplay);
    stRunning = True;
    while (stRunning) {
        struct timeval timeout = { 0, 2500 };
        FD_ZERO(&fdSet);
        FD_SET(xConnection, &fdSet);

        if (select(xConnection + 1, &fdSet, NULL, NULL, &timeout) > 0) {
            while (XPending(stDisplay)) {
                XEvent e;
                XNextEvent(stDisplay, &e);

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
                }

                XFlush(stDisplay);
            }
        }
    }

    /* teardown */
    for (Monitor *m = stMonitors; m; m = m->next) {
        Client *c, *d;
        for (c = m->chead, d = c ? c->next : 0; c; c = d, d = c ? c->next : 0)
            ForgetWindow(c->window, True);
    }

    XDestroyWindow(stDisplay, supportingWindow);
    XSetInputFocus(stDisplay, PointerRoot, RevertToPointerRoot, CurrentTime);
    XSetErrorHandler(defaultErrorHandler);
    XUngrabKey(stDisplay, AnyKey, AnyModifier, stRoot);
    XSelectInput(stDisplay, stRoot, NoEventMask);
}

void Stop()
{
    stRunning = False;
}

void
ManageWindow(Window w, Bool exists)
{
    if (Lookup(w))
        return;

    Client *c = malloc(sizeof(Client));
    if (!c) {
        ELog("can't allocate client.");
        return;
    }
    memset(c, 0, sizeof(Client));

    DLog("manage: %ld", w);
    XAddToSaveSet(stDisplay, w);

    /* get info about the window */
    Window r = None, t = None;
    int wx = 0, wy = 0 ;
    unsigned int ww, wh, d, b;

    XGetGeometry(stDisplay, w, &r, &wx, &wy, &ww, &wh, &b, &d);
    XGetTransientForHint(stDisplay, w, &t);
    GetWMProtocols(w, &c->protocols);
    GetNetWMWindowType(w, &c->types);
    GetWMHints(w, &c->hints);
    GetNetWMStates(w, &c->states);
    GetWMNormals(w, &c->normals);
    GetWMName(w, &c->name);
    GetWMClass(w, &c->wmclass);
    GetWMStrut(w, &c->strut);

    c->active = False;
    c->decorated = !(c->types & NetWMTypeFixed);
    c->tiled = False;
    c->desktop = stActiveMonitor->activeDesktop;

    /* if transient for, register the client we are transient for
     * and regiter this new client as a transient of this
     * transient for client */
    if (t != None) {
        Transient *tc = malloc(sizeof(Transient));
        if (tc) {
            c->transfor = Lookup(t);
            if (c->transfor) {
                /* attache tc to the transient for client */
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
    c->frame = XCreateWindow(stDisplay, stRoot, 0, 0, 1, 1, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWEventMask | CWBackingStore, &fattrs);
    XMapWindow(stDisplay, c->frame);

    /* client */
    c->window = w;
    XSetWindowAttributes cattrs = {0};
    cattrs.event_mask = WindowEventMask;
    cattrs.do_not_propagate_mask = WindowDoNotPropagateMask;
    XChangeWindowAttributes(stDisplay, w, CWEventMask | CWDontPropagate, &cattrs);
    XSetWindowBorderWidth(stDisplay, w, 0);
    XReparentWindow(stDisplay, w, c->frame, 0, 0);
    if (c->hints & HintsFocusable && !(c->types & NetWMTypeFixed)) {
        XUngrabButton(stDisplay, AnyButton, AnyModifier, c->window);
        XGrabButton(stDisplay, Button1, AnyModifier, c->window, False,
                ButtonPressMask | ButtonReleaseMask, GrabModeSync,
                GrabModeSync, None, None);
    }
    if (!exists)
        XMapWindow(stDisplay, w);

    /* windows with EWMH type fixed are neither moveable,
     * resizable nor decorated.  While windows fixed by ICCCM normals 
     * are decorated and moveable but not resizable) */
    if (!(c->types & NetWMTypeFixed)) {
        /* topbar */
        XSetWindowAttributes tattrs = {0};
        tattrs.event_mask = HandleEventMask;
        tattrs.cursor = stCursors[CursorNormal];
        c->topbar = XCreateWindow(stDisplay, c->frame, 0, 0, 1, 1, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWEventMask | CWCursor, &tattrs);
        XMapWindow(stDisplay, c->topbar);

        /* buttons */
        XSetWindowAttributes battrs = {0};
        battrs.event_mask = ButtonEventMask;
        for (int i = 0; i < ButtonCount; ++i) {
            Window b = XCreateWindow(stDisplay, c->topbar, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOutput, CopyFromParent,
                    CWEventMask | CWCursor, &battrs);
            XMapWindow(stDisplay, b);
            c->buttons[i] = b;
        }
    }

    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals)) {
        /* handles */
        XSetWindowAttributes hattrs = {0};
        hattrs.event_mask = HandleEventMask;
        for (int i = 0; i < HandleCount; ++i) {
            hattrs.cursor = stCursors[CursorResizeNorth + i];
            Window h = XCreateWindow(stDisplay, stRoot, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOnly, CopyFromParent,
                    CWEventMask | CWCursor, &hattrs);
            XMapWindow(stDisplay, h);
            c->handles[i] = h;
        }
    }

    /* we need a first Move resize to sync the geometries before attaching */
    c->sbw = b;
    MoveResizeClientWindow(c, wx, wy, ww, wh, False);
    AttachClientToMonitor(stActiveMonitor, c);

    /* we can only apply structure states once attached in floating mode */
    if (! stActiveMonitor->desktops[stActiveMonitor->activeDesktop].dynamic) {
        if (c->states & NetWMStateMaximizedHorz)
            MaximizeClientHorizontally(c);
        if (c->states & NetWMStateMaximizedVert)
            MaximizeClientVertically(c);
        if (c->states & NetWMStateFullscreen)
            FullscreenClient(c);
    }

    if (!(c->types & NetWMTypeFixed))
        SetActiveClient(c);

    /* let anyone interrested in ewmh knows what we honor */
    SetNetWMAllowedActions(w, NetWMActionDefault);

    /* update the client list */
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetClientList], XA_WINDOW,
            32, PropModeAppend, (unsigned char *) &(w), 1);
}

void
ForgetWindow(Window w, Bool survives)
{
    Client *c = Lookup(w);
    if (c) {
        DetachClientFromMonitor(c->monitor, c);

        /* if transient for someone unregister this client */
        if (c->transfor) {
            Transient **tc;
            for (tc = &c->transfor->transients;
                    *tc && (*tc)->client != c; tc = &(*tc)->next);
            *tc = (*tc)->next;
        }

        if (c == lastActiveClient)
            lastActiveClient = NULL;

        if (c == stActiveClient) {
            stActiveClient = NULL;
            FindNextActiveClient();
        }

        if (c->name)
            free(c->name);

        if (c->wmclass.cname)
            free(c->wmclass.cname);

        if (c->wmclass.iname)
            free(c->wmclass.iname);

        if (survives) {
            XReparentWindow(stDisplay, c->window, stRoot, c->wx, c->wy);
            XSetWindowBorderWidth(stDisplay, c->window, c->sbw);
        }

        if (!(c->types & NetWMTypeFixed)) {
            for (int i = 0; i < ButtonCount; ++i)
                XDestroyWindow(stDisplay, c->buttons[i]);
            XDestroyWindow(stDisplay, c->topbar);
        }

        if (!(c->types & NetWMTypeFixed) 
                && (c->normals.minw != c->normals.maxw
                    && c->normals.minh != c->normals.maxh)) {
            for (int i = 0; i < HandleCount; ++i)
                XDestroyWindow(stDisplay, c->handles[i]);
        }

        XDestroyWindow(stDisplay, c->frame);

        if (survives)
            XRemoveFromSaveSet(stDisplay, c->window);

        free(c);

        /* update the client list */
        XDeleteProperty(stDisplay, stRoot, stAtoms[AtomNetClientList]);
        for (Monitor *m = stMonitors; m; m = m->next)
            for (Client *c2 = m->chead; c2; c2 = c2->next)
                XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetClientList],
                        XA_WINDOW, 32, PropModeAppend,
                        (unsigned char *) &c2->window, 1);
    }
}

Client *
Lookup(Window w)
{
    for (Monitor *m = stMonitors; m; m = m->next) {
        for (Client *c = m->chead; c; c = c->next) {
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
SetActiveClient(Client *c)
{
    if (c) {
        if (stActiveClient && stActiveClient != c) {
            lastActiveClient = stActiveClient;
            SetClientActive(stActiveClient, False);
            LowerClient(c);
            SetNetWMStates(c->window, c->states);
        }

        SetClientActive(c, True);
        stActiveClient = c;
        RaiseClient(stActiveClient);
        SetNetWMStates(stActiveClient->window, stActiveClient->states);
        c->monitor->desktops[c->desktop].activeOnLeave = c;
    }
}

void
FindNextActiveClient()
{
    Client *head = NULL;
    Client *nc = NULL;

    if (stActiveClient)
        head = stActiveClient;
    else 
        head = stActiveMonitor->chead;

    /* use the last active client if valid */
    if (lastActiveClient 
            && lastActiveClient->desktop == stActiveMonitor->activeDesktop
            && !(lastActiveClient->states & NetWMStateHidden)) {
        nc = lastActiveClient;
    } else if (head) { /* try to find a new one */
        for (nc = NextClient(stActiveMonitor, head);
                nc != head
                    && (nc->desktop != stActiveMonitor->activeDesktop
                    || !(nc->types & NetWMTypeNormal)
                    || nc->states & NetWMStateHidden);
                nc = NextClient(stActiveMonitor, nc));
    }

    if (nc && nc->desktop == stActiveMonitor->activeDesktop
            && nc->types & NetWMTypeNormal
            && !(nc->states & NetWMStateHidden)) {
        SetActiveClient(nc);
    } else if (stActiveClient) {
        stActiveMonitor->desktops[stActiveMonitor->activeDesktop].activeOnLeave = NULL;
        SetClientActive(stActiveClient, False);
        stActiveClient = NULL;
    }
}

void
ActivateNext()
{
    Client *head = stActiveClient ? stActiveClient : stActiveMonitor->chead;
    Client *nc = NULL;
    if (head) {
        for (nc = NextClient(stActiveMonitor, head);
                nc && nc != head
                    && (nc->desktop != stActiveMonitor->activeDesktop
                    || !(nc->types & NetWMTypeNormal));
                nc = NextClient(stActiveMonitor, nc));

        if (nc)
            SetActiveClient(nc);
    }
}

void
ActivatePrev()
{
    Client *tail = stActiveClient ? stActiveClient : stActiveMonitor->ctail;
    Client *pc = NULL;
    if (tail) {
        for (pc = PreviousClient(stActiveMonitor, tail);
                pc && pc != tail
                    && (pc->desktop != stActiveMonitor->activeDesktop
                    || !(pc->types & NetWMTypeNormal));
                pc = PreviousClient(stActiveMonitor, pc));

        if (pc)
            SetActiveClient(pc);
    }
}

void
MoveForward()
{
    if (stActiveMonitor
            && stActiveMonitor->desktops[stActiveMonitor->activeDesktop].dynamic) {

        Client *after = NULL;
        for (after = stActiveClient->next;
                        after && (after->desktop != stActiveClient->desktop
                        || !(after->types & NetWMTypeNormal)
                        ||  after->states & NetWMStateHidden);
                after = after->next);

        if (after) {
            MoveClientAfter(stActiveMonitor, stActiveClient, after);
            Restack(stActiveMonitor);
            return;
        }

        Client *before = NULL;
        for (before = stActiveClient->monitor->chead;
                        before && (before->desktop != stActiveClient->desktop
                        || !(before->types & NetWMTypeNormal)
                        ||  before->states & NetWMStateHidden);
                before = before->next);

        if (before) {
            MoveClientBefore(stActiveMonitor, stActiveClient, before);
            Restack(stActiveMonitor);
            return;
        }
    }
}

void
MoveBackward()
{
    if (stActiveMonitor
            && stActiveMonitor->desktops[stActiveMonitor->activeDesktop].dynamic) {

        Client *before = NULL;
        for (before = stActiveClient->prev;
                        before && (before->desktop != stActiveClient->desktop
                        || !(before->types & NetWMTypeNormal)
                        ||  before->states & NetWMStateHidden);
                before = before->prev);

        if (before) {
            MoveClientBefore(stActiveMonitor, stActiveClient, before);
            Restack(stActiveMonitor);
            return;
        }

        Client *after = NULL;
        for (after = stActiveClient->monitor->ctail;
                        after && (after->desktop != stActiveClient->desktop
                        || !(after->types & NetWMTypeNormal)
                        ||  after->states & NetWMStateHidden);
                after = after->prev);

        if (after) {
            MoveClientAfter(stActiveMonitor, stActiveClient, after);
            Restack(stActiveMonitor);
            return;
        }
    }
}

void
ShowDesktop(int desktop)
{
    /* save the active client if any */
    if (stActiveClient && stActiveClient->desktop == stActiveMonitor->activeDesktop)
        stActiveMonitor->desktops[stActiveMonitor->activeDesktop].activeOnLeave = stActiveClient;
    else
        stActiveMonitor->desktops[stActiveMonitor->activeDesktop].activeOnLeave = NULL;;

    SetActiveDesktop(stActiveMonitor, desktop);

    /* Restore the active client if any */
    if (stActiveMonitor->desktops[stActiveMonitor->activeDesktop].activeOnLeave)
        SetActiveClient(stActiveMonitor->desktops[stActiveMonitor->activeDesktop].activeOnLeave);
    else 
        FindNextActiveClient();

    Restack(stActiveMonitor);
}

void
MoveToDesktop(int desktop)
{
    if (stActiveClient) {
        Client *toMove = stActiveClient;
        RemoveClientFromDesktop(toMove->monitor, toMove, toMove->desktop);
        FindNextActiveClient();
        AddClientToDesktop(toMove->monitor, toMove, desktop);
        Restack(toMove->monitor);
    }
}

void
ToggleDynamic()
{
    if (stActiveMonitor) {
        Bool current = stActiveMonitor->desktops[stActiveMonitor->activeDesktop].dynamic;
        stActiveMonitor->desktops[stActiveMonitor->activeDesktop].dynamic = !current;
        /* Untile, restore all windows */
        if (current) {
            for (Client *c = stActiveMonitor->chead; c; c = c->next) {
                if (c->desktop == stActiveMonitor->activeDesktop) {
                    RestoreClient(c);
                }
            }
        } else {
            Restack(stActiveMonitor);
        }
    }
}

void
AddMaster(int nb)
{
    if (stActiveMonitor
            && stActiveMonitor->desktops[stActiveMonitor->activeDesktop].dynamic) {
        stActiveMonitor->desktops[stActiveMonitor->activeDesktop].masters = 
            Max(stActiveMonitor->desktops[stActiveMonitor->activeDesktop].masters + nb, 1);
        Restack(stActiveMonitor);
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
            SetNetWMStates(c->window, c->states);
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
    /* TODO: root active window and Curent desktop 
     * Especially since we have advertised supported actions
     * _NET_CLOSE_WINDOW
     * _NET_MOVERESIZE_WINDOW 
     * _NET_WM_MOVERESIZE
     * _NET_RESTACK_WINDOW */

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

