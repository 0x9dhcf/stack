#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

#include "X11/X.h"
#include "config.h"
#include "log.h"
#include "stack.h"
#include "client.h"
#include "output.h"

#define RootEventMask (\
          SubstructureRedirectMask\
        | StructureNotifyMask\
        | PropertyChangeMask\
        | ButtonPressMask\
        | PointerMotionMask\
        | FocusChangeMask\
        | EnterWindowMask\
        | LeaveWindowMask)

static Client *Lookup(Window w);
//static int ErrorHandler(Display *d, XErrorEvent *e);
static void OnEvent(XEvent *e);
static void OnConfigureRequest(XConfigureRequestEvent *e);
static void OnMapRequest(XMapRequestEvent *e);
static void OnUnmapNotify(XUnmapEvent *e);
static void OnDestroyNotify(XDestroyWindowEvent *e);
static void OnKeyPress(XKeyPressedEvent *e);
static void Spawn(char **args);
static void CreateSupportingWindow();
static void ScanOutputs();
static void DestroyOutputs();
static void ManageExistingWindows();
//static void RestoreManagedWindows();
static void GrabKeys();
static void SetActive(Client *c);

static Window supporting;
static Bool running = False;
static Output *outputs = NULL;
static Output *active = NULL;
static Client *focused = NULL;
static int (*DefaultErrorHandler)(Display *, XErrorEvent *) = NULL;

void
CycleForward()
{
    //DLog("focused %ld: ", focused->window);
    if (!focused)
        return;

    SetActive(NextOutputClient(active, focused));
}

void
CycleBackward()
{
    if (!focused)
        return;

    SetActive(PrevOutputClient(active, focused));
}

Client*
Lookup(Window w)
{
    for (Output *o = outputs; o; o = o->next) {
        Client *c = LookupOutputClient(o, w);
        if (c)
            return c;
    }
    return NULL;
}

void
OnEvent(XEvent *e)
{
    /* Catch events of interest for the WM */
    switch(e->type) {
        case ConfigureRequest:
            OnConfigureRequest(&e->xconfigurerequest);
        break;
        case MapRequest:
            OnMapRequest(&e->xmaprequest);
        break;
        case UnmapNotify:
            OnUnmapNotify(&e->xunmap);
        break;
        case DestroyNotify:
            OnDestroyNotify(&e->xdestroywindow);
        break;
        case KeyPress:
            OnKeyPress(&e->xkey);
        break;
    }

    /* in case we call the quit callback */
    if (!running || e->xany.window == st_root)
        return;

    /* Dispatch to clients */
    Client *c = Lookup(e->xany.window);
    if (c) {
        DLog("found a client:%ld, type:%d", c->window, e->type);
        switch (e->type) {
            case Expose:
                OnClientExpose(c, &e->xexpose);
            break;
            case ConfigureRequest:
                OnClientConfigureRequest(c, &e->xconfigurerequest);
            break;
            case PropertyNotify:
                OnClientPropertyNotify(c, &e->xproperty);
            break;
            case ButtonPress:
                if (c != focused)
                    SetActive(c);
                OnClientButtonPress(c, &e->xbutton);
            break;
            case ButtonRelease:
                OnClientButtonRelease(c, &e->xbutton);
            break;
            case MotionNotify:
                OnClientMotionNotify(c, &e->xmotion);
            break;
            case EnterNotify:
                OnClientEnter(c, &e->xcrossing);
            break;
            case LeaveNotify:
                OnClientLeave(c, &e->xcrossing);
            break;
            case ClientMessage:
                OnClientMessage(c, &e->xclient);
            break;
        }
    }
}



