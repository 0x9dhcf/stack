#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "config.h"
#include "manage.h"
#include "client.h"
#include "monitor.h"

static const Config defaultConfig = {
    .borderWidth = 1,
    .topbarHeight = 28,
    .handleWidth = 6,
    .buttonSize = 20,
    .buttonGap = 5,

    /* window */
    .activeBackground                   = 0xD8D8D8,
    .activeForeground                   = 0x202020,

    .inactiveBackground                 = 0xE8E8E8,
    .inactiveForeground                 = 0x808080,

    .urgentBackground                   = 0xFF0000,
    .urgentForeground                   = 0x202020,

    /* Buttons */
    .activeButtonBackground             = 0xE8E8E8,
    .activeButtonForeground             = 0x808080,

    .inactiveButtonBackground           = 0xE8E8E8,
    .inactiveButtonForeground           = 0x808080,

    .activeButtonHoveredBackground      = 0xF8F8F8,
    .activeButtonHoveredForeground      = 0x202020,

    .inactiveButtonHoveredBackground    = 0xF8F8F8,
    .inactiveButtonHoveredForeground    = 0x202020,

    //.buttonIcons = { "\uf057", "\uf111", "\uf192" },
    .buttonIcons = { "C", "M", "I" },
    .labelFontname = "Sans:antialias=true:size=9",
    .iconFontname = "Sans:antialias=true:size=11",
    //.iconFontname = "Font Awesome 5 Free Regular:size=11",
    .shortcuts = {
        { Modkey | ShiftMask,     XK_q,     CV,     { .vcb={Quit} } },
        { Modkey | ShiftMask,     XK_h,     CV,     { .vcb={MaximizeActiveClientHorizontally} } },
        { Modkey | ShiftMask,     XK_v,     CV,     { .vcb={MaximizeActiveClientVertically} } },
        { Modkey | ShiftMask,     XK_Left,  CV,     { .vcb={MaximizeActiveClientLeft} } },
        { Modkey | ShiftMask,     XK_Right, CV,     { .vcb={MaximizeActiveClientRight} } },
        { Modkey | ShiftMask,     XK_Up,    CV,     { .vcb={MaximizeActiveClientTop} } },
        { Modkey | ShiftMask,     XK_Down,  CV,     { .vcb={MaximizeActiveClientBottom} } },
        { Modkey,                 XK_Up,    CV,     { .vcb={MaximizeActiveClient} } },
        { Modkey,                 XK_Down,  CV,     { .vcb={RestoreActiveClient} } },
        { Modkey,                 XK_Tab,   CV,     { .vcb={CycleActiveMonitorForward} } },
        { Modkey | ShiftMask,     XK_Tab,   CV,     { .vcb={CycleActiveMonitorBackward} } },
        { Modkey,                 XK_1,     CI,     { .icb={ShowActiveMonitorDesktop, 0} } },
        { Modkey,                 XK_2,     CI,     { .icb={ShowActiveMonitorDesktop, 1} } },
        { Modkey,                 XK_3,     CI,     { .icb={ShowActiveMonitorDesktop, 2} } },
        { Modkey,                 XK_4,     CI,     { .icb={ShowActiveMonitorDesktop, 3} } },
        { Modkey,                 XK_5,     CI,     { .icb={ShowActiveMonitorDesktop, 4} } },
        { Modkey,                 XK_6,     CI,     { .icb={ShowActiveMonitorDesktop, 5} } },
        { Modkey,                 XK_7,     CI,     { .icb={ShowActiveMonitorDesktop, 6} } },
        { Modkey,                 XK_8,     CI,     { .icb={ShowActiveMonitorDesktop, 7} } },
        { Modkey,                 XK_9,     CI,     { .icb={ShowActiveMonitorDesktop, 8} } },
        { Modkey,                 XK_0,     CI,     { .icb={ShowActiveMonitorDesktop, 9} } }
    },
    .terminal = {"uxterm", NULL}
};

Config stConfig;

/* TODO */
void
LoadConfig()
{
    memcpy(&stConfig, &defaultConfig, sizeof(Config));
}
