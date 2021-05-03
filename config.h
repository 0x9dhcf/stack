#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef NDEBUG
#define Modkey Mod4Mask
#else
#define Modkey Mod1Mask
#endif

typedef struct _Client Client;

enum Shortcuts {
    ShortcutQuit,
    ShortcutMaximizeVertically,
    ShortcutMaximizeHorizontally,
    ShortcutMaximizeLeft,
    ShortcutMaximizeRight,
    ShortcutMaximizeTop,
    ShortcutMaximizeBottom,
    ShortcutMaximize,
    //ShortcutMinimize, No minimize for now
    //ShortcutFullscreen,
    ShortcutRestore,
    ShortcutCycleForward,
    ShortcutCycleBackward,
    ShortcutCount
};

typedef struct _Shortcut {
    unsigned long modifier;
    unsigned long keysym;
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
    Shortcut shortcuts[ShortcutCount];
    char *terminal[2];
} Config;

extern Config stConfig;

void LoadConfig();

#endif
