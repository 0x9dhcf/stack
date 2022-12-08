#ifndef __SETTINGS_H__
#define __SETTINGS_H__


#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#ifndef VERSION
    #define VERSION "0.0.0"
#endif

// TODO: make it configurable
#define Modkey Mod1Mask
#define ModkeySym XK_Alt_L

#define Max(v1, v2) (v1 > v2 ? v1 : v2)
#define Min(v1, v2) (v1 < v2 ? v1 : v2)

typedef struct Monitor Monitor;
typedef struct Client Client;

enum {
    ButtonRectangle,
    ButtonCirle
};

#define ShortcutCount 52
typedef struct Settings {
    /* style */
    char labelFontname[128];
    char iconFontname[128];
    int borderWidth;
    int topbarHeight;
    int handleWidth;
    int buttonSize;
    int buttonGap;
    int buttonShape;
    int activeTileBackground;
    int inactiveTileBackground;
    int activeBackground;
    int activeForeground;
    int activeBorder;
    int inactiveBackground;
    int inactiveForeground;
    int inactiveBorder;
    int urgentBackground;
    int urgentForeground;
    int urgentBorder;
    struct {
        char icon[8];
        int activeBackground;
        int activeForeground;
        int activeBorder;
        int inactiveBackground;
        int inactiveForeground;
        int inactiveBorder;
        int activeHoveredBackground;
        int activeHoveredForeground;
        int activeHoveredBorder;
        int inactiveHoveredBackground;
        int inactiveHoveredForeground;
        int inactiveHoveredBorder;
    } buttonStyles[3];
    /* global */
    int focusFollowsPointer;
    /* dynamic */
    int decorateTiles;
    int masters;
    float split;
    /* shortcuts */
    struct Shortcut {
        unsigned long modifier;
        unsigned long keysym;
        enum {CV, CC, CCI, CM, CMC, CMI} type;
        union {
            struct { void (*f)(); } vcb;
            struct { void (*f)(Client *); } ccb;
            struct { void (*f)(Client *, int); int i; } cicb;
            struct { void (*f)(Monitor *); } mcb;
            struct { void (*f)(Monitor *, Client *); } mccb;
            struct { void (*f)(Monitor *, int); int i; } micb;
        } cb;
    } shortcuts[ShortcutCount];
} Settings;

extern Settings settings;

void LoadConfigFile();
void ExecAutostartFile();

#endif /* __CONFIG_H___ */
