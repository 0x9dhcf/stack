#include <X11/extensions/Xrandr.h>

#include "log.h"
#include "x11.h"

Display        *stDisplay;
int             stScreen;
Window          stRoot;
unsigned long   stNumLockMask;
int             stXRandREventBase;

void
InitializeX11()
{
    int ebr;
    XModifierKeymap *modmap;

    /* open the display */
    stDisplay = XOpenDisplay(0);
    if (!stDisplay)
        FLog("Can't open display.");

    /* check for extensions */
    if (!XRRQueryExtension(stDisplay, &stXRandREventBase, &ebr)) {
        XCloseDisplay(stDisplay);
        FLog("Can't load XRandR extension.");
    }

    stScreen = DefaultScreen(stDisplay);
    stRoot = RootWindow(stDisplay, stScreen);

    /* setup the num lock modifier */
    modmap = XGetModifierMapping(stDisplay);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < modmap->max_keypermod; j++)
            if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(stDisplay, XK_Num_Lock))
                stNumLockMask = (1 << i);
    XFreeModifiermap(modmap);
}


void
TeardownX11()
{
    XSync(stDisplay, False);
    XCloseDisplay(stDisplay);
}

