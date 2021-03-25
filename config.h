#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct _Client Client;

typedef enum _CallbackType {
    CallbackVoid,
    CallbackClient
} CallbackType;

typedef struct _Shortcut {
    unsigned long modifier;
    unsigned long keysym;
    CallbackType type;
    union {
        void (*vcb)();
        void (*ccb)(Client *);
    } callback;
} Shortcut;

enum ClientShortcutNames {
    ShortcutQuit,
    ShortcutVMaximize,
    ShortcutHMaximize,
    ShortcutFMaximize,
    //ShortcutMinimize, No minimize for now
    //ShortcutFullscreen,
    ShortcutRestore,
    ShortcutCycleForward,
    ShortcutCycleBackward,
    ShortcutCount
};

typedef struct _Config {
    int border_width;
    int topbar_height;
    int handle_width;
    int button_size;
    int button_gap;
    int active_background;
    int active_foreground;
    int inactive_background;
    int inactive_foreground;
    int urgent_background;
    int urgent_foreground;
    int active_button_background;
    int active_button_foreground;
    int inactive_button_background;
    int inactive_button_foreground;
    int active_button_hovered_background;
    int active_button_hovered_foreground;
    int inactive_button_hovered_background;
    int inactive_button_hovered_foreground;
    char *button_icons[2]; /* close, maximize, minimize */
    char *label_fontname;
    char *icon_fontname;

    Shortcut shortcuts[ShortcutCount];

    char *terminal[2];
} Config;

#endif