void
CreateSupportingWindow()
{
    supporting = XCreateSimpleWindow(st_dpy, st_root, 0, 0, 1, 1, 0, 0, 0);
    /* Configure supporting window */
    XChangeProperty(st_dpy, supporting, st_atm[AtomNetSupportingWMCheck],
            XA_WINDOW, 32, PropModeReplace, (unsigned char *) &supporting, 1);
    XChangeProperty(st_dpy, supporting, XA_WM_NAME, XA_STRING, 8,
            PropModeReplace, (unsigned char*)"stack", 5);
    XChangeProperty(st_dpy, supporting, st_atm[AtomNetWMPID], XA_INTEGER, 32,
            PropModeReplace, (unsigned char *) &supporting, 1);

    /* Configure the root window */
    XChangeProperty(st_dpy, st_root, st_atm[AtomNetSupportingWMCheck],
            XA_WINDOW, 32, PropModeReplace, (unsigned char *) &supporting, 1);
    XChangeProperty(st_dpy, st_root, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
            (unsigned char*)"stack", 5);
    XChangeProperty(st_dpy, st_root, st_atm[AtomNetWMPID], XA_INTEGER, 32,
            PropModeReplace, (unsigned char *) &supporting, 1);
    XSetWindowAttributes wa;
    wa.cursor = st_cur[CursorNormal];
    wa.event_mask =  RootEventMask;
    XChangeWindowAttributes(st_dpy, st_root, CWEventMask | CWCursor, &wa);

    /* Let ewmh listeners know about what is supported. */
    XChangeProperty(st_dpy, st_root, st_atm[AtomNetSupported], XA_ATOM, 32,
            PropModeReplace, (unsigned char *) st_atm , AtomCount);

    /* Reset the client list. */
    XDeleteProperty(st_dpy, st_root, st_atm[AtomNetClientList]);
}

void
ManageExistingWindows()
{
    Window *wins,  w0, w1, rwin, cwin;
    unsigned int nwins, mask;
    int rx, ry, wx, wy;

    XQueryPointer(st_dpy, st_root, &rwin, &cwin, &rx, &ry, &wx, &wy, &mask);

    /* to avoid unmap genetated by reparenting to destroy the newly created client */
    if (XQueryTree(st_dpy, st_root, &w0, &w1, &wins, &nwins)) {
        for (unsigned int i = 0; i < nwins; i++) {
            if (wins[i] == supporting)
                continue;

            XWindowAttributes xwa;
            XGetWindowAttributes(st_dpy, wins[i], &xwa);
            if (!xwa.override_redirect) {
                Client *c = CreateClient(wins[i]);
                /* reparenting does not set the e->event to root
                 * we discar events to prevent the unmap to be catched
                 * and wrongly interpreted */
                XSync(st_dpy, True);
                AttachClientToOutput(active, c);
                XSetInputFocus(st_dpy, wins[i], RevertToPointerRoot, CurrentTime);
                XChangeProperty(st_dpy, st_root, st_atm[AtomNetClientList],
                        XA_WINDOW, 32, PropModeAppend,
                        (unsigned char *) &wins[i], 1);
            }
        }
        XFree(wins);
    }
}

//void
//RestoreManagedWindows()
//{
//    DLog();
//    for (Output *o = outputs; o; o = o->next) {
//        for (Client *c = o->clients; c; c = c->next) {
//            XReparentWindow(st_dpy, c->window, st_root, c->x, c->y);
//            XSetWindowBorderWidth(st_dpy, c->window, c->sbw);
//        }
//    }
//}


// XXX: move to Action
void
SetActive(Client *c)
{
    if (focused)
        SetClientActive(focused, False);
    if (c)
        SetClientActive(c, True);
    focused = c;
}

void
StartMainLoop()
{
    if (running) {
        ELog("Already running.");
        return;
    }

    XSync(st_dpy, True);
    DefaultErrorHandler = XSetErrorHandler(ErrorHandler);

    CreateSupportingWindow();
    ScanOutputs();
    ManageExistingWindows();
    GrabKeys();
    XSync(st_dpy, False);

    ILog("Entering event loop.");
    int fd = XConnectionNumber(st_dpy);
    fd_set set;
    running = True;
    while (running) {
        struct timeval timeout = { 0, 2500 };
        FD_ZERO(&set);
        FD_SET(fd, &set);

        if (select(fd + 1, &set, NULL, NULL, &timeout) > 0) {
            while (XPending(st_dpy)) {
                XEvent e;
                XNextEvent(st_dpy, &e);
                OnEvent(&e);
                XFlush(st_dpy);
            }
        }
    }

    ILog("Cleaning up.");
    //RestoreManagedWindows();
    DestroyOutputs();
    XDestroyWindow(st_dpy, supporting);
    XSetInputFocus(st_dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XSetErrorHandler(DefaultErrorHandler);
}

void
StopMainLoop()
{
    running = False;
    XUngrabKey(st_dpy, AnyKey, AnyModifier, st_root);
}
