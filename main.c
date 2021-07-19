#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>

#include "config.h"
#include "log.h"
#include "monitor.h"
#include "stack.h"
#include "utils.h"
#include "x11.h"

static void TrapSignal(int sig);

void
TrapSignal(int sig)
{
    (void)sig;
    Stop();
}

int
main(int argc, char **argv)
{
    if (argc == 2 && !strcmp("-v", argv[1])) {
        printf("stack-%s\n", VERSION);
        exit(0);
    } else if (argc != 1) {
        fprintf(stderr, "usage: stack [-v]\n");
        exit(2);
    }

    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		ELog("no locale support");

    signal(SIGINT, TrapSignal);
    signal(SIGKILL, TrapSignal);
    signal(SIGTERM, TrapSignal);

    InitializeX11();
    InitializeMonitors();
    Start();
    TeardownMonitors();
    TeardownX11();

    ILog("Bye...");

    return 0;
}
