#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "client.h"
#include "settings.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "settings.h"

static void FindFile(const char *name, char *dest);
static void SplitLine(char *str, char **key, char **val);
static void SetIntValue(const char *val, void *to);
static void SetFloatValue(const char *val, void *to);
static void SetStrValue(const char *val, void *to);
static void SetColValue(const char *val, void *to);

/* default #include settings */
Settings settings = {
    /* Globals */
    .labelFontname  = "Sans 10",
    .iconFontname   = "Sans 10",

    /* Toplevel windows */
    .borderWidth    = 2,
    .topbarHeight   = 28,
    .handleWidth    = 6,
    .buttonSize     = 16,
    .buttonGap      = 8,
    .buttonShape    = ButtonRectangle,
    .activeTileBackground               = 0x005577,
    .inactiveTileBackground             = 0x444444,
    .activeBackground                   = 0xE0E0E0,
    .activeForeground                   = 0x202020,
    .activeBorder                       = 0x005577,
    .inactiveBackground                 = 0xA0A0A0,
    .inactiveForeground                 = 0x202020,
    .inactiveBorder                     = 0x101010,
    .urgentBackground                   = 0xFF0000,
    .urgentForeground                   = 0x202020,
    .urgentBorder                       = 0xEE0000,
    /* Buttons */
    .buttonStyles = {
        /* close */
        {
            .icon                       = "",
            .activeBackground           = 0xE0E0E0,
            .activeForeground           = 0x202020,
            .activeBorder               = 0x808080,
            .inactiveBackground         = 0xA0A0A0,
            .inactiveForeground         = 0x202020,
            .inactiveBorder             = 0x808080,
            .activeHoveredBackground    = 0xFC4138,
            .activeHoveredForeground    = 0xE0E0E0,
            .activeHoveredBorder        = 0x808080,
            .inactiveHoveredBackground  = 0xFC4138,
            .inactiveHoveredForeground  = 0xE0E0E0,
            .inactiveHoveredBorder      = 0x808080
        },
        /* maximize */
        {
            .icon                       = "",
            .activeBackground           = 0xE0E0E0,
            .activeForeground           = 0x202020,
            .activeBorder               = 0x808080,
            .inactiveBackground         = 0xA0A0A0,
            .inactiveForeground         = 0x202020,
            .inactiveBorder             = 0x808080,
            .activeHoveredBackground    = 0xFAFAFA,
            .activeHoveredForeground    = 0x202020,
            .activeHoveredBorder        = 0x808080,
            .inactiveHoveredBackground  = 0xB0B0B0,
            .inactiveHoveredForeground  = 0x2A2A2A,
            .inactiveHoveredBorder      = 0x808080
        },
        /* minimize */
        {
            .icon                       = "",
            .activeBackground           = 0xE0E0E0,
            .activeForeground           = 0x202020,
            .activeBorder               = 0x808080,
            .inactiveBackground         = 0xA0A0A0,
            .inactiveForeground         = 0x202020,
            .inactiveBorder             = 0x808080,
            .activeHoveredBackground    = 0xFAFAFA,
            .activeHoveredForeground    = 0x202020,
            .activeHoveredBorder        = 0x808080,
            .inactiveHoveredBackground  = 0xB0B0B0,
            .inactiveHoveredForeground  = 0x2A2A2A,
            .inactiveHoveredBorder      = 0x808080
        },
    },

    /* global */
    .focusFollowsPointer = False,
    /* dynamic desktops */
    .decorateTiles  = True,
    .masters        = 1,
    .split          = .6,

    .shortcuts = {
        { Modkey | ShiftMask,   XK_q,         CV,     { .vcb={StopEventLoop} } },
        { Modkey | ShiftMask,   XK_r,         CV,     { .vcb={ReloadConfig} } },
        { Modkey | ShiftMask,   XK_k,         CC,     { .ccb={KillClient} } },
        { Modkey | ShiftMask,   XK_h,         CC,     { .ccb={MaximizeClientHorizontally} } },
        { Modkey | ShiftMask,   XK_v,         CC,     { .ccb={MaximizeClientVertically} } },
        { Modkey | ShiftMask,   XK_Left,      CC,     { .ccb={MaximizeClientLeft} } },
        { Modkey | ShiftMask,   XK_Right,     CC,     { .ccb={MaximizeClientRight} } },
        { Modkey | ShiftMask,   XK_Up,        CC,     { .ccb={MaximizeClientTop} } },
        { Modkey | ShiftMask,   XK_Down,      CC,     { .ccb={MaximizeClientBottom} } },
        { Modkey,               XK_Up,        CC,     { .ccb={MaximizeClient} } },
        { Modkey,               XK_c,         CC,     { .ccb={CenterClient} } },
        { Modkey | ControlMask, XK_Left,      CC,     { .ccb={MoveClientLeftmost} } },
        { Modkey | ControlMask, XK_Up,        CC,     { .ccb={MoveClientTopmost} } },
        { Modkey | ControlMask, XK_Right,     CC,     { .ccb={MoveClientRightmost} } },
        { Modkey | ControlMask, XK_Down,      CC,     { .ccb={MoveClientBottommost} } },
        { Modkey,               XK_Down,      CC,     { .ccb={RestoreClient} } },
        { Modkey,               XK_Delete,    CC,     { .ccb={MinimizeClient} } },
        { Modkey,               XK_t,         CC,     { .ccb={ToggleClientTopbar} } },
        { Modkey,               XK_Tab,       CV,     { .vcb={SwitchToNextClient} } },
        { Modkey,               XK_Right,     CV,     { .vcb={SwitchToNextClient} } },
        { Modkey | ShiftMask,   XK_Tab,       CV,     { .vcb={SwitchToPreviousClient} } },
        { Modkey,               XK_Left,      CV,     { .vcb={SwitchToPreviousClient} } },
        { Modkey,               XK_1,         CMI,    { .micb={ShowDesktop, 0} } },
        { Modkey,               XK_2,         CMI,    { .micb={ShowDesktop, 1} } },
        { Modkey,               XK_3,         CMI,    { .micb={ShowDesktop, 2} } },
        { Modkey,               XK_4,         CMI,    { .micb={ShowDesktop, 3} } },
        { Modkey,               XK_5,         CMI,    { .micb={ShowDesktop, 4} } },
        { Modkey,               XK_6,         CMI,    { .micb={ShowDesktop, 5} } },
        { Modkey,               XK_7,         CMI,    { .micb={ShowDesktop, 6} } },
        { Modkey,               XK_8,         CMI,    { .micb={ShowDesktop, 7} } },
        { Modkey,               XK_Page_Down, CM,     { .mcb={SwitchToNextDesktop} } },
        { Modkey,               XK_Page_Up,   CM,     { .mcb={SwitchToPreviousDesktop} } },
        { Modkey | ShiftMask,   XK_1,         CCI,    { .cicb={MoveClientToDesktop, 0} } },
        { Modkey | ShiftMask,   XK_2,         CCI,    { .cicb={MoveClientToDesktop, 1} } },
        { Modkey | ShiftMask,   XK_3,         CCI,    { .cicb={MoveClientToDesktop, 2} } },
        { Modkey | ShiftMask,   XK_4,         CCI,    { .cicb={MoveClientToDesktop, 3} } },
        { Modkey | ShiftMask,   XK_5,         CCI,    { .cicb={MoveClientToDesktop, 4} } },
        { Modkey | ShiftMask,   XK_6,         CCI,    { .cicb={MoveClientToDesktop, 5} } },
        { Modkey | ShiftMask,   XK_7,         CCI,    { .cicb={MoveClientToDesktop, 6} } },
        { Modkey | ShiftMask,   XK_8,         CCI,    { .cicb={MoveClientToDesktop, 7} } },
        { Modkey | ShiftMask,   XK_Page_Down, CC,     { .ccb={MoveClientToNextDesktop } } },
        { Modkey | ShiftMask,   XK_Page_Up,   CC,     { .ccb={MoveClientToPreviousDesktop } } },
        { Modkey,               XK_d,         CM,     { .mcb={ToggleDynamic} } },
        { Modkey | ShiftMask,   XK_t,         CM,     { .mcb={ToggleTopbar} } },
        { Modkey,               XK_equal,     CMI,    { .micb={AddMaster, 1} } },
        { Modkey,               XK_minus,     CMI,    { .micb={AddMaster, -1} } },
        { Modkey | ShiftMask | ControlMask, XK_Right,     CC,     { .ccb={StackClientDown} } },
        { Modkey | ShiftMask | ControlMask, XK_Left,      CC,     { .ccb={StackClientUp} } },
        { Modkey,               XK_period,    CV,     { .vcb={SwitchToNextMonitor} } },
        { Modkey,               XK_comma,     CV,     { .vcb={SwitchToPreviousMonitor} } },
        { Modkey | ShiftMask,   XK_period,    CC,     { .ccb={MoveClientToNextMonitor} } },
        { Modkey | ShiftMask,   XK_comma,     CC,     { .ccb={MoveClientToPreviousMonitor} } }
    },
};

