#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>

#include "event.h"
#include "manager.h"
#include "monitor.h"
#include "stack.h"

static void SetupX11();
static void CleanupX11();
static void TrapSignal(int sig);
#include "atoms.inc"
#include "cursors.inc"

Display *display;
Window root;
unsigned long numLockMask;
Atom atoms[AtomCount];
Cursor cursors[CursorCount];
XftFont *fonts[FontCount];
Monitor *monitors;

void
SetupX11()
{
    int ebr;
    int xreb;
    XModifierKeymap *modmap;
    XRRScreenResources *sr;
    XRRCrtcInfo *ci;
    int i;

    /* open the display */
    display = XOpenDisplay(0);
    if (! display)
        FLog("Can't open display.");

    /* check for extensions */
    if (! XRRQueryExtension(display, &xreb, &ebr)) {
        XCloseDisplay(display);
        FLog("Can't load XRandR extension.");
    }

    /* get the root window */
    root = RootWindow(display, DefaultScreen(display));

    /* setup the num lock modifier */
    modmap = XGetModifierMapping(display);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(display, XK_Num_Lock))
                numLockMask = (1 << i);
    XFreeModifiermap(modmap);

    /* initialize atoms */
    XInternAtoms(display, atomNames, AtomCount, False, atoms);

    /* initialize cursors */
    for (int i = 0; i < CursorCount; ++i)
        cursors[i] = XCreateFontCursor(display, cursorIds[i]);

    /* initialise fonts */
    fonts[FontLabel] = XftFontOpenName(display, 0, config.labelFontname);
    fonts[FontIcon] = XftFontOpenName(display, 0, config.iconFontname);

    /* scan for monitors */
    sr = XRRGetScreenResources(display, root);
    for (i = 0, ci = NULL; i < sr->ncrtc; ++i) {
        ci = XRRGetCrtcInfo(display, sr, sr->crtcs[i]);
        if (ci == NULL)
            continue;
        if (ci->noutput == 0) {
            XRRFreeCrtcInfo(ci);
            continue;
        }

        Monitor *m = malloc(sizeof(Monitor));
        if (!m) FLog("could not allocate memory for output.");
        memset(m, 0 , sizeof(Monitor));

        m->x = ci->x;
        m->y = ci->y;
        m->w = ci->width;
        m->h = ci->height;
        for (int j = 0; j < DesktopCount; ++j) {
            m->desktops[j].wx = m->x;
            m->desktops[j].wy = m->y;
            m->desktops[j].ww = m->w;
            m->desktops[j].wh = m->h;
            m->desktops[j].masters = config.masters;
            m->desktops[j].split = config.split;
        }
        m->activeDesktop = 0;
        m->head= NULL;
        m->tail= NULL;
        m->next = monitors;
        monitors = m;
        XRRFreeCrtcInfo(ci);
    }
    XRRFreeScreenResources(sr);

    /* fallback */
    if (!monitors) {
        monitors = malloc(sizeof(Monitor));
        monitors->x = 0;
        monitors->y = 0;
        monitors->w = DisplayWidth(display, DefaultScreen(display));
        monitors->h = XDisplayHeight(display, DefaultScreen(display));
        for (int i = 0; i < DesktopCount; ++i) {
            monitors->desktops[i].wx = monitors->x;
            monitors->desktops[i].wy = monitors->y;
            monitors->desktops[i].ww = monitors->w;
            monitors->desktops[i].wh = monitors->h;
            monitors->desktops[i].masters = config.masters;
            monitors->desktops[i].split = config.split;
        }
        monitors->activeDesktop = 0;
        monitors->head= NULL;
        monitors->tail= NULL;
        monitors->next = NULL;
    }
}

void
CleanupX11()
{
    /* release monitors */
    Monitor *m = monitors;
    while (m) {
        Monitor *p = m->next;
        free(m);
        m = p;
    }
    monitors = NULL;

    /* release x11 resources */
    XftFontClose(display, fonts[FontLabel]);
    XftFontClose(display, fonts[FontIcon]);

    for (int i = 0; i < CursorCount; ++i)
        cursors[i] = XFreeCursor(display, cursors[i]);

    XSync(display, False);
    XCloseDisplay(display);
}

void
TrapSignal(int sig)
{
    (void)sig;
    StopEventLoop();
}

int
main(int argc, char **argv)
{
    if (argc == 2 && !strcmp("-v", argv[1])) {
        printf("stack-" VERSION "\n"
                "Stacking window manager with dynamic tiling capabilities\n\n"
                "Copyright (C) 2021-2022 by 0x9dhcf <0x9dhcf at gmail dot com>\n\n"
                "Permission to use, copy, modify, and/or distribute this software for any purpose\n"
                "with or without fee is hereby granted.\n\n"
                "THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH\n"
                "REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND\n"
                "FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,\n"
                "INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS\n"
                "OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\n"
                "TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF\n"
                "THIS SOFTWARE.\n");
        exit(0);
    } else if (argc != 1) {
        fprintf(stderr, "usage: stack [-v]\n");
        exit(2);
    }

    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        ELog("no locale support");

    signal(SIGINT, TrapSignal);
    signal(SIGKILL, TrapSignal);
    signal(SIGTERM, TrapSignal);

    FindConfigFile();
    FindAutostartFile();
    LoadConfigFile();

    SetupX11();
    SetupWindowManager();
    StartEventLoop();
    CleanupWindowManager();
    CleanupX11();

    ILog("Bye...");

    return 0;
}
