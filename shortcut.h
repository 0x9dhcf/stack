#ifndef __SHORTCUT_H__
#define __SHORTCUT_H__

typedef struct _Client Client;
typedef struct _Monitor Monitor;

typedef struct _Shortcut {
    unsigned long modifier;
    unsigned long keysym;
    enum {CV, CI} type;
    union {
        struct { void (*f)(); } vcb;
        struct { void (*f)(int); int i; } icb;
    } cb;
} Shortcut;

void Spawn(char **args);
void Quit();
void MaximizeActiveClientVertically();
void MaximizeActiveClientHorizontally();
void MaximizeActiveClientLeft();
void MaximizeActiveClientRight();
void MaximizeActiveClientTop();
void MaximizeActiveClientBottom();
void MaximizeActiveClient();
void RestoreActiveClient();
void CycleActiveMonitorForward();
void CycleActiveMonitorBackward();
void ShowActiveMonitorDesktop(int desktop);

#endif