static struct {
    char *key;
    void *to;
    void (*set)(const char *, void *);
} callbacks[] = {
    {"LabelFont",                           (void*)&settings.labelFontname,                             SetStrValue},
    {"IconFont",                            (void*)&settings.iconFontname,                              SetStrValue},
    {"BorderWidth",                         (void*)&settings.borderWidth,                               SetIntValue},
    {"TopbarHeight",                        (void*)&settings.topbarHeight,                              SetIntValue},
    {"HandleWidth",                         (void*)&settings.handleWidth,                               SetIntValue},
    {"ButtonSize",                          (void*)&settings.buttonSize,                                SetIntValue},
    {"ButtonGap",                           (void*)&settings.buttonGap,                                 SetIntValue},
    {"ButtonShape",                         (void*)&settings.buttonShape,                               SetIntValue},
    {"ActiveBackground",                    (void*)&settings.activeBackground,                          SetColValue},
    {"ActiveForeground",                    (void*)&settings.activeForeground,                          SetColValue},
    {"ActiveBorder",                        (void*)&settings.activeBorder,                              SetColValue},
    {"InactiveBackground",                  (void*)&settings.inactiveBackground,                        SetColValue},
    {"InactiveForeground",                  (void*)&settings.inactiveForeground,                        SetColValue},
    {"InactiveBorder",                      (void*)&settings.inactiveBorder,                            SetColValue},
    {"UrgentBackground",                    (void*)&settings.urgentBackground,                          SetColValue},
    {"UrgentForeground",                    (void*)&settings.urgentForeground,                          SetColValue},
    {"UrgentBorder",                        (void*)&settings.urgentBorder,                              SetColValue},
    {"ActiveTileBackground",                (void*)&settings.activeTileBackground,                      SetColValue},
    {"InactiveTileBackground",              (void*)&settings.inactiveTileBackground,                    SetColValue},
    {"CloseIcon",                           (void*)&settings.buttonStyles[0].icon,                      SetStrValue},
    {"CloseActiveBackground",               (void*)&settings.buttonStyles[0].activeBackground,          SetColValue},
    {"CloseActiveForeground",               (void*)&settings.buttonStyles[0].activeForeground,          SetColValue},
    {"CloseActiveBorder",                   (void*)&settings.buttonStyles[0].activeBorder,              SetColValue},
    {"CloseInactiveBackground",             (void*)&settings.buttonStyles[0].inactiveBackground,        SetColValue},
    {"CloseInactiveForeground",             (void*)&settings.buttonStyles[0].inactiveForeground,        SetColValue},
    {"CloseInactiveBorder",                 (void*)&settings.buttonStyles[0].inactiveBorder,            SetColValue},
    {"CloseActiveHoveredBackground",        (void*)&settings.buttonStyles[0].activeHoveredBackground,   SetColValue},
    {"CloseActiveHoveredForeground",        (void*)&settings.buttonStyles[0].activeHoveredForeground,   SetColValue},
    {"CloseActiveHoveredBorder",            (void*)&settings.buttonStyles[0].activeHoveredBorder,       SetColValue},
    {"CloseInactiveHoveredBackground",      (void*)&settings.buttonStyles[0].inactiveHoveredBackground, SetColValue},
    {"CloseInactiveHoveredForeground",      (void*)&settings.buttonStyles[0].inactiveHoveredForeground, SetColValue},
    {"CloseInactiveHoveredBorder",          (void*)&settings.buttonStyles[0].inactiveHoveredBorder,     SetColValue},
    {"MaximizeIcon",                        (void*)&settings.buttonStyles[1].icon,                      SetStrValue},
    {"MaximizeActiveBackground",            (void*)&settings.buttonStyles[1].activeBackground,          SetColValue},
    {"MaximizeActiveForeground",            (void*)&settings.buttonStyles[1].activeForeground,          SetColValue},
    {"MaximizeActiveBorder",                (void*)&settings.buttonStyles[1].activeBorder,              SetColValue},
    {"MaximizeInactiveBackground",          (void*)&settings.buttonStyles[1].inactiveBackground,        SetColValue},
    {"MaximizeInactiveForeground",          (void*)&settings.buttonStyles[1].inactiveForeground,        SetColValue},
    {"MaximizeInactiveBorder",              (void*)&settings.buttonStyles[1].inactiveBorder,            SetColValue},
    {"MaximizeActiveHoveredBackground",     (void*)&settings.buttonStyles[1].activeHoveredBackground,   SetColValue},
    {"MaximizeActiveHoveredForeground",     (void*)&settings.buttonStyles[1].activeHoveredForeground,   SetColValue},
    {"MaximizeActiveHoveredBorder",         (void*)&settings.buttonStyles[1].activeHoveredBorder,       SetColValue},
    {"MaximizeInactiveHoveredBackground",   (void*)&settings.buttonStyles[1].inactiveHoveredBackground, SetColValue},
    {"MaximizeInactiveHoveredForeground",   (void*)&settings.buttonStyles[1].inactiveHoveredForeground, SetColValue},
    {"MaximizeInactiveHoveredBorder",       (void*)&settings.buttonStyles[1].inactiveHoveredBorder,     SetColValue},
    {"MinimizeIcon",                        (void*)&settings.buttonStyles[2].icon,                      SetStrValue},
    {"MinimizeActiveBackground",            (void*)&settings.buttonStyles[2].activeBackground,          SetColValue},
    {"MinimizeActiveForeground",            (void*)&settings.buttonStyles[2].activeForeground,          SetColValue},
    {"MinimizeActiveBorder",                (void*)&settings.buttonStyles[2].activeBorder,              SetColValue},
    {"MinimizeInactiveBackground",          (void*)&settings.buttonStyles[2].inactiveBackground,        SetColValue},
    {"MinimizeInactiveForeground",          (void*)&settings.buttonStyles[2].inactiveForeground,        SetColValue},
    {"MinimizeInactiveBorder",              (void*)&settings.buttonStyles[2].inactiveBorder,            SetColValue},
    {"MinimizeActiveHoveredBackground",     (void*)&settings.buttonStyles[2].activeHoveredBackground,   SetColValue},
    {"MinimizeActiveHoveredForeground",     (void*)&settings.buttonStyles[2].activeHoveredForeground,   SetColValue},
    {"MinimizeActiveHoveredBorder",         (void*)&settings.buttonStyles[2].activeHoveredBorder,       SetColValue},
    {"MinimizeInactiveHoveredBackground",   (void*)&settings.buttonStyles[2].inactiveHoveredBackground, SetColValue},
    {"MinimizeInactiveHoveredForeground",   (void*)&settings.buttonStyles[2].inactiveHoveredForeground, SetColValue},
    {"MinimizeInactiveHoveredBorder",       (void*)&settings.buttonStyles[2].inactiveHoveredBorder,     SetColValue},
    {"FocusFollowsPointer",                 (void*)&settings.focusFollowsPointer,                       SetIntValue},
    {"Masters",                             (void*)&settings.masters,                                   SetIntValue},
    {"Split",                               (void*)&settings.split,                                     SetFloatValue},
    {"DecorateTiles",                       (void*)&settings.decorateTiles,                             SetIntValue}
};

