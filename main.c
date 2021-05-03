#include "X11/X.h"
#include "X11/Xlib.h"
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>

#include <signal.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "atoms.h"
#include "client.h"
#include "config.h"
#include "cursors.h"
#include "events.h"
#include "font.h"
#include "log.h"
#include "manage.h"
#include "monitor.h"
#include "x11.h"

#define RootEventMask (\
        SubstructureRedirectMask    /* Map/Configure Request    */\
      | SubstructureNotifyMask)     /* UnmapNotify              */

static void GrabShortcutKeys();
static void TrapSignal(int sig);
static int WMDetectedErrorHandler(Display *d, XErrorEvent *e);
static int EventLoopErrorHandler(Display *d, XErrorEvent *e);

static XErrorHandler defaultErrorHandler; 
static Window supportingWindow;

Bool stRunning;

void
GrabShortcutKeys()
{
    KeyCode code;
    unsigned int modifiers[] = { 0, LockMask, stNumLockMask, stNumLockMask | LockMask };

    XUngrabKey(stDisplay, AnyKey, AnyModifier, stRoot);

    // TODO manage binding
    // For now just a way to launch a terminal
    if ((code = XKeysymToKeycode(stDisplay, XK_Return)))
        for (int j = 0; j < 4; ++j)
            XGrabKey(stDisplay, code, Modkey | ShiftMask | modifiers[j],
                    stRoot, True, GrabModeSync, GrabModeAsync);

    for (int i = 0; i < ShortcutCount; ++i)
        if ((code = XKeysymToKeycode(stDisplay, stConfig.shortcuts[i].keysym))) {
            for (int j = 0; j < 4; ++j)
                XGrabKey(stDisplay, code,
                        stConfig.shortcuts[i].modifier | modifiers[j],
                        stRoot, True, GrabModeSync, GrabModeAsync);
        }
}

void
TrapSignal(int sig)
{
    (void)sig;
    stRunning = False;
}

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
        return 0;
    }
    ELog("fatal error: request code=%d, error code=%d\n",
            e->request_code, e->error_code);
    return defaultErrorHandler(d, e); /* may call exit */
}

void
CreateSupportingWindow()
{
    XSetWindowAttributes wa;

    supportingWindow = XCreateSimpleWindow(stDisplay, stRoot, 0, 0, 1, 1, 0,
            0, 0);

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

    wa.cursor = stCursors[CursorNormal];
    wa.event_mask = RootEventMask;
    XChangeWindowAttributes(stDisplay, stRoot, CWEventMask | CWCursor, &wa);

    /* Let ewmh listeners know about what is supported. */
    XChangeProperty(stDisplay, stRoot, stAtoms[AtomNetSupported], XA_ATOM, 32,
            PropModeReplace, (unsigned char *) stAtoms , AtomCount);

    /* Reset the client list. */
    XDeleteProperty(stDisplay, stRoot, stAtoms[AtomNetClientList]);
}

void
ManageExistingWindows()
{
    Window *wins,  w0, w1, rwin, cwin;
    unsigned int nwins, mask;
    int rx, ry, wx, wy;

    XQueryPointer(stDisplay, stRoot, &rwin, &cwin, &rx, &ry, &wx, &wy, &mask);

    /* to avoid unmap genetated by reparenting to destroy the newly created client */
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
                // XXX: WRONG (see on map request)
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
}

int
main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    int xConnection;
    fd_set fdSet;

    /* be sure to exit as properly as possible */
    signal(SIGINT, TrapSignal);
    signal(SIGKILL, TrapSignal);
    signal(SIGTERM, TrapSignal);

    LoadConfig();

    /* Initialize */
    InitializeX11();
    InitializeAtoms();
    InitializeCursors();
    InitializeFonts();
    InitializeMonitors();
    
    /* Check for exiting wm runnig */
    XErrorHandler xerror = XSetErrorHandler(WMDetectedErrorHandler);
    XSelectInput(stDisplay, stRoot, SubstructureRedirectMask);
    XSync(stDisplay, False);
    XSetErrorHandler(xerror);
    XSync(stDisplay, False);

    /* Setup */
    defaultErrorHandler = XSetErrorHandler(EventLoopErrorHandler);
    CreateSupportingWindow();
    ManageExistingWindows();
    GrabShortcutKeys();
    XSync(stDisplay, False);

    /* Start event loop */
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
                DispatchEvent(&e);
                XFlush(stDisplay);
            }
        }
    }

    /* Clean up */
    XDestroyWindow(stDisplay, supportingWindow);
    XSetInputFocus(stDisplay, PointerRoot, RevertToPointerRoot, CurrentTime);
    XSetErrorHandler(defaultErrorHandler);
    XUngrabKey(stDisplay, AnyKey, AnyModifier, stRoot);
    TeardownMonitors();
    TeardownFonts();
    TeardownCursors();
    TeardownX11();

    ILog("Bye...");

    return 0;
}
