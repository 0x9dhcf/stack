#ifndef __CONFIG_H__
#define __CONFIG_H__


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

#define ShortcutCount 52
typedef struct Config {
    /* style */
    char labelFontname[128];
    char iconFontname[128];
    int borderWidth;
    int topbarHeight;
    int handleWidth;
    int buttonSize;
    int buttonGap;
    int activeTileBackground;
    int inactiveTileBackground;
    int activeBackground;
    int activeForeground;
    int inactiveBackground;
    int inactiveForeground;
    int urgentBackground;
    int urgentForeground;
    struct {
        char icon[8];
        int activeBackground;
        int activeForeground;
        int inactiveBackground;
        int inactiveForeground;
        int activeHoveredBackground;
        int activeHoveredForeground;
        int inactiveHoveredBackground;
        int inactiveHoveredForeground;
    } buttonStyles[3];
    /* global */
    int focusFollowsPointer;
    /* dynamic */
    int decorateTiles;
    int masters;
    float split;
    /* shortcuts */
    struct {
        unsigned long modifier;
        unsigned long keysym;
        enum {CV, CC, CCI, CM, CMI} type;
        union {
            struct { void (*f)(); } vcb;
            struct { void (*f)(Client *); } ccb;
            struct { void (*f)(Client *, int); int i; } cicb;
            struct { void (*f)(Monitor *); } mcb;
            struct { void (*f)(Monitor *, int); int i; } micb;
        } cb;
    } shortcuts[ShortcutCount];
} Config;

extern Config config;

void LoadConfigFile();
void ExecAutostartFile();

#endif /* __CONFIG_H___ */
