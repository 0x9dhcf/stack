#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "client.h"
#include "config.h"
#include "stack.h"

Config stConfig = {
    /* globals */
    .labelFontname = "Sans-10",
    .iconFontname = " Material Icons Sharp:style=Regular:antialias=true:pixelsize=16",
    /*.iconFontname = "Sans:antialias=true:size=10",*/

    /* toplevel windows */
    .borderWidth    = 1,
    .topbarHeight   = 32,
    .handleWidth    = 6,
    .buttonSize     = 32,
    .buttonGap      = 4,
    .activeBackground                   = 0xD8D8D8,
    .activeForeground                   = 0x202020,
    .inactiveBackground                 = 0xE8E8E8,
    .inactiveForeground                 = 0x808080,
    .urgentBackground                   = 0xFF0000,
    .urgentForeground                   = 0x202020,

    /* buttons */
    .buttonStyles = {
        /* close */
        {
            /*.icon                       = "C",*/
            .icon                       = "\ue5cd",
            .activeBackground           = 0xD8D8D8,
            .activeForeground           = 0x202020,
            .inactiveBackground         = 0xE8E8E8,
            .inactiveForeground         = 0x808080,
            .activeHoveredBackground    = 0xFC4138,
            .activeHoveredForeground    = 0xFFFFFF,
            .inactiveHoveredBackground  = 0xFC4138,
            .inactiveHoveredForeground  = 0xFFFFFF
        },
        /* maximize */
        {
            /*.icon                       = "M",*/
            .icon                       = "\ue835",
            .activeBackground           = 0xD8D8D8,
            .activeForeground           = 0x202020,
            .inactiveBackground         = 0xE8E8E8,
            .inactiveForeground         = 0x808080,
            .activeHoveredBackground    = 0xF8F8F8,
            .activeHoveredForeground    = 0x202020,
            .inactiveHoveredBackground  = 0xF8F8F8,
            .inactiveHoveredForeground  = 0x202020
        },
        /* minimize */
        {
            /*.icon                       = "H",*/
            .icon                       = "\ue931",
            .activeBackground           = 0xD8D8D8,
            .activeForeground           = 0x202020,
            .inactiveBackground         = 0xE8E8E8,
            .inactiveForeground         = 0x808080,
            .activeHoveredBackground    = 0xF8F8F8,
            .activeHoveredForeground    = 0x202020,
            .inactiveHoveredBackground  = 0xF8F8F8,
            .inactiveHoveredForeground  = 0x202020,
        },
    },

    /* dynamic desktops */
    .masters = 1,
    .split = .6,

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
        { Modkey | ShiftMask,     XK_Tab,       CV,     { .vcb={ActivatePrev} } },
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
