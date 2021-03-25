#include "X11/Xlib.h"
#include "config.h"
#include "log.h"
#include "stack.h"
#include "client.h"
#include "output.h"

#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>

static void Trap(int sig);
static int WMDetectedErrorHandler(Display *d, XErrorEvent *e);
static void Initialize();
static void Teardown();

/* globals */
Display*        st_dpy;
int             st_scn;
Window          st_root;
Atom            st_atm[AtomCount];
Cursor          st_cur[CursorCount];
Config          st_cfg;
XftFont*        st_lft;
XftFont*        st_ift;
unsigned long   st_nlm = 0;
int             st_xeb = 0;

#include "atoms.inc"
#include "cursors.inc"
#include "default.inc"

void
Trap(int sig)
{
    (void)sig;
    StopMainLoop();
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
Initialize()
{
    int ebr;
    XModifierKeymap *modmap;

    /* open the display */
    st_dpy = XOpenDisplay(0);
    if (!st_dpy)
        FLog("Can't open display.");

    /* check for extensions */
    if (!XRRQueryExtension(st_dpy, &st_xeb, &ebr)) {
        XCloseDisplay(st_dpy);
        FLog("Can't load XRandR extension.");
    }

    st_scn = DefaultScreen(st_dpy);
    st_root = RootWindow(st_dpy, st_scn);

    /* load defaut config */
    memcpy(&st_cfg, &default_config, sizeof(Config));

    /* setup the cursors */
    for (int i = 0; i < CursorCount; ++i)
        st_cur[i] = XCreateFontCursor(st_dpy, xcursors[i]);

    /* setup the atoms */
    XInternAtoms(st_dpy, xatoms, AtomCount, False, st_atm);

    /* setup the fonts */
    st_lft = XftFontOpenName(st_dpy, 0, st_cfg.label_fontname);
    st_ift = XftFontOpenName(st_dpy, 0, st_cfg.icon_fontname);

    /* setup the num lock modifier */
    modmap = XGetModifierMapping(st_dpy);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(st_dpy, XK_Num_Lock))
                st_nlm = (1 << i);
    XFreeModifiermap(modmap);

}

void
Teardown()
{
    for (int i = 0; i < CursorCount; ++i)
        st_cur[i] = XFreeCursor(st_dpy, st_cur[i]);
    XftFontClose(st_dpy, st_lft);
    XftFontClose(st_dpy, st_ift);
    XSync(st_dpy, False);
    XCloseDisplay(st_dpy);
}

void
CheckForExistingWM()
{
    /* Check for exiting wm runnig */
    XErrorHandler xerror = XSetErrorHandler(WMDetectedErrorHandler);
    XSelectInput(st_dpy, st_root, SubstructureRedirectMask);
    XSync(st_dpy, False);
    XSetErrorHandler(xerror);
    XSync(st_dpy, False);
}

int
main(int argc, char **argv)
{

    (void)argc;
    (void)argv;

    signal(SIGINT, Trap);
    signal(SIGKILL, Trap);
    signal(SIGTERM, Trap);

    Initialize();
    CheckForExistingWM();
    StartMainLoop();
    Teardown();
    ILog("Bye...");

    return 0;
}
