#include <X11/Xlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "X11/X.h"
#include "client.h"
#include "event.h"
#include "hints.h"
#include "log.h"
#include "macros.h"
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
        | PropertyChangeMask\
        | ButtonPressMask\
        | EnterWindowMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define WindowEvenMask (\
        PropertyChangeMask)

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)

static void AttachClient(Client *c);
static void DetachClient(Client *c);

static Window supportingWindow;
static Client *lastActiveClient = NULL;

Monitor *activeMonitor = NULL;
Client *activeClient = NULL;

void
SetupWindowManager()
{
    XSetWindowAttributes wa;
    XErrorHandler h;
    Window *wins,  w0, w1, rwin, cwin;
    KeyCode code;
    unsigned int nwins, mask;
    int rx, ry, wx, wy;
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
            (unsigned char *)&activeMonitor->activeDesktop, 1);

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
    //if ((code = XKeysymToKeycode(display, XK_Return)))
    //    for (int j = 0; j < 4; ++j)
    //        XGrabKey(display, code, ModCtrlShift | modifiers[j],
    //                root, True, GrabModeAsync, GrabModeAsync);

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
    Client *c, *d;
    for (c = clients, d = c ? c->next : 0; c; c = d, d = c ? c->next : 0)
        UnmanageWindow(c->window, False);

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

    Desktop *dp = &activeMonitor->desktops[activeMonitor->activeDesktop];
    c->ww = Min(dp->ww, (int)ww);
    c->wh = Min(dp->wh, (int)wh);
    c->wx = Min(Max(dp->wx, wx), dp->wx + dp->ww - (int)ww);
    c->wy = Min(Max(dp->wy, wy), dp->wy + dp->wh - (int)wh);
    c->sbw = b;
    decorated = (c->types & NetWMTypeFixed
            || (c->motifs.flags == 0x2 && c->motifs.decorations == 0))
        ? False : True;
    c->isBorderVisible = decorated;
    c->isTopbarVisible = decorated;
    c->hasTopbar = decorated && !(c->types & NetWMTypeNoTopbar);
    c->hasHandles = decorated && !IsFixed(c->normals);
    c->isFocused = False;
    c->isTiled = False;
    c->isVisible = False;
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
    XSetWindowAttributes wattrs = {0};
    wattrs.event_mask = WindowEvenMask;
    XChangeWindowAttributes(display, w, CWEventMask, &wattrs);
    if (c->hints & HintsFocusable && !(c->types & NetWMTypeFixed)) {
        /* allows activation on click */
        XUngrabButton(display, AnyButton, AnyModifier, c->window);
        XGrabButton(display, Button1, AnyModifier, c->window, False,
                ButtonPressMask, GrabModeSync,
                GrabModeSync, None, None);
    }

    /* Windows with EWMH type fixed are neither moveable,
     * resizable nor decorated. While windows fixed by ICCCM normals
     * are decorated and moveable but not resizable */
    if (c->hasTopbar) {
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

    /* handles */
    if (c->hasHandles) {
        XSetWindowAttributes hattrs = {0};
        hattrs.event_mask = HandleEventMask;
        for (int i = 0; i < HandleCount; ++i) {
            hattrs.cursor = cursors[CursorResizeNorthEast + i];
            c->handles[i] = XCreateWindow(display, root, 0, 0, 1, 1, 0,
                    CopyFromParent, InputOnly, CopyFromParent,
                    CWEventMask | CWCursor, &hattrs);
        }
    }

    /* attach */
    AttachClient(c);
    SynchronizeFrameGeometry(c);
    AttachClientToMonitor(activeMonitor, c);
    ShowClient(c);

    /* place the window */
    if (! c->monitor->desktops[c->desktop].isDynamic
            && ! (c->types & NetWMTypeFixed)
            && ! (IsFixed(c->normals))) {
        if (c->states & (NetWMStateMaximized|NetWMStateFullscreen)) {
            /* honor states */
            if (c->states & NetWMStateMaximizedHorz)
                MaximizeClientHorizontally(c);
            if (c->states & NetWMStateMaximizedVert)
                MaximizeClientVertically(c);
            if (c->states & NetWMStateFullscreen)
                FullscreenClient(c);
        } else {
            /* honor placement strategy if any */
            Desktop *d = &c->monitor->desktops[c->desktop];
            int nx = c->fx;
            int ny = c->fy;
            int nw = Min(c->fw, d->ww); 
            int nh = Min(c->fh, d->wh);
            if (settings.placement == StrategyCenter) {
                nx =  d->wx + (d->ww - nw) / 2;
                ny =  d->wy + (d->wh - nh) / 2;
            }
            if (settings.placement == StrategyPointer) {
                Window rr, cr;
                int rx, ry, wx, wy;
                unsigned int mr;
                if (XQueryPointer(display, w, &rr, &cr, &rx, &ry,
                            &wx, &wy, &mr)) {
                    nx = rx - nw / 2;
                    ny = ry - nh / 2;
                }
            }
            /* be sure to be fully visible */
            nx = Max(d->wx, Min(d->wx + d->ww - nw , nx));
            ny = Max(d->wy, Min(d->wy + d->wh - nh , ny));
            MoveResizeClientFrame(c, nx, ny, nw, nh, False);
        }
    }

    /* if transient for someone center ourself */
    if (c->transfor) {
        MoveClientFrame(c,
            c->transfor->fx + (c->transfor->fw - ww) / 2,
            c->transfor->fy + (c->transfor->fh - wh) / 2);
    }

    /* map */
    XMapWindow(display, c->frame);
    if (! mapped)
        XMapWindow(display, w);

    if (c->hasTopbar) {
        XMapWindow(display, c->topbar);
        for (int i = 0; i < ButtonCount; ++i) {
            XMapWindow(display, c->buttons[i]);
        }
    }

    if (c->hasHandles)
        for (int i = 0; i < HandleCount; ++i)
            XMapWindow(display, c->handles[i]);

    if (c->hints & HintsFocusable && !(c->types & NetWMTypeFixed)) {
        SetFocusedClient(c);
        /* let anyone interrested in ewmh knows what we honor */
        SetNetWMAllowedActions(w, NetWMActionDefault);
    }

    /* if dynamic we need to refresh the tiling */
    if (c->monitor->desktops[c->desktop].isDynamic)
        RefreshMonitor(c->monitor);

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
        t->client->transfor = NULL;
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
    DetachClient(c);

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

    if (c->hasTopbar) {
        for (int i = 0; i < ButtonCount; ++i)
            XDestroyWindow(display, c->buttons[i]);
        XDestroyWindow(display, c->topbar);
    }

    if (c->hasHandles)
        for (int i = 0; i < HandleCount; ++i)
            XDestroyWindow(display, c->handles[i]);

    XDestroyWindow(display, c->frame);

    if (destroyed)
        XRemoveFromSaveSet(display, c->window);

    free(c);

    /* update the client list */
    XDeleteProperty(display, root, atoms[AtomNetClientList]);
    for (Client *it = clients; it; it = it->next)
        XChangeProperty(display, root, atoms[AtomNetClientList], XA_WINDOW, 32,
                PropModeAppend, (unsigned char *) &it->window, 1);

    if (! activeClient)
        SetFocusedClient(NULL);
}

