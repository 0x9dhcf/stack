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

#define ShortcutCount 39
typedef struct Config {
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
    int decorateTiles;
    int masters;
    float split;
    struct {
        unsigned long modifier;
        unsigned long keysym;
        enum {CV, CI, CC} type;
        union {
            struct { void (*f)(); } vcb;            /* Void callback        */
            struct { void (*f)(int); int i; } icb;  /* Integer callback     */
            struct { void (*f)(Client *); } ccb;    /* Client callback      */
        } cb;
    } shortcuts[ShortcutCount];
} Config;

extern Config config;

void LoadConfigFile();
void ExecAutostartFile();

#endif /* __CONFIG_H___ */
