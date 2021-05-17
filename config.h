#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef NDEBUG
#define Modkey Mod4Mask
#else
#define Modkey Mod1Mask
#endif

#define ShortcutCount 31

typedef struct _Shortcut {
    unsigned long modifier;
    unsigned long keysym;
    enum {CV, CI} type;
    union {
        struct { void (*f)(); } vcb;
        struct { void (*f)(int); int i; } icb;
    } cb;
} Shortcut;

typedef struct _Config {
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
    int activeButtonBackground;
    int activeButtonForeground;
    int inactiveButtonBackground;
    int inactiveButtonForeground;
    int activeButtonHoveredBackground;
    int activeButtonHoveredForeground;
    int inactiveButtonHoveredBackground;
    int inactiveButtonHoveredForeground;
    char *buttonIcons[3]; /* close, maximize, minimize */
    char *labelFontname;
    char *iconFontname;
    char *terminal[2];
    Shortcut shortcuts[ShortcutCount];
} Config;

extern Config stConfig;

void LoadConfig();

#endif
