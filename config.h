#ifndef __CONFIG_H__
#define __CONFIG_H__

#define Modkey Mod1Mask
#define DesktopCount 8
#define ShortcutCount 32

typedef struct Client Client;

typedef struct Shortcut {
    unsigned long modifier;
    unsigned long keysym;
    enum {CV, CI, CC} type;
    union {
        struct { void (*f)(); } vcb;
        struct { void (*f)(int); int i; } icb;
        struct { void (*f)(Client *); } ccb;
    } cb;
} Shortcut;

typedef struct Config {
    char *labelFontname;
    char *iconFontname;
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
    } buttonStyles[3];
    int masters;
    float split;
    Shortcut shortcuts[ShortcutCount];
} Config;

extern Config  stConfig;

#endif
