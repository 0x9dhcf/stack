#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "config.h"
#include "manage.h"
#include "client.h"

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
        { Modkey | ShiftMask,     XK_q        },
        { Modkey | ShiftMask,     XK_h        },
        { Modkey | ShiftMask,     XK_v        },
        { Modkey | ShiftMask,     XK_Left     },
        { Modkey | ShiftMask,     XK_Right    },
        { Modkey | ShiftMask,     XK_Up       },
        { Modkey | ShiftMask,     XK_Down     },
        { Modkey,                 XK_Up       },
        { Modkey,                 XK_Down     },
        { Modkey,                 XK_Tab      },
        { Modkey | ShiftMask,     XK_Tab      },
        { Modkey,                 XK_1        },
        { Modkey,                 XK_2        }
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
