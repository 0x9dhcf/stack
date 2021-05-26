#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "client.h"

#ifdef NDEBUG
#define Modkey Mod4Mask
#else
#define Modkey Mod1Mask
#endif

/* XXX: Find another way!!! */
#define ShortcutCount 36

//typedef struct _Client Client;

typedef struct _Shortcut {
    unsigned long modifier;
    unsigned long keysym;
    enum {CV, CI, CC} type;
    union {
        struct { void (*f)(); } vcb;
        struct { void (*f)(int); int i; } icb;
        struct { void (*f)(Client *); } ccb;
    } cb;
} Shortcut;

typedef struct _Config {
    // TODO: window style structure
    /* window */
    int borderWidth;
    int topbarHeight;
    int handleWidth;
    int buttonSize;
    int buttonGap;
    int activeBackground;
    int activeForeground;
    int inactiveBackground;
    int inactiveForeground;
    int urgentBackground;
    int urgentForeground;

    struct {
        char *icon;
        int activeBackground;
        int activeForeground;
        int inactiveBackground;
        int inactiveForeground;
        int activeHoveredBackground;
        int activeHoveredForeground;
        int inactiveHoveredBackground;
        int inactiveHoveredForeground;
    } buttonStyles[ButtonCount];

    char *labelFontname;
    char *iconFontname;

    /* dynamic desktop */
    int masters;
    float split;

    Shortcut shortcuts[ShortcutCount];
} Config;

extern Config stConfig;

void LoadConfig();

#endif
