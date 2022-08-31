#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "client.h"
#include "config.h"
#include "stack.h"

Config stConfig = {
    /* Globals */
    .labelFontname  = "Sans-10",
    .iconFontname   = "Material Icons Sharp:style=Regular:antialias=true:pixelsize=16",
    /*.iconFontname = "Sans:antialias=true:size=10",*/

    /* Toplevel windows */
    .borderWidth    = 1,
    .topbarHeight   = 28,
    .handleWidth    = 6,
    .buttonSize     = 28,
    .buttonGap      = 4,
    .activeTileBackground               = 0x005077,
    .inactiveTileBackground             = 0x444444,
    .activeBackground                   = 0xE0E0E0,
    .activeForeground                   = 0x202020,
    .inactiveBackground                 = 0xA0A0A0,
    .inactiveForeground                 = 0x202020,
    .urgentBackground                   = 0xFF0000,
    .urgentForeground                   = 0x202020,
    /* Buttons */
    .buttonStyles = {
        /* close */
        {
            .icon                       = "\ue5cd",
            .activeBackground           = 0xE0E0E0,
            .activeForeground           = 0x202020,
            .inactiveBackground         = 0xA0A0A0,
            .inactiveForeground         = 0x202020,
            .activeHoveredBackground    = 0xFC4138,
            .activeHoveredForeground    = 0xE0E0E0,
            .inactiveHoveredBackground  = 0xFC4138,
            .inactiveHoveredForeground  = 0xE0E0E0
        },
        /* maximize */
        {
            .icon                       = "\ue835",
            .activeBackground           = 0xE0E0E0,
            .activeForeground           = 0x202020,
            .inactiveBackground         = 0xA0A0A0,
            .inactiveForeground         = 0x202020,
            .activeHoveredBackground    = 0xF0F0F0,
            .activeHoveredForeground    = 0x202020,
            .inactiveHoveredBackground  = 0xB0B0B0,
            .inactiveHoveredForeground  = 0x202020
        },
        /* minimize */
        {
            .icon                       = "\ue931",
            .activeBackground           = 0xE0E0E0,
            .activeForeground           = 0x202020,
            .inactiveBackground         = 0xA0A0A0,
            .inactiveForeground         = 0x202020,
            .activeHoveredBackground    = 0xF0F0F0,
            .activeHoveredForeground    = 0x202020,
            .inactiveHoveredBackground  = 0xB0B0B0,
            .inactiveHoveredForeground  = 0x202020,
        },
    },

    /* Dynamic desktops */
    .masters    = 1,
    .split      = .6,

    .shortcuts = {
        { Modkey | ShiftMask,     XK_q,         CV,     { .vcb={Stop} } },
        { Modkey | ShiftMask,     XK_h,         CC,     { .ccb={MaximizeClientHorizontally} } },
        { Modkey | ShiftMask,     XK_v,         CC,     { .ccb={MaximizeClientVertically} } },
        { Modkey | ShiftMask,     XK_Left,      CC,     { .ccb={MaximizeClientLeft} } },
        { Modkey | ShiftMask,     XK_Right,     CC,     { .ccb={MaximizeClientRight} } },
        { Modkey | ShiftMask,     XK_Up,        CC,     { .ccb={MaximizeClientTop} } },
        { Modkey | ShiftMask,     XK_Down,      CC,     { .ccb={MaximizeClientBottom} } },
        { Modkey,                 XK_Up,        CC,     { .ccb={MaximizeClient} } },
        { Modkey,                 XK_Down,      CC,     { .ccb={RestoreClient} } },
        { Modkey,                 XK_Tab,       CV,     { .vcb={ActivateNext} } },
        { Modkey,                 XK_Right,     CV,     { .vcb={ActivateNext} } },
        { Modkey | ShiftMask,     XK_Tab,       CV,     { .vcb={ActivatePrev} } },
        { Modkey,                 XK_Left,      CV,     { .vcb={ActivatePrev} } },
        { Modkey,                 XK_1,         CI,     { .icb={ShowDesktop, 0} } },
        { Modkey,                 XK_2,         CI,     { .icb={ShowDesktop, 1} } },
        { Modkey,                 XK_3,         CI,     { .icb={ShowDesktop, 2} } },
        { Modkey,                 XK_4,         CI,     { .icb={ShowDesktop, 3} } },
        { Modkey,                 XK_5,         CI,     { .icb={ShowDesktop, 4} } },
        { Modkey,                 XK_6,         CI,     { .icb={ShowDesktop, 5} } },
        { Modkey,                 XK_7,         CI,     { .icb={ShowDesktop, 6} } },
        { Modkey,                 XK_8,         CI,     { .icb={ShowDesktop, 7} } },
        { Modkey | ShiftMask,     XK_1,         CI,     { .icb={MoveToDesktop, 0} } },
        { Modkey | ShiftMask,     XK_2,         CI,     { .icb={MoveToDesktop, 1} } },
        { Modkey | ShiftMask,     XK_3,         CI,     { .icb={MoveToDesktop, 2} } },
        { Modkey | ShiftMask,     XK_4,         CI,     { .icb={MoveToDesktop, 3} } },
        { Modkey | ShiftMask,     XK_5,         CI,     { .icb={MoveToDesktop, 4} } },
        { Modkey | ShiftMask,     XK_6,         CI,     { .icb={MoveToDesktop, 5} } },
        { Modkey | ShiftMask,     XK_7,         CI,     { .icb={MoveToDesktop, 6} } },
        { Modkey | ShiftMask,     XK_8,         CI,     { .icb={MoveToDesktop, 7} } },
        { Modkey,                 XK_d,         CV,     { .vcb={ToggleDynamic} } },
        { Modkey,                 XK_Page_Up,   CI,     { .icb={AddMaster, 1} } },
        { Modkey,                 XK_Page_Down, CI,     { .icb={AddMaster, -1} } },
        { Modkey | ControlMask,   XK_Right,     CV,     { .vcb={MoveForward} } },
        { Modkey | ControlMask,   XK_Left,      CV,     { .vcb={MoveBackward} } }
    },
};
