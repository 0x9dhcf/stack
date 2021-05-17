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
    .buttonSize     = 22,
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
    //.iconFontname = "Siji:style=regular:size=18",
    //.iconFontname = "-wuncon-siji-medium-r-normal--10-100-75-75-c-80-iso10646-1",
    //.iconFontname = "Font Awesome 5 Free Regular:size=11",
    //.iconFontname = "Font Awesome 5 Free Regular:antialias=true:size=14",
    .iconFontname = " Material Icons Outlined:style=Regular:antialias=true:size=14",
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
        { Modkey,                 XK_0,     CI,     { .icb={ShowActiveMonitorDesktop, 9} } },
        { Modkey | ShiftMask,     XK_1,     CI,     { .icb={MoveActiveClientToDesktop, 0} } },
        { Modkey | ShiftMask,     XK_2,     CI,     { .icb={MoveActiveClientToDesktop, 1} } },
        { Modkey | ShiftMask,     XK_3,     CI,     { .icb={MoveActiveClientToDesktop, 2} } },
        { Modkey | ShiftMask,     XK_4,     CI,     { .icb={MoveActiveClientToDesktop, 3} } },
        { Modkey | ShiftMask,     XK_5,     CI,     { .icb={MoveActiveClientToDesktop, 4} } },
        { Modkey | ShiftMask,     XK_6,     CI,     { .icb={MoveActiveClientToDesktop, 5} } },
        { Modkey | ShiftMask,     XK_7,     CI,     { .icb={MoveActiveClientToDesktop, 6} } },
        { Modkey | ShiftMask,     XK_8,     CI,     { .icb={MoveActiveClientToDesktop, 7} } },
        { Modkey | ShiftMask,     XK_9,     CI,     { .icb={MoveActiveClientToDesktop, 8} } },
        { Modkey | ShiftMask,     XK_0,     CI,     { .icb={MoveActiveClientToDesktop, 9} } }
    },
    .terminal = {"uxterm", NULL}
};

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
