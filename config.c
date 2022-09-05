#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "client.h"
#include "config.h"
#include "log.h"
#include "stack.h"

static void SplitLine(char *str, char **key, char **val);
static void SetIntValue(const char *val, void *to);
static void SetStrValue(const char *val, void *to);
static void SetColValue(const char *val, void *to);

char stConfigFile[256];

Config stConfig = {
    /* Globals */
    .labelFontname  = "Sans-10",
    .iconFontname   = "Sans-10",

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
            //.icon                       = "\ue5cd",
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
            //.icon                       = "\ue835",
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
            //.icon                       = "\ue931",
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
    .masters    = 1,
    .split      = .6,

    .shortcuts = {
        { Modkey | ShiftMask,     XK_q,         CV,     { .vcb={Stop} } },
        { Modkey | ShiftMask,     XK_r,         CV,     { .vcb={Reload} } },
        { Modkey | ShiftMask,     XK_k,         CC,     { .ccb={KillClient} } },
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

static struct {
    char *key;
    void *to;
    void (*set)(const char *, void *);
} callbacks[] = {
    {"LabelFont",                           (void*)&stConfig.labelFontname,                             SetStrValue},
    {"IconFont",                            (void*)&stConfig.iconFontname,                              SetStrValue},
    {"BorderWidth",                         (void*)&stConfig.borderWidth,                               SetIntValue},
    {"TopbarHeight",                        (void*)&stConfig.topbarHeight,                              SetIntValue},
    {"HandleWidth",                         (void*)&stConfig.handleWidth,                               SetIntValue},
    {"ButtonSize",                          (void*)&stConfig.buttonSize,                                SetIntValue},
    {"ButtonGap",                           (void*)&stConfig.buttonGap,                                 SetIntValue},
    {"ActiveBackground",                    (void*)&stConfig.activeBackground,                          SetColValue},
    {"ActiveForeground",                    (void*)&stConfig.activeForeground,                          SetColValue},
    {"InactiveBackground",                  (void*)&stConfig.inactiveBackground,                        SetColValue},
    {"InactiveForeground",                  (void*)&stConfig.inactiveForeground,                        SetColValue},
    {"UrgentBackground",                    (void*)&stConfig.urgentBackground,                          SetColValue},
    {"UrgentForeground",                    (void*)&stConfig.urgentForeground,                          SetColValue},
    {"ActiveTileBackground",                (void*)&stConfig.activeTileBackground,                      SetColValue},
    {"InactiveTileBackground",              (void*)&stConfig.inactiveTileBackground,                    SetColValue},
    {"CloseIcon",                           (void*)&stConfig.buttonStyles[0].icon,                      SetStrValue},
    {"CloseActiveBackground",               (void*)&stConfig.buttonStyles[0].activeBackground,          SetColValue},
    {"CloseActiveForeground",               (void*)&stConfig.buttonStyles[0].activeForeground,          SetColValue},
    {"CloseInactiveBackground",             (void*)&stConfig.buttonStyles[0].inactiveBackground,        SetColValue},
    {"CloseInactiveForeground",             (void*)&stConfig.buttonStyles[0].inactiveForeground,        SetColValue},
    {"CloseActiveHoveredBackground",        (void*)&stConfig.buttonStyles[0].activeHoveredBackground,   SetColValue},
    {"CloseActiveHoveredForeground",        (void*)&stConfig.buttonStyles[0].activeHoveredForeground,   SetColValue},
    {"CloseInactiveHoveredBackground",      (void*)&stConfig.buttonStyles[0].inactiveHoveredBackground, SetColValue},
    {"CloseInactiveHoveredForeground",      (void*)&stConfig.buttonStyles[0].inactiveHoveredForeground, SetColValue},
    {"MaximizeIcon",                        (void*)&stConfig.buttonStyles[1].icon,                      SetStrValue},
    {"MaximizeActiveBackground",            (void*)&stConfig.buttonStyles[1].activeBackground,          SetColValue},
    {"MaximizeActiveForeground",            (void*)&stConfig.buttonStyles[1].activeForeground,          SetColValue},
    {"MaximizeInactiveBackground",          (void*)&stConfig.buttonStyles[1].inactiveBackground,        SetColValue},
    {"MaximizeInactiveForeground",          (void*)&stConfig.buttonStyles[1].inactiveForeground,        SetColValue},
    {"MaximizeActiveHoveredBackground",     (void*)&stConfig.buttonStyles[1].activeHoveredBackground,   SetColValue},
    {"MaximizeActiveHoveredForeground",     (void*)&stConfig.buttonStyles[1].activeHoveredForeground,   SetColValue},
    {"MaximizeInactiveHoveredBackground",   (void*)&stConfig.buttonStyles[1].inactiveHoveredBackground, SetColValue},
    {"MaximizeInactiveHoveredForeground",   (void*)&stConfig.buttonStyles[1].inactiveHoveredForeground, SetColValue},
    {"MinimizeIcon",                        (void*)&stConfig.buttonStyles[2].icon,                      SetStrValue},
    {"MinimizeActiveBackground",            (void*)&stConfig.buttonStyles[2].activeBackground,          SetColValue},
    {"MinimizeActiveForeground",            (void*)&stConfig.buttonStyles[2].activeForeground,          SetColValue},
    {"MinimizeInactiveBackground",          (void*)&stConfig.buttonStyles[2].inactiveBackground,        SetColValue},
    {"MinimizeInactiveForeground",          (void*)&stConfig.buttonStyles[2].inactiveForeground,        SetColValue},
    {"MinimizeActiveHoveredBackground",     (void*)&stConfig.buttonStyles[2].activeHoveredBackground,   SetColValue},
    {"MinimizeActiveHoveredForeground",     (void*)&stConfig.buttonStyles[2].activeHoveredForeground,   SetColValue},
    {"MinimizeInactiveHoveredBackground",   (void*)&stConfig.buttonStyles[2].inactiveHoveredBackground, SetColValue},
    {"MinimizeInactiveHoveredForeground",   (void*)&stConfig.buttonStyles[2].inactiveHoveredForeground, SetColValue}
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
SetStrValue(const char *val, void *to) {
    strcpy(to, val);
}

void
SetColValue(const char *val, void *to) {
    *(int*)to = (int)strtol(val, NULL, 16);
}

void
FindConfigFile()
{
    char *home;
    char *format =
        "./stack.conf:"
        "%s/.stack.conf:"
        "%s/.config/stack/stack.conf:"
        "/usr/local/etc/stack.conf:"
        "/etc/stack.conf";
    char *paths = NULL;
    char *token = NULL;

    stConfigFile[0] = '\0';

    paths = malloc(1024);
    if (!paths) {
        ELog("Can't allocate config path.");
        return;
    }

    if (!(home = getenv("HOME"))) {
        ELog("Can't get HOME directory.");
        return;
    }

    if (sprintf(paths, format, home, home) < 0) {
         ELog("Can't format path.");
        return;
    }

    token = strtok(paths, ":");
    while(token) {
        if (access(token, F_OK) == 0) {
            strcpy(stConfigFile, token);
            break;
        }
        token = strtok(NULL, ":");
    }
    free(paths);
}

void
LoadConfigFile()
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    FILE *stream;

    if (stConfigFile[0] == '\0') {
        ILog("No configuration file defined.");
        return;
    }

    ILog("Using %s configuration file", stConfigFile);

    stream = fopen(stConfigFile, "r");
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
