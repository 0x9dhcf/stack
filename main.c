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

#include "event.h"
#include "log.h"
#include "manager.h"
#include "monitor.h"
#include "settings.h"
#include "stack.h"
#include "x11.h"

static void TrapSignal(int sig);

void
TrapSignal(int sig)
{
    (void)sig;
    StopEventLoop();
}

int
main(int argc, char **argv)
{
    if (argc == 2 && !strcmp("-v", argv[1])) {
        printf("stack-" VERSION "\n"
                "Stacking window manager with dynamic tiling capabilities\n\n"
                "Copyright (C) 2021-2022 by 0x9dhcf <0x9dhcf at gmail dot com>\n\n"
                "Permission to use, copy, modify, and/or distribute this software for any purpose\n"
                "with or without fee is hereby granted.\n\n"
                "THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH\n"
                "REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND\n"
                "FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,\n"
                "INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS\n"
                "OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\n"
                "TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF\n"
                "THIS SOFTWARE.\n");
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

    LoadConfigFile();

    SetupX11();
    SetupMonitors();
    SetupWindowManager();
    StartEventLoop();
    CleanupWindowManager();
    CleanupMonitors();
    CleanupX11();

    ILog("Bye...");

    return 0;
}
