#ifndef __X11_H__
#define __X11_H__

#include <X11/Xlib.h>

#define CleanMask(mask)\
    (mask & ~(stNumLockMask|LockMask) &\
         ( ShiftMask\
         | ControlMask\
         | Mod1Mask\
         | Mod2Mask\
         | Mod3Mask\
         | Mod4Mask\
         | Mod5Mask))

extern Display         *stDisplay;
extern int              stScreen;
extern Window           stRoot;
extern unsigned long    stNumLockMask;
extern int              stXRandREventBase;

void InitializeX11();
void TeardownX11();

#endif

