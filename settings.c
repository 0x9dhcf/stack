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
static void SetBoolValue(const char *val, void *to);
static void SetStrValue(const char *val, void *to);
static void SetColValue(const char *val, void *to);
static void SetShapeValue(const char *val, void *to);
static void SetPlacementValue(const char *val, void *to);

/* default settings */
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
    .buttonShape    = ButtonSquare,
    .activeTileBackground               = 0x005577,
    .inactiveTileBackground             = 0x444444,
    .activeBackground                   = 0xE0E0E0,
    .activeForeground                   = 0x202020,
    .activeBorder                       = 0xE0E0E0,
    .inactiveBackground                 = 0xA0A0A0,
    .inactiveForeground                 = 0x202020,
    .inactiveBorder                     = 0xA0A0A0,
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
    .snapping = 20,
    .placement = StrategyNone,
    /* dynamic desktops */
    .focusFollowsPointer    = False,
    .decorateTiles          = True,
    .masters                = 1,
    .split                  = .6,
    .shortcuts = {
        /* manager */
        { ModCtrlShift, XK_q,           VCB,    { .v_cb={Quit} } },
        { ModCtrlShift, XK_r,           VCB,    { .v_cb={Reload} } },
        { ModCtrlShift, XK_x,           CCB,    { .c_cb={KillClient} } },
        { Mod,          XK_period,      VCB,    { .v_cb={FocusNextMonitor} } },
        { Mod,          XK_comma,       VCB,    { .v_cb={FocusPreviousMonitor} } },
        { Mod,          XK_Tab,         VCB,    { .v_cb={FocusNextClient} } },
        { Mod,          XK_l,           VCB,    { .v_cb={FocusNextClient} } },
        { ModShift,     XK_Tab,         VCB,    { .v_cb={FocusPreviousClient} } },
        { Mod,          XK_h,           VCB,    { .v_cb={FocusPreviousClient} } },
        /* active monitor */
        { Mod,          XK_1,           MICB,   { .mi_cb={ShowMonitorDesktop, 0} } },
        { Mod,          XK_2,           MICB,   { .mi_cb={ShowMonitorDesktop, 1} } },
        { Mod,          XK_3,           MICB,   { .mi_cb={ShowMonitorDesktop, 2} } },
        { Mod,          XK_4,           MICB,   { .mi_cb={ShowMonitorDesktop, 3} } },
        { Mod,          XK_5,           MICB,   { .mi_cb={ShowMonitorDesktop, 4} } },
        { Mod,          XK_6,           MICB,   { .mi_cb={ShowMonitorDesktop, 5} } },
        { Mod,          XK_7,           MICB,   { .mi_cb={ShowMonitorDesktop, 6} } },
        { Mod,          XK_8,           MICB,   { .mi_cb={ShowMonitorDesktop, 7} } },
        { Mod,          XK_Page_Down,   MCB,    { .m_cb={ShowNextMonitorDesktop} } },
        { Mod,          XK_Page_Up,     MCB,    { .m_cb={ShowPreviousMonitorDesktop} } },
        { Mod,          XK_d,           MDCB,   { .md_cb={ToggleMonitorDesktopDynamic} } },
        { Mod,          XK_equal,       MDCB,   { .md_cb={IncreaseMonitorDesktopMasterCount } } },
        { Mod,          XK_minus,       MDCB,   { .md_cb={DecreaseMonitorDesktopMasterCount} } },
        { ModShift,     XK_d,           MCB,    { .m_cb={ToggleMonitorDynamic} } },
        { ModShift,     XK_equal,       MCB,    { .m_cb={IncreaseMonitorMasterCount } } },
        { ModShift,     XK_minus,       MCB,    { .m_cb={DecreaseMonitorMasterCount} } },
        { ModShift,     XK_t,           MDCB,   { .md_cb={ToggleMonitorDesktopTopbar} } },
        { ModCtrlShift, XK_t,           MCB,    { .m_cb={ToggleMonitorTopbar} } },
        { ModShift,     XK_h,           MCCB,   { .mc_cb={StackClientUp} } },
        { ModShift,     XK_l,           MCCB,   { .mc_cb={StackClientDown} } },
        /* active client */
        { Mod,          XK_t,           CCB,    { .c_cb={ToggleClientTopbar} } },
        { ModCtrlShift, XK_minus,       CCB,    { .c_cb={MaximizeClientHorizontally} } },
        { ModCtrlShift, XK_backslash,   CCB,    { .c_cb={MaximizeClientVertically} } },
        { Mod,          XK_Return,      CCB,    { .c_cb={MaximizeClient} } },
        { ModCtrlShift, XK_h,           CCB,    { .c_cb={MaximizeClientLeft} } },
        { ModCtrlShift, XK_l,           CCB,    { .c_cb={MaximizeClientRight} } },
        { ModCtrlShift, XK_k,           CCB,    { .c_cb={MaximizeClientTop} } },
        { ModCtrlShift, XK_j,           CCB,    { .c_cb={MaximizeClientBottom} } },
        { Mod,          XK_Delete,      CCB,    { .c_cb={MinimizeClient} } },
        { ModCtrl,      XK_h,           CCB,    { .c_cb={MoveClientLeftmost} } },
        { ModCtrl,      XK_k,           CCB,    { .c_cb={MoveClientTopmost} } },
        { ModCtrl,      XK_l,           CCB,    { .c_cb={MoveClientRightmost} } },
        { ModCtrl,      XK_j,           CCB,    { .c_cb={MoveClientBottommost} } },
        { ModCtrl,      XK_c,           CCB,    { .c_cb={CenterClient} } },
        { ModShift,     XK_f,           CCB,    { .c_cb={FullscreenClient} } },
        { ModShift,     XK_Return,      CCB,    { .c_cb={RestoreClient} } },
        { ModShift,     XK_1,           CICB,   { .ci_cb={MoveClientToDesktop, 0} } },
        { ModShift,     XK_2,           CICB,   { .ci_cb={MoveClientToDesktop, 1} } },
        { ModShift,     XK_3,           CICB,   { .ci_cb={MoveClientToDesktop, 2} } },
        { ModShift,     XK_4,           CICB,   { .ci_cb={MoveClientToDesktop, 3} } },
        { ModShift,     XK_5,           CICB,   { .ci_cb={MoveClientToDesktop, 4} } },
        { ModShift,     XK_6,           CICB,   { .ci_cb={MoveClientToDesktop, 5} } },
        { ModShift,     XK_7,           CICB,   { .ci_cb={MoveClientToDesktop, 6} } },
        { ModShift,     XK_8,           CICB,   { .ci_cb={MoveClientToDesktop, 7} } },
        { ModShift,     XK_Page_Down,   CCB,    { .c_cb={MoveClientToNextDesktop } } },
        { ModShift,     XK_Page_Up,     CCB,    { .c_cb={MoveClientToPreviousDesktop } } },
        { ModShift,     XK_period,      CCB,    { .c_cb={MoveClientToNextMonitor} } },
        { ModShift,     XK_comma,       CCB,    { .c_cb={MoveClientToPreviousMonitor} } }
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
    {"ButtonShape",                         (void*)&settings.buttonShape,                               SetShapeValue},
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
    {"Snapping",                            (void*)&settings.snapping,                                  SetIntValue},
    {"Placement",                           (void*)&settings.placement,                                 SetPlacementValue},
    {"FocusFollowsPointer",                 (void*)&settings.focusFollowsPointer,                       SetBoolValue},
    {"DecorateTiles",                       (void*)&settings.decorateTiles,                             SetBoolValue},
    {"Masters",                             (void*)&settings.masters,                                   SetIntValue},
    {"Split",                               (void*)&settings.split,                                     SetFloatValue}
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
SetBoolValue(const char *val, void *to)
{
    *(Bool*)to = False;
    if (! strcmp(val, "1")
            && ! strcasecmp(val, "true")
            && ! strcasecmp(val, "yes"))
        *(Bool*)to = True;
}
void
SetShapeValue(const char *val, void *to)
{
    *(int*)to = ButtonSquare; 
    if (! strcasecmp(val, "round"))
        *(int*)to = ButtonRound; 
}

void
SetPlacementValue(const char *val, void *to)
{
    *(int*)to = StrategyNone; 
    if (! strcasecmp(val, "center"))
        *(int*)to = StrategyCenter; 
    else if (! strcasecmp(val, "pointer"))
        *(int*)to = StrategyPointer; 
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
        "/etc/%s:"
        "%s/.local/share/stack/%s:"
        "/usr/local/share/stack/%s:"
        "/usr/share/stack/%s";
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

    if (sprintf(paths, format, name, home, name, home, name,
                name, name, home, name, name, name) < 0) {
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
    char configFile[1024];
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
        ILog("executing: %s", rcFile);
        system(cmd);
    } else {
        ELog("rc file is not executable.");
    }
}
