#ifndef __SETTINGS_H__
#define __SETTINGS_H__


#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#define Mod             Mod1Mask
#define ModSym          XK_Alt_L
#define ModShift        Mod | ShiftMask
#define ModCtrl         Mod | ControlMask
#define ModCtrlShift    Mod | ShiftMask | ControlMask
#define ShortcutCount   57

typedef enum ButtonShape ButtonShape;

typedef struct Monitor Monitor;
typedef struct Client Client;

enum ButtonShape {
    ButtonRectangle,
    ButtonCirle
};

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
        enum {VCB, CCB, MCB, CICB, MICB, MDCB, MCCB} type;
        union {
            struct { void (*f)();                       } v_cb;
            struct { void (*f)(Client *);               } c_cb;
            struct { void (*f)(Monitor *);              } m_cb;
            struct { void (*f)(Client *, int); int i;   } ci_cb;
            struct { void (*f)(Monitor *, int); int i;  } mi_cb;
            struct { void (*f)(Monitor *, int);         } md_cb;
            struct { void (*f)(Monitor *, Client*);     } mc_cb;
        } cb;
    } shortcuts[ShortcutCount];
} Settings;

extern Settings settings;

void LoadConfigFile();
void ExecAutostartFile();

#endif /* __CONFIG_H___ */
