#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>

#include "stack.h"

Client *lastActiveClient = NULL;

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

    ///* we can only apply structure states once attached in floating mode */
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
Quit()
{
    stRunning = False;
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

// XXX useless call from shortcut
void
ShowDesktop(int desktop)
{
    if (desktop >= 0 && desktop < DesktopCount)
        SetActiveDesktop(stActiveMonitor, desktop);
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
        /* untile, restore all windows */
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

