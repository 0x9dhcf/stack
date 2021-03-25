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
          SubstructureRedirectMask                                          \
        | StructureNotifyMask /* when the user adds a screen (e.g. video    \
                               * projector), the root window gets a         \
                               * ConfigureNotify */                         \
        | PropertyChangeMask                                                \
        | ButtonPressMask                                                   \
        | PointerMotionMask                                                 \
        | FocusChangeMask                                                   \
        | EnterWindowMask                                                   \
        | LeaveWindowMask)

static Client *Lookup(Window w);
static int ErrorHandler(Display *d, XErrorEvent *e);
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

static Window supporting;
static Bool running = False;
static Output *outputs = NULL;
static Output *active = NULL;
static Client *focused = NULL;
static int (*DefaultErrorHandler)(Display *, XErrorEvent *) = NULL;

void
CycleForward()
{
    DLog();
    Client *c = FindNextFocusableClient(active, focused);
    if (c)
        FocusClient(c);
}

void
CycleBackward()
{
    DLog();
    Client *c = FindPrevFocusableClient(active, focused);
    if (c)
        FocusClient(c);
}

Client*
Lookup(Window w)
{
    for (Output *o = outputs; o; o = o->next) {
        for (Client *c = o->clients; c; c = c->next) {

            if (c->window == w || c->frame == w || c->topbar == w)
                return c;

            for (int i = 0; i < ButtonCount; ++i)
                if (c->buttons[i] == w)
                    return c;

            for (int i = 0; i < HandleCount; ++i)
                if (c->handles[i] == w)
                    return c;
        }
    }
    return NULL;
}

int
ErrorHandler(Display *d, XErrorEvent *e)
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
        return 0;
    }
    ELog("fatal error: request code=%d, error code=%d\n",
            e->request_code, e->error_code);
    return DefaultErrorHandler(d, e); /* may call exit */
}

void
OnEvent(XEvent *e)
{
    Client *c = Lookup(e->xany.window);

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
        case FocusIn:
            c ? focused = c : NULL;
        break;
        case KeyPress:
            OnKeyPress(&e->xkey);
        break;
    }

    /* in case we call the quit callback */
    if (!running)
        return;

    /* Dispatch to clients */
    if (c) {
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
            case FocusIn:
                focused = c;
                OnClientFocusIn(c, &e->xfocus);
            break;
            case FocusOut:
                OnClientFocusOut(c, &e->xfocus);
            break;
            case ButtonPress:
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
            //case KeyPress:
            //    OnClientKeyPress(c, &e->xkey);
            //break;
        }
    }
}

void
OnConfigureRequest(XConfigureRequestEvent *e)
{
    if (!Lookup(e->window)) {
        XWindowChanges xwc;
        xwc.x = e->x;
        xwc.y = e->y;
        xwc.width = e->width;
        xwc.height = e->height;
        xwc.border_width = e->border_width;
        xwc.sibling = e->above;
        xwc.stack_mode = e->detail;
        XConfigureWindow(st_dpy, e->window, e->value_mask, &xwc);
        XSync(st_dpy, False);
    }
}

void
OnMapRequest(XMapRequestEvent *e)
{
    if (! Lookup(e->window)) {
        XWindowAttributes xwa;
        XGetWindowAttributes(st_dpy, e->window, &xwa);
        if (!xwa.override_redirect) {
            Client *c = CreateClient(e->window);
            XSync(st_dpy, False);
            AttachClientToOutput(active, c);
            XSetInputFocus(st_dpy, e->window, RevertToPointerRoot, CurrentTime);
            XChangeProperty(st_dpy, st_root,
                    st_atm[AtomNetClientList], XA_WINDOW, 32, PropModeAppend,
                    (unsigned char *) &e->window, 1);
        }
    }
}

void
OnUnmapNotify(XUnmapEvent *e)
{
    DLog("%ld:%ld", e->event, e->window);
    /* ignore UnmapNotify for reparented pre-existing window */
    if (e->event == st_root || e->event == None) {
        DLog("Reparenting forget that!");
        return;
    }

    Client *c = Lookup(e->window);
    if (c) {
        DetachClientFromOutput(c->output, c);
        DestroyClient(c);

        XDeleteProperty(st_dpy, st_root, st_atm[AtomNetClientList]);
        for (Output *o = outputs; o; o = o->next)
            for (Client *c2 = o->clients; c2; c2 = c2->next)
                XChangeProperty(st_dpy, st_root, st_atm[AtomNetClientList], XA_WINDOW,
                        32, PropModeAppend, (unsigned char *) &c2->window, 1);
        /* TODO: find the new focused */
        //XSync(st_dpy, False);
    }
}

