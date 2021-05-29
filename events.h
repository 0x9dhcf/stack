#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <X11/Xlib.h>

#define RootEventMask (\
          SubstructureRedirectMask\
        | SubstructureNotifyMask)\

#define FrameEvenMask (\
          ExposureMask\
        | ButtonPressMask\
        | SubstructureRedirectMask\
        | SubstructureNotifyMask)

#define WindowEventMask (\
          PropertyChangeMask)

#define WindowDoNotPropagateMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask\
        | KeyPressMask\
        | KeyReleaseMask)

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)

void DispatchEvent(XEvent *e);

#endif