Client *
LookupClient(Window w)
{
    for (Client *c = clients; c; c = c->next) {
        if (c->window == w || c->frame == w || c->topbar == w)
            return c;

        for (int i = 0; i < ButtonCount; ++i)
            if (c->buttons[i] == w)
                return c;

        for (int i = 0; i < HandleCount; ++i)
            if (c->handles[i] == w)
                return c;
    };
    return NULL;
}

void
Quit()
{
    StopEventLoop();
}

void
Reload()
{
    LoadConfigFile();
    for (Monitor *m = monitors; m; m = m->next) {
        for (int i = 0; i < DesktopCount; ++i) {
            m->desktops[i].masters = settings.masters;
            m->desktops[i].split = settings.split;
        }
    }

    for (Client *c = clients; c; c = c->next) {
        SynchronizeWindowGeometry(c);
        ShowClient(c);
    }

    RefreshMonitor(activeMonitor);
}

void
SetFocusedMonitor(Monitor *m)
{
    if (m)
        activeMonitor = m;
    else
        activeMonitor = monitors;
    SetFocusedClient(NULL);
}

void
FocusNextMonitor()
{
    SetFocusedMonitor(NextMonitor(activeMonitor));
    SetFocusedClient(NULL);
}

void
FocusPreviousMonitor()
{
    SetFocusedMonitor(PreviousMonitor(activeMonitor));
    SetFocusedClient(NULL);
}

void
SetFocusedClient(Client *c)
{
    Client *n = c;

    if (activeClient && activeClient == n)
        return;

    /* we need to find a new one to activate */
    if (!n)  {
        if (lastActiveClient
                && IsClientFocusable(lastActiveClient)
                && !(lastActiveClient->states & NetWMStateHidden))
            n = lastActiveClient;
        else if (activeMonitor->head)
            for (n = activeMonitor->head;
                    n && (!IsClientFocusable(n)
                        || n->states & NetWMStateHidden);
                    n = n->snext);
    }

    /* the last active is now the current active (could be NULL) */
    lastActiveClient = activeClient;

    /* the current active, if exists, should not be active anymore */
    if (activeClient)
        FocusClient(activeClient, False);
    activeClient = NULL;

    /* if someone is to be activated do it */
    if (n && IsClientFocusable(n)) {
        FocusClient(n, True);
        activeClient = n;
        RaiseClient(n);
        if (n->monitor != activeMonitor)
            SetFocusedMonitor(n->monitor);
        XChangeProperty(display, root, atoms[AtomNetActiveWindow],
                XA_WINDOW, 32, PropModeReplace, (unsigned char *)&n->window, 1);
    } else {
        /* otherwise let everybody know there's no more active client */
        XDeleteProperty(display, root, atoms[AtomNetActiveWindow]);
    }
}

void
FocusNextClient()
{
    Client *h = activeClient ? activeClient : activeMonitor->head;
    Client *n = NULL;
    for (n = h ? h->snext ? h->snext : h->monitor->head : h;
            n && n != h && (!IsClientFocusable(n) || n->transfor);
            n = n->snext ? n->snext : n->monitor->head);

    if (n && IsClientFocusable(n)) {
        if (!n->isVisible)
            RestoreClient(n);
        SetFocusedClient(n);
    }
}

void
FocusPreviousClient()
{
    Client *h = activeClient ? activeClient : activeMonitor->head;
    Client *p = NULL;

    for (p = h ? h->sprev ? h->sprev : h->monitor->tail : h;
            p && p != h && (!IsClientFocusable(p) || p->transfor);
            p = p->sprev ? p->sprev : p->monitor->tail);

    if (p && IsClientFocusable(p)) {
        if (!p->isVisible)
            RestoreClient(p);
        SetFocusedClient(p);
    }
}

void
AttachClient(Client *c)
{
    c->next = clients;
    clients = c;
}

void
DetachClient(Client *c)
{
    Client **tc;
    for (tc = &clients; *tc && *tc != c; tc = &(*tc)->next);
    *tc = c->next;
}

