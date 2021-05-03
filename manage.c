#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "X11/X.h"
#include "atoms.h"
#include "client.h"
#include "cursors.h"
#include "hints.h"
#include "log.h"
#include "manage.h"
#include "monitor.h"
#include "stack.h"
#include "x11.h"

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)

#define FrameEvenMask (\
          ButtonPressMask               /* activate client  */\
        | SubstructureNotifyMask)       /* UnmapNotify      */

#define ClientEventMask (PropertyChangeMask)

#define ClientDoNotPropagateMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask\
        | KeyPressMask\
        | KeyReleaseMask)

Monitor *stActiveMonitor = NULL; 
Client *stActiveClient = NULL;

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

    XSetWindowAttributes fattrs = {0};
    fattrs.event_mask = FrameEvenMask;
    fattrs.backing_store = WhenMapped;
    c->frame = XCreateWindow(stDisplay, stRoot, 0, 0, 1, 1, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWEventMask | CWBackingStore, &fattrs);
    XMapWindow(stDisplay, c->frame);

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

    /* client */
    c->window = w;
    //XSetWindowAttributes cattrs = {0};
    //cattrs.event_mask = ClientEventMask;
    //cattrs.do_not_propagate_mask = ClientDoNotPropagateMask;
    //XChangeWindowAttributes(stDisplay, w, CWEventMask | CWDontPropagate, &cattrs);
    XSetWindowBorderWidth(stDisplay, w, 0);
    XReparentWindow(stDisplay, w, c->frame, 0, 0);
    if (!exists)
        XMapWindow(stDisplay, w);

    /* get fixed icccm or ewmh attributes */
    c->states = StateNone;

    Window r;
    int wx, wy;
    unsigned int ww, wh, d, b;
    XGetGeometry(stDisplay, w, &r, &wx, &wy, &ww, &wh, &b, &d);
    MoveResizeClientWindow(c, wx, wy, ww, wh, False);
    c->sbw = b;

    int flag = 0;
    GetWMProtocols(w, &flag);
    if (flag & NetWMProtocolTakeFocus)
        c->states |= StateTakeFocus;
    if (flag & NetWMProtocolDeleteWindow)
        c->states |= StateDeleteWindow;

    flag = 0;
    GetNetWMWindowType(w, &flag);
    if (flag & (NetWMTypeDesktop | NetWMTypeDock | NetWMTypeSplash))
        c->states |= StateFixed;

    if (!flag || flag & (NetWMTypeNormal | NetWMTypeDialog))
        c->states |= StateDecorated;

    GetWMNormals(w, &c->normals);
    GetWMClass(w, &c->wmclass);
    GetWMStrut(w, &c->strut);

    /* client should be attached before updating states */
    AttachClientToMonitor(stActiveMonitor, c);

    UpdateClientName(c);
    UpdateClientHints(c);
    UpdateClientState(c);

    SetActiveClient(c);

    /* update the client list */
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetClientList], XA_WINDOW,
            32, PropModeAppend, (unsigned char *) &(w), 1);
}

void
ForgetWindow(Window w)
{
    Client *c = Lookup(w);
    if (c) {
        /* find a new home for the active window */
        if (c == stActiveClient)
            SetActiveClient(NextClient(c->monitor, c));

        DetachClientFromMonitor(c->monitor, c);
        if (c->name)
            free(c->name);

        if (c->wmclass.cname)
            free(c->wmclass.cname);

        if (c->wmclass.iname)
            free(c->wmclass.iname);

        /* the client window might be destroyed already */
        XReparentWindow(stDisplay, c->window, stRoot, c->wx, c->wy);
        XSetWindowBorderWidth(stDisplay, c->window, c->sbw);

        for (int i = 0; i < ButtonCount; ++i)
            XDestroyWindow(stDisplay, c->buttons[i]);
        XDestroyWindow(stDisplay, c->topbar);

        for (int i = 0; i < HandleCount; ++i)
            XDestroyWindow(stDisplay, c->handles[i]);

        XDestroyWindow(stDisplay, c->frame);

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
        Client *c = LookupMonitorClient(m, w);
        if (c)
            return c;
    }
    return NULL;
}

void
SetActiveClient(Client *c)
{
    if (stActiveClient)
        SetClientActive(stActiveClient, False);
    if (c)
        SetClientActive(c, True);
    stActiveClient = c;
}

void
Spawn(char **args)
{
    if (fork() == 0) {
        if (stDisplay)
              close(ConnectionNumber(stDisplay));
        setsid();
        execvp((char *)args[0], (char **)args);
        ELog(" failed");
        exit(EXIT_SUCCESS);
    }
}

void
Quit()
{
    stRunning = False;
}

void
MaximizeVertically()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximizedVertically, True);
}

void
MaximizeHorizontally()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximizedHorizontally, True);
}

void MaximizeLeft()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximizedLeft, True);
}

void
MaximizeRight()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximizedRight, True);
}

void 
MaximizeTop()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximizedTop, True);
}

void
MaximizeBottom()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximizedBottom, True);
}

void
Maximize()
{
    if (stActiveClient)
        MaximizeClient(stActiveClient, StateMaximized, True);
}

void
Restore()
{
    if (stActiveClient)
        RestoreClient(stActiveClient, True);
}

void
CycleForward()
{
    if (stActiveClient)
        SetActiveClient(NextClient(stActiveMonitor, stActiveClient));
}

void
CycleBackward()
{
    if (stActiveClient)
        SetActiveClient(PreviousClient(stActiveMonitor, stActiveClient));
}