void
SplitLine(char *str, char **key, char **val)
{
    char *end;
    *key = *val = NULL;

    while (isspace((unsigned char)*str)) str++;
    if (*str == 0 || *str == '#')
        return;

    *key = str;
    while (! isspace((unsigned char)*str)) str++;
    if (*str == 0)
        return;
    *str++ = '\0';

    while (isspace((unsigned char)*str)) str++;
    if (*str == 0)
        return;

    *val = str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

void
SetIntValue(const char *val, void *to)
{
    *(int*)to = atoi(val);
}

void
SetFloatValue(const char *val, void *to)
{
    *(float*)to = atof(val);
}
void
SetStrValue(const char *val, void *to)
{
    strcpy(to, val);
}

void
SetColValue(const char *val, void *to)
{
    *(int*)to = (int)strtol(val, NULL, 16);
}

void
FindFile(const char *name, char *dest)
{
    char *home;
    char *format =
        "./%s:"
        "%s/.%s:"
        "%s/.config/stack/%s:"
        "/usr/local/etc/%s:"
        "/etc/%s";
    char *paths = NULL;
    char *token = NULL;

    dest[0] = '\0';

    paths = malloc(1024);
    if (!paths) {
        ELog("Can't allocate config path.");
        return;
    }

    if (!(home = getenv("HOME"))) {
        ELog("Can't get HOME directory.");
        return;
    }

    if (sprintf(paths, format, name, home, name, home, name, name, name) < 0) {
         ELog("Can't format path.");
        return;
    }

    token = strtok(paths, ":");
    while(token) {
        if (access(token, F_OK) == 0) {
            strcpy(dest, token);
            break;
        }
        token = strtok(NULL, ":");
    }
    free(paths);
}

void
LoadConfigFile()
{
    char configFile[256];
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *stream;

    FindFile("stack.conf", configFile);

    if (configFile[0] == '\0') {
        ILog("No configuration file defined.");
        return;
    }

    ILog("Using %s configuration file", configFile);

    stream = fopen(configFile, "r");
    if (stream == NULL) {
        ELog("Can't open config file, %s", strerror(errno));
        return;
    }

    while ((nread = getline(&line, &len, stream)) != -1) {
        char *key, * val;
        SplitLine(line, &key, &val);
        if (key && val)
            for (long unsigned int i = 0; i < sizeof(callbacks)/sizeof(callbacks[0]); i++)
                if (!strcmp(callbacks[i].key, key))
                    callbacks[i].set(val, callbacks[i].to);
    }

    free(line);
    fclose(stream);
    return;
}

void
ExecAutostartFile()
{
    char rcFile[256];
    struct stat st;

    FindFile("stackrc", rcFile);

    if (rcFile[0] == '\0') {
        ILog("No rc file defined.");
    }

    if (stat(rcFile, &st) == 0 && st.st_mode & S_IXUSR) {
        char cmd[512];
        sprintf(cmd, "sh -c %s &", rcFile);
        ILog("execute: %s.", rcFile);
        system(cmd);
    } else {
        ELog("rc file is not executable.");
    }
}
