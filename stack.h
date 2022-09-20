#ifndef __STACK_H__
#define __STACK_H__


#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#ifndef VERSION
    #define VERSION "0.0.0"
#endif

// TODO: make it configurable
#define Modkey Mod1Mask
#define ModkeySym XK_Alt_L

#define Max(v1, v2) (v1 > v2 ? v1 : v2)
#define Min(v1, v2) (v1 < v2 ? v1 : v2)

#endif
