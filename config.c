#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "client.h"
#include "config.h"
#include "event.h"
#include "log.h"
#include "manager.h"

static void FindFile(const char *name, char *dest);
static void SplitLine(char *str, char **key, char **val);
static void SetIntValue(const char *val, void *to);
static void SetFloatValue(const char *val, void *to);
static void SetStrValue(const char *val, void *to);
static void SetColValue(const char *val, void *to);


/* default config */
Config config = {
    /* Globals */
    .labelFontname  = "Sans-10",
    .iconFontname   = "Sans-10",

    /* Toplevel windows */
    .borderWidth    = 1,
    .topbarHeight   = 28,
    .handleWidth    = 6,
    .buttonSize     = 28,
    .buttonGap      = 4,
    .activeTileBackground               = 0x005577,
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
            .icon                       = "C",
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
            .icon                       = "M",
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
            .icon                       = "H",
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
        { Modkey,               XK_Down,      CC,     { .ccb={RestoreClient} } },
        { Modkey,               XK_Delete,    CC,     { .ccb={MinimizeClient} } },
        { Modkey,               XK_t,         CC,     { .ccb={ToggleClientTopbar} } },
        { Modkey,               XK_Tab,       CV,     { .vcb={SwitchToNextClient} } },
        { Modkey,               XK_Right,     CV,     { .vcb={SwitchToNextClient} } },
        { Modkey | ShiftMask,   XK_Tab,       CV,     { .vcb={SwitchToPreviousClient} } },
        { Modkey,               XK_Left,      CV,     { .vcb={SwitchToPreviousClient} } },
        { Modkey,               XK_1,         CI,     { .icb={ShowDesktop, 0} } },
        { Modkey,               XK_2,         CI,     { .icb={ShowDesktop, 1} } },
        { Modkey,               XK_3,         CI,     { .icb={ShowDesktop, 2} } },
        { Modkey,               XK_4,         CI,     { .icb={ShowDesktop, 3} } },
        { Modkey,               XK_5,         CI,     { .icb={ShowDesktop, 4} } },
        { Modkey,               XK_6,         CI,     { .icb={ShowDesktop, 5} } },
        { Modkey,               XK_7,         CI,     { .icb={ShowDesktop, 6} } },
        { Modkey,               XK_8,         CI,     { .icb={ShowDesktop, 7} } },
        { Modkey | ShiftMask,   XK_1,         CI,     { .icb={MoveActiveClientToDesktop, 0} } },
        { Modkey | ShiftMask,   XK_2,         CI,     { .icb={MoveActiveClientToDesktop, 1} } },
        { Modkey | ShiftMask,   XK_3,         CI,     { .icb={MoveActiveClientToDesktop, 2} } },
        { Modkey | ShiftMask,   XK_4,         CI,     { .icb={MoveActiveClientToDesktop, 3} } },
        { Modkey | ShiftMask,   XK_5,         CI,     { .icb={MoveActiveClientToDesktop, 4} } },
        { Modkey | ShiftMask,   XK_6,         CI,     { .icb={MoveActiveClientToDesktop, 5} } },
        { Modkey | ShiftMask,   XK_7,         CI,     { .icb={MoveActiveClientToDesktop, 6} } },
        { Modkey | ShiftMask,   XK_8,         CI,     { .icb={MoveActiveClientToDesktop, 7} } },
        { Modkey,               XK_d,         CV,     { .vcb={ToggleDynamicForActiveDesktop} } },
        { Modkey | ShiftMask,   XK_t,         CV,     { .vcb={ToggleTopbarForActiveDesktop} } },
        { Modkey,               XK_Page_Up,   CI,     { .icb={AddMasterToActiveDesktop, 1} } },
        { Modkey,               XK_Page_Down, CI,     { .icb={AddMasterToActiveDesktop, -1} } },
        { Modkey | ControlMask, XK_Right,     CV,     { .vcb={StackActiveClientDown} } },
        { Modkey | ControlMask, XK_Left,      CV,     { .vcb={StackActiveClientUp} } }
    },
};

