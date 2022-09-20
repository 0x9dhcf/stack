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
#include "config.h"
#include "event.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "x11.h"

#define RootEventMask (SubstructureRedirectMask | SubstructureNotifyMask)

#define FrameEvenMask (\
          ButtonPressMask\
        | EnterWindowMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define WindowEventMask (PropertyChangeMask)

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)

static Window supportingWindow;
static Client *lastActiveClient = NULL;
static int WMDetectedErrorHandler(Display *d, XErrorEvent *e);

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
        if ((code = XKeysymToKeycode(display, config.shortcuts[i].keysym))) {
            for (int j = 0; j < 4; ++j)
                XGrabKey(display, code,
                        config.shortcuts[i].modifier | modifiers[j],
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
    c->hasTopbar = decorated;
    c->hasHandles = !((c->types & NetWMTypeFixed) && !IsFixed(c->normals));
    c->active = False;
    c->tiled = False;
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
    XMapWindow(display, c->frame);

    /* client */
    c->window = w;
    XSetWindowAttributes cattrs = {0};
    cattrs.event_mask = NoEventMask;
    cattrs.do_not_propagate_mask = NoEventMask;
    XChangeWindowAttributes(display, w, CWEventMask | CWDontPropagate, &cattrs);
    XSetWindowBorderWidth(display, w, 0);
    XReparentWindow(display, w, c->frame, 0, 0);
    if (c->hints & HintsFocusable && !(c->types & NetWMTypeFixed)) {
        XUngrabButton(display, AnyButton, AnyModifier, c->window);
        XGrabButton(display, Button1, AnyModifier, c->window, False,
                ButtonPressMask | ButtonReleaseMask, GrabModeSync,
                GrabModeSync, None, None);
    }

    if (! mapped)
        XMapWindow(display, w);

    /*
     * windows with EWMH type fixed are neither moveable,
     * resizable nor decorated. While windows fixed by ICCCM normals
     * are decorated and moveable but not resizable)
     */
    if (!(c->types & NetWMTypeFixed)) {
        long state[] = {NormalState, None};

        /* topbar */
        XSetWindowAttributes tattrs = {0};
        tattrs.event_mask = HandleEventMask;
        tattrs.cursor = cursors[CursorNormal];
        c->topbar = XCreateWindow(display, c->frame, 0, 0, 1, 1, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWEventMask | CWCursor, &tattrs);
        XMapWindow(display, c->topbar);

        // XXX: WHY HERE!!!
        XChangeProperty(display, w, atoms[AtomWMState],
                atoms[AtomWMState], 32, PropModeReplace,
                (unsigned char *)state, 2);

        /* buttons */
        XSetWindowAttributes battrs = {0};
        battrs.event_mask = ButtonEventMask;
        for (int i = 0; i < ButtonCount; ++i) {
            Window b = XCreateWindow(display, c->topbar, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOutput, CopyFromParent,
                    CWEventMask | CWCursor, &battrs);
            c->buttons[i] = b;
            XMapWindow(display, b);
        }
    }

    if (!(c->types & NetWMTypeFixed) && !IsFixed(c->normals)) {
        /* handles */
        XSetWindowAttributes hattrs = {0};
        hattrs.event_mask = HandleEventMask;
        for (int i = 0; i < HandleCount; ++i) {
            hattrs.cursor = cursors[CursorResizeNorth + i];
            Window h = XCreateWindow(display, root, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOnly, CopyFromParent,
                    CWEventMask | CWCursor, &hattrs);
            c->handles[i] = h;
            XMapWindow(display, h);
        }
    }

    c->sbw = b;
    /* we need a first Move resize to sync the geometries before attaching */
    /* make sure we're in the working area */
    Desktop *dp = &activeMonitor->desktops[activeMonitor->activeDesktop];
    c->ww = Min(dp->ww, (int)ww);
    c->wh = Min(dp->wh, (int)wh);
    c->wx = Min(Max(dp->wx, wx), dp->wx + dp->ww - (int)ww);
    c->wy = Min(Max(dp->wy, wy), dp->wy + dp->wh - (int)wh);
    c->wx = (dp->wx + dp->ww - (int)ww) / 2;
    c->wy = (dp->wy + dp->wh - (int)wh) / 2;
    SynchronizeFrameGeometry(c);
    AttachClientToMonitor(activeMonitor, c);

    /* we can only apply structure states once attached in floating mode */
    if (! activeMonitor->desktops[activeMonitor->activeDesktop].dynamic) {
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

    /* if transient for someone unregister this client */
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
            m->desktops[i].masters = config.masters;
            m->desktops[i].split = config.split;
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
StackActiveClientDown()
{
    if (activeClient && activeClient->transfor)
        return;

    if (activeMonitor->desktops[activeMonitor->activeDesktop].dynamic) {

        Client *after = NULL;
        for (after = activeClient->next;
                        after && (after->desktop != activeClient->desktop
                        || !(after->types & NetWMTypeNormal)
                        ||  after->states & NetWMStateHidden);
                after = after->next);

        if (after) {
            StackClientAfter(activeClient, after);
            RestackMonitor(activeMonitor);
            return;
        }

        Client *before = NULL;
        for (before = activeClient->monitor->head;
                        before && (before->desktop != activeClient->desktop
                        || !(before->types & NetWMTypeNormal)
                        ||  before->states & NetWMStateHidden);
                before = before->next);

        if (before) {
            StackClientBefore(activeClient, before);
            RestackMonitor(activeMonitor);
        }
    }
}

void
StackActiveClientUp()
{
    if (activeClient && activeClient->transfor)
        return;

    if (activeMonitor->desktops[activeMonitor->activeDesktop].dynamic) {

        Client *before = NULL;
        for (before = activeClient->prev;
                        before && (before->desktop != activeClient->desktop
                        || !(before->types & NetWMTypeNormal)
                        ||  before->states & NetWMStateHidden);
                before = before->prev);

        if (before) {
            StackClientBefore(activeClient, before);
            RestackMonitor(activeMonitor);
            return;
        }

        Client *after = NULL;
        for (after = activeClient->monitor->tail;
                        after && (after->desktop != activeClient->desktop
                        || !(after->types & NetWMTypeNormal)
                        ||  after->states & NetWMStateHidden);
                after = after->prev);

        if (after) {
            StackClientAfter(activeClient, after);
            RestackMonitor(activeMonitor);
        }
    }
}

void
ShowDesktop(int desktop)
{
    activeMonitor->desktops[activeMonitor->activeDesktop].activeOnLeave = activeClient;
    SetActiveDesktop(activeMonitor, desktop);
    SetActiveClient(activeMonitor->desktops[activeMonitor->activeDesktop].activeOnLeave);
    RestackMonitor(activeMonitor);
}

void
MoveActiveClientToDesktop(int desktop)
{
    if (activeClient) {
        AssignClientToDesktop(activeClient, desktop);
        SetActiveClient(NULL);
        activeMonitor->desktops[activeMonitor->activeDesktop].activeOnLeave = activeClient;
        RestackMonitor(activeMonitor);
    }
}

void
ToggleDynamicForActiveDesktop()
{
    Bool b = activeMonitor->desktops[activeMonitor->activeDesktop].dynamic;
    activeMonitor->desktops[activeMonitor->activeDesktop].dynamic = ! b;
    /* Untile, restore all windows */
    if (b)
        for (Client *c = activeMonitor->head; c; c = c->next)
            if (c->desktop == activeMonitor->activeDesktop)
                UntileClient(c);
    RestackMonitor(activeMonitor);
}

void
ToggleTopbarForActiveDesktop()
{
    Bool b = activeMonitor->desktops[activeMonitor->activeDesktop].toolbar;
    activeMonitor->desktops[activeMonitor->activeDesktop].toolbar = ! b;
    for (Client *c = activeMonitor->head; c; c = c->next) {
        if (c->desktop == activeMonitor->activeDesktop)
            SetClientTopbarVisible(c, !b);
        RefreshClient(c);
    }
    RestackMonitor(activeMonitor);
}

void
AddMasterToActiveDesktop(int nb)
{
    if (activeMonitor->desktops[activeMonitor->activeDesktop].dynamic) {
        activeMonitor->desktops[activeMonitor->activeDesktop].masters =
            Max(activeMonitor->desktops[activeMonitor->activeDesktop].masters + nb, 1);
        RestackMonitor(activeMonitor);
    }
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
            Client *head = activeClient ? activeClient : activeMonitor->head;
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
        activeClient->states |= NetWMStateBelow;
        activeClient->states &= ~NetWMStateAbove;
        SetNetWMStates(activeClient->window, activeClient->states);
        activeClient = NULL;
    }

    if (toActivate && toActivate->desktop == activeMonitor->activeDesktop
            && toActivate->types & NetWMTypeNormal
            && !(toActivate->states & NetWMStateHidden)) {
        /* if someone is to be activated do it */
        SetClientActive(toActivate, True);
        activeClient = toActivate;
        RaiseClient(activeClient);
        XChangeProperty(display, root, atoms[AtomNetActiveWindow],
                XA_WINDOW, 32, PropModeReplace,
                (unsigned char *) &toActivate->window, 1);
    } else {
        /* otherwise let anybody know  there's no more active client */
        XDeleteProperty(display, root, atoms[AtomNetActiveWindow]);
    }
}
