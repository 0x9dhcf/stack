#ifndef __STACK_H__
#define __STACK_H__

#include <X11/Xlib.h>

#define Max(v1, v2) (v1 > v2 ? v1 : v2)
#define Min(v1, v2) (v1 < v2 ? v1 : v2)

typedef struct _Monitor Monitor;
typedef struct _Client Client;

extern Bool     stRunning;
extern Monitor  *stActiveMonitor;
extern Client   *stActiveClient;

#endif
