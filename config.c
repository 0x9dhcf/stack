#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "config.h"
#include "manage.h"
#include "client.h"
#include "monitor.h"

/* TODO
static char *Trim(char *str);
static char *FindConfigFile();
*/

static const Config defaultConfig = {
    .borderWidth    = 1,
    .topbarHeight   = 32,
    .handleWidth    = 6,
    .buttonSize     = 32,
    .buttonGap      = 4,

    /* window */
    .activeBackground                   = 0xD8D8D8,
    .activeForeground                   = 0x202020,

    .inactiveBackground                 = 0xE8E8E8,
    .inactiveForeground                 = 0x808080,

    .urgentBackground                   = 0xFF0000,
    .urgentForeground                   = 0x202020,

    /* Buttons */
    //.activeButtonBackground             = 0xE8E8E8,
    .activeButtonBackground             = 0xD8D8D8,
    .activeButtonForeground             = 0x202020,

    //.inactiveButtonBackground           = 0xE8E8E8,
    .inactiveButtonBackground           = 0xE8E8E8,
    .inactiveButtonForeground           = 0x808080,

    .activeButtonHoveredBackground      = 0xF8F8F8,
    .activeButtonHoveredForeground      = 0x202020,

    .inactiveButtonHoveredBackground    = 0xF8F8F8,
    .inactiveButtonHoveredForeground    = 0x202020,

    .buttonIcons = { "\ue5cd", "\ue835", "\ue931" },
    //.buttonIcons = { "C", "M", "I" },
    .labelFontname = "Sans:antialias=true:size=10",
    //.iconFontname = "Font Awesome 5 Free Regular:antialias=true:size=14",
    .iconFontname = " Material Icons Sharp:style=Regular:antialias=true:pixelsize=14",

    .masters = 1,
    .split = .6,

    .shortcuts = {
        { Modkey | ShiftMask,     XK_q,         CV,     { .vcb={Quit} } },
        { Modkey | ShiftMask,     XK_h,         CC,     { .ccb={MaximizeClientHorizontally} } },
        { Modkey | ShiftMask,     XK_v,         CC,     { .ccb={MaximizeClientVertically} } },
        { Modkey | ShiftMask,     XK_Left,      CC,     { .ccb={MaximizeClientLeft} } },
        { Modkey | ShiftMask,     XK_Right,     CC,     { .ccb={MaximizeClientRight} } },
        { Modkey | ShiftMask,     XK_Up,        CC,     { .ccb={MaximizeClientTop} } },
        { Modkey | ShiftMask,     XK_Down,      CC,     { .ccb={MaximizeClientBottom} } },
        { Modkey,                 XK_Up,        CC,     { .ccb={MaximizeClient} } },
        { Modkey,                 XK_Down,      CC,     { .ccb={RestoreClient} } },
        { Modkey,                 XK_Tab,       CV,     { .vcb={ActiveNext} } },
        { Modkey | ShiftMask,     XK_Tab,       CV,     { .vcb={ActivePrev} } },
        { Modkey,                 XK_1,         CI,     { .icb={ShowDesktop, 0} } },
        { Modkey,                 XK_2,         CI,     { .icb={ShowDesktop, 1} } },
        { Modkey,                 XK_3,         CI,     { .icb={ShowDesktop, 2} } },
        { Modkey,                 XK_4,         CI,     { .icb={ShowDesktop, 3} } },
        { Modkey,                 XK_5,         CI,     { .icb={ShowDesktop, 4} } },
        { Modkey,                 XK_6,         CI,     { .icb={ShowDesktop, 5} } },
        { Modkey,                 XK_7,         CI,     { .icb={ShowDesktop, 6} } },
        { Modkey,                 XK_8,         CI,     { .icb={ShowDesktop, 7} } },
        { Modkey,                 XK_9,         CI,     { .icb={ShowDesktop, 8} } },
        { Modkey,                 XK_0,         CI,     { .icb={ShowDesktop, 9} } },
        { Modkey | ShiftMask,     XK_1,         CI,     { .icb={MoveToDesktop, 0} } },
        { Modkey | ShiftMask,     XK_2,         CI,     { .icb={MoveToDesktop, 1} } },
        { Modkey | ShiftMask,     XK_3,         CI,     { .icb={MoveToDesktop, 2} } },
        { Modkey | ShiftMask,     XK_4,         CI,     { .icb={MoveToDesktop, 3} } },
        { Modkey | ShiftMask,     XK_5,         CI,     { .icb={MoveToDesktop, 4} } },
        { Modkey | ShiftMask,     XK_6,         CI,     { .icb={MoveToDesktop, 5} } },
        { Modkey | ShiftMask,     XK_7,         CI,     { .icb={MoveToDesktop, 6} } },
        { Modkey | ShiftMask,     XK_8,         CI,     { .icb={MoveToDesktop, 7} } },
        { Modkey | ShiftMask,     XK_9,         CI,     { .icb={MoveToDesktop, 8} } },
        { Modkey | ShiftMask,     XK_0,         CI,     { .icb={MoveToDesktop, 9} } },
        { Modkey,                 XK_d,         CV,     { .vcb={ToggleDynamic} } },
        { Modkey,                 XK_Page_Up,   CI,     { .icb={AddMaster, 1} } },
        { Modkey,                 XK_Page_Down, CI,     { .icb={AddMaster, -1} } },
        { Modkey | ControlMask,   XK_Right,     CV,     { .vcb={MoveForward} } },
        { Modkey | ControlMask,   XK_Left,      CV,     { .vcb={MoveBackward} } }
    },
};

/*
static const char *paths[] = {
    "USER_DIR/.sstrc",
    "USER_DIR/.config/sstrc",
    "USER_DIR/.config/sst/sstrc",
    "/etc/sstrc",
    "/etc/sst/sstrc",
    "/usr/local/etc/sstrc",
    "/usr/local/etc/sst/sstrc",
    NULL
};
*/

Config stConfig;

/* TODO */
void
LoadConfig()
{
    memcpy(&stConfig, &defaultConfig, sizeof(Config));
}

/* TODO
char *
FindConfigFile()
{
//    char *home_dir;
//
//    if (const char* c = getenv("HOME"))
//          home_dir.assign(c);
//
//      for (auto path : paths) {
//          if (size_t p = path.find("USER_DIR") != path.npos)
//              path.replace(p - 1, 8, home_dir);
//         if (std::filesystem::exists(path))
//              return path;
//      }
//      return {};

}

char *
Trim(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}
*/
