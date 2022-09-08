#ifndef __EVENT_H__
#define __EVENT_H__

#include <X11/Xlib.h>

void StartEventLoop();
void StopEventLoop();
int EnableErrorHandler(Display *d, XErrorEvent *e);
int DisableErrorHandler(Display *d, XErrorEvent *e);

#endif /* __EVENT_H__ */