void
OnDestroyNotify(XDestroyWindowEvent *e)
{
    DLog("XXX: Not implemented yet.");
    Client *c = Lookup(e->window);
    if (c) { DLog("Found a client"); }
}

void
OnKeyPress(XKeyPressedEvent *e)
{
    KeySym keysym;
    //keysym = XkbKeycodeToKeysym(st_dpy, e->keycode, 0, e->state & ShiftMask ? 1 : 0);
    keysym = XkbKeycodeToKeysym(st_dpy, e->keycode, 0, 0);

    //if (keysym == XK_q && (ShiftMask | MODKEY) == e->state)
    //    StopMainLoop();

    // TODO manage binding
    if (keysym == (XK_Return) && CleanMask(Modkey|ShiftMask) == CleanMask(e->state))
        Spawn((char**)st_cfg.terminal);

    for (int i = 0; i < ShortcutCount; ++i) {
        if (keysym == (st_cfg.shortcuts[i].keysym) &&
                CleanMask(st_cfg.shortcuts[i].modifier) == CleanMask(e->state)) {
            switch (st_cfg.shortcuts[i].type) {
                case CallbackVoid:
                    (*st_cfg.shortcuts[i].callback.vcb)();
                    break;
                case CallbackClient:
                    if (focused) 
                        (*st_cfg.shortcuts[i].callback.ccb)(focused);
                    break;
            }
            break;
        }
    }
}

void
Spawn(char **args)
{
    DLog();
    if (fork() == 0) {
        if (st_dpy)
              close(ConnectionNumber(st_dpy));
        setsid();
        execvp((char *)args[0], (char **)args);
        ELog(" failed");
        exit(EXIT_SUCCESS);
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

/* XXX not re entrant */
void
ScanOutputs()
{
    XRRScreenResources *sr;
    XRRCrtcInfo *ci;
    int i;

    sr = XRRGetScreenResources(st_dpy, st_root);
    for (i = 0, ci = NULL; i < sr->ncrtc; ++i) {
        ci = XRRGetCrtcInfo(st_dpy, sr, sr->crtcs[i]);
        if (ci == NULL)
            continue;
        if (ci->noutput == 0) {
            XRRFreeCrtcInfo(ci);
            continue;
        }
        //DLog("Found output: (%d, %d) [%d, %d]",
        //        ci->x, ci->y, ci->width, ci->height);
        Output *o = CreateOutput(ci->x, ci->y, ci->width, ci->height);
        o->next = outputs;
        outputs = o;
        XRRFreeCrtcInfo(ci);
    }
    XRRFreeScreenResources(sr);

    /* fallback */
    if (!outputs)
        outputs = CreateOutput(0, 0, DisplayWidth(st_dpy, st_scn),
                XDisplayHeight(st_dpy, st_scn));

    active = outputs;
}

void
DestroyOutputs()
{
    DLog();
    Output *o = outputs;
    while (o) {
        Output *p = o->next;
        DestroyOutput(o);
        o = p;
    }
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

void
GrabKeys()
{
    KeyCode code;
    unsigned int modifiers[] = { 0, LockMask, st_nlm, st_nlm | LockMask };

    XUngrabKey(st_dpy, AnyKey, AnyModifier, st_root);

    // TODO manage binding
    // For now just a way to launch a terminal
    if ((code = XKeysymToKeycode(st_dpy, XK_Return)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(st_dpy, code, Modkey |ShiftMask | modifiers[j],
                    st_root, True, GrabModeSync, GrabModeAsync);

    for (int i = 0; i < ShortcutCount; ++i)
        if ((code = XKeysymToKeycode(st_dpy, st_cfg.shortcuts[i].keysym)))
            for (int j = 0; j < 4; ++j)
                XGrabKey(st_dpy, code,
                        st_cfg.shortcuts[i].modifier | modifiers[j],
                        st_root, True, GrabModeSync, GrabModeAsync);
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