static struct {
    char *key;
    void *to;
    void (*set)(const char *, void *);
} callbacks[] = {
    {"LabelFont",                           (void*)&config.labelFontname,                             SetStrValue},
    {"IconFont",                            (void*)&config.iconFontname,                              SetStrValue},
    {"BorderWidth",                         (void*)&config.borderWidth,                               SetIntValue},
    {"TopbarHeight",                        (void*)&config.topbarHeight,                              SetIntValue},
    {"HandleWidth",                         (void*)&config.handleWidth,                               SetIntValue},
    {"ButtonSize",                          (void*)&config.buttonSize,                                SetIntValue},
    {"ButtonGap",                           (void*)&config.buttonGap,                                 SetIntValue},
    {"ActiveBackground",                    (void*)&config.activeBackground,                          SetColValue},
    {"ActiveForeground",                    (void*)&config.activeForeground,                          SetColValue},
    {"InactiveBackground",                  (void*)&config.inactiveBackground,                        SetColValue},
    {"InactiveForeground",                  (void*)&config.inactiveForeground,                        SetColValue},
    {"UrgentBackground",                    (void*)&config.urgentBackground,                          SetColValue},
    {"UrgentForeground",                    (void*)&config.urgentForeground,                          SetColValue},
    {"ActiveTileBackground",                (void*)&config.activeTileBackground,                      SetColValue},
    {"InactiveTileBackground",              (void*)&config.inactiveTileBackground,                    SetColValue},
    {"CloseIcon",                           (void*)&config.buttonStyles[0].icon,                      SetStrValue},
    {"CloseActiveBackground",               (void*)&config.buttonStyles[0].activeBackground,          SetColValue},
    {"CloseActiveForeground",               (void*)&config.buttonStyles[0].activeForeground,          SetColValue},
    {"CloseInactiveBackground",             (void*)&config.buttonStyles[0].inactiveBackground,        SetColValue},
    {"CloseInactiveForeground",             (void*)&config.buttonStyles[0].inactiveForeground,        SetColValue},
    {"CloseActiveHoveredBackground",        (void*)&config.buttonStyles[0].activeHoveredBackground,   SetColValue},
    {"CloseActiveHoveredForeground",        (void*)&config.buttonStyles[0].activeHoveredForeground,   SetColValue},
    {"CloseInactiveHoveredBackground",      (void*)&config.buttonStyles[0].inactiveHoveredBackground, SetColValue},
    {"CloseInactiveHoveredForeground",      (void*)&config.buttonStyles[0].inactiveHoveredForeground, SetColValue},
    {"MaximizeIcon",                        (void*)&config.buttonStyles[1].icon,                      SetStrValue},
    {"MaximizeActiveBackground",            (void*)&config.buttonStyles[1].activeBackground,          SetColValue},
    {"MaximizeActiveForeground",            (void*)&config.buttonStyles[1].activeForeground,          SetColValue},
    {"MaximizeInactiveBackground",          (void*)&config.buttonStyles[1].inactiveBackground,        SetColValue},
    {"MaximizeInactiveForeground",          (void*)&config.buttonStyles[1].inactiveForeground,        SetColValue},
    {"MaximizeActiveHoveredBackground",     (void*)&config.buttonStyles[1].activeHoveredBackground,   SetColValue},
    {"MaximizeActiveHoveredForeground",     (void*)&config.buttonStyles[1].activeHoveredForeground,   SetColValue},
    {"MaximizeInactiveHoveredBackground",   (void*)&config.buttonStyles[1].inactiveHoveredBackground, SetColValue},
    {"MaximizeInactiveHoveredForeground",   (void*)&config.buttonStyles[1].inactiveHoveredForeground, SetColValue},
    {"MinimizeIcon",                        (void*)&config.buttonStyles[2].icon,                      SetStrValue},
    {"MinimizeActiveBackground",            (void*)&config.buttonStyles[2].activeBackground,          SetColValue},
    {"MinimizeActiveForeground",            (void*)&config.buttonStyles[2].activeForeground,          SetColValue},
    {"MinimizeInactiveBackground",          (void*)&config.buttonStyles[2].inactiveBackground,        SetColValue},
    {"MinimizeInactiveForeground",          (void*)&config.buttonStyles[2].inactiveForeground,        SetColValue},
    {"MinimizeActiveHoveredBackground",     (void*)&config.buttonStyles[2].activeHoveredBackground,   SetColValue},
    {"MinimizeActiveHoveredForeground",     (void*)&config.buttonStyles[2].activeHoveredForeground,   SetColValue},
    {"MinimizeInactiveHoveredBackground",   (void*)&config.buttonStyles[2].inactiveHoveredBackground, SetColValue},
    {"MinimizeInactiveHoveredForeground",   (void*)&config.buttonStyles[2].inactiveHoveredForeground, SetColValue},
    {"Masters",                             (void*)&config.masters,                                   SetIntValue},
    {"Split",                               (void*)&config.split,                                     SetFloatValue},
    {"DecorateTiles",                       (void*)&config.decorateTiles,                             SetIntValue}
};

void
SplitLine(char *str, char **key, char **val) {
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
SetIntValue(const char *val, void *to) {
    *(int*)to = atoi(val);
}

void
SetFloatValue(const char *val, void *to) {
    *(int*)to = atof(val);
}
void
SetStrValue(const char *val, void *to) {
    strcpy(to, val);
}

void
SetColValue(const char *val, void *to) {
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
