#include <stdlib.h>
#include <string.h>

#include "atoms.h"
#include "client.h"
#include "cursors.h"
#include "events.h"
#include "hints.h"
#include "log.h"
#include "manage.h"
#include "monitor.h"
#include "stack.h"
#include "x11.h"

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
    GetNetWMState(w, &c->states);
    GetWMNormals(w, &c->normals);
    GetWMName(w, &c->name);
    GetWMClass(w, &c->wmclass);
    GetWMStrut(w, &c->strut);

    c->active = False;
    c->decorated = (c->types & NetWMTypeFixed) ? False : True;
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

    /* fixed windows are neither decorated nor resizable
     * thus don't need this */
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

    /* we can only apply structure states once attached */
    if (c->states & NetWMStateMaximizedHorz)
        MaximizeClientHorizontally(c);
    if (c->states & NetWMStateMaximizedVert)
        MaximizeClientVertically(c);

    if (c->states & NetWMStateHidden)
        MinimizeClient(c);

    if (c->states & NetWMStateFullscreen)
        FullscreenClient(c);

    if (c->states & NetWMStateAbove)
        RaiseClient(c);

    if (c->states & NetWMStateBelow)
        LowerClient(c);

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
        //if (c == c->monitor->desktops[c->desktop].activeOnLeave)
        //    c->monitor->desktops[c->desktop].activeOnLeave = NULL;
        ///* find a new home for the active window */
        //if (c == stActiveClient) {
        //    Client *nc = NULL;
        //    for (nc = NextClient(stActiveClient);
        //            nc != stActiveClient
        //                && (nc->desktop != stActiveClient->desktop
        //                || !(nc->types & NetWMTypeNormal)
        //                || nc->states & NetWMStateHidden);
        //            nc = NextClient(nc));
        //    if (nc && nc != c) {
        //        DLog("set active: %ld", nc->window);
        //        SetActiveClient(nc);
        //    }
        //    else
        //        stActiveClient = NULL;
        //}
        FindNextActiveClient();

        /* if transient for someone unregister this client */
        if (c->transfor) {
            Transient **tc;
            for (tc = &c->transfor->transients;
                    *tc && (*tc)->client != c; tc = &(*tc)->next);
            *tc = (*tc)->next;
        }

        DetachClientFromMonitor(c->monitor, c);
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
            stLastActiveClient = stActiveClient;
            SetClientActive(stActiveClient, False);
            LowerClient(c);
            SetNetWMState(c->window, c->states);
        }

        SetClientActive(c, True);
        stActiveClient = c;
        RaiseClient(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
        c->monitor->desktops[c->desktop].activeOnLeave = c;
    }
}

void
FindNextActiveClient()
{
    if (stActiveClient) {
        stActiveClient->monitor->desktops[stActiveClient->desktop].activeOnLeave = NULL;
        Client *nc = NULL;
        /* use the last active client is valid */
        if (stLastActiveClient &&
                stLastActiveClient->desktop == stActiveClient->desktop) {
            nc = stLastActiveClient;
        } else { /* try to find a new one */
            for (nc = NextClient(stActiveClient);
                    nc != stActiveClient
                        && (nc->desktop != stActiveClient->desktop
                        || !(nc->types & NetWMTypeNormal)
                        || nc->states & NetWMStateHidden);
                    nc = NextClient(nc));
        }
        if (nc && nc != stActiveClient)
            SetActiveClient(nc);
        else
            stActiveClient = NULL;
    }
}

void
Quit()
{
    stRunning = False;
}

void
MaximizeActiveClientVertically()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientVertically(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientHorizontally()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientHorizontally(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientLeft()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientLeft(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientRight()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientRight(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientTop()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientTop(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientBottom()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientBottom(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClient()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientHorizontally(stActiveClient);
        MaximizeClientVertically(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
RestoreActiveClient()
{
    if (stActiveClient) {
        RestoreClient(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
CycleActiveMonitorForward()
{
    if (stActiveClient) {
        Client *nc = NULL;
        for (nc = NextClient(stActiveClient); nc != stActiveClient
                    && (nc->desktop != stActiveClient->desktop
                    || !(nc->types & NetWMTypeNormal)
                    ||  nc->states & NetWMStateHidden);
                nc = NextClient(nc));

        if (nc)
            SetActiveClient(nc);
    }
}

void
CycleActiveMonitorBackward()
{
    if (stActiveClient) {
        Client *pc = NULL;
        for (pc = PreviousClient(stActiveClient);
                pc != stActiveClient
                    && (pc->desktop != stActiveClient->desktop
                    || !(pc->types & NetWMTypeNormal)
                    || pc->states & NetWMStateHidden);
                pc = PreviousClient(pc));
        if (pc)
            SetActiveClient(pc);
    }
}

void
ShowActiveMonitorDesktop(int desktop)
{
    if (desktop >= 0 && desktop < DesktopCount)
        SetActiveDesktop(stActiveMonitor, desktop);
}

void
MoveActiveClientToDesktop(int desktop)
{
    if (stActiveClient) {
        Client *toMove = stActiveClient;
        FindNextActiveClient();
        RemoveClientFromDesktop(toMove->monitor, toMove, toMove->desktop);
        AddClientToDesktop(toMove->monitor, toMove, desktop);
        Restack(toMove->monitor);
    }
}
