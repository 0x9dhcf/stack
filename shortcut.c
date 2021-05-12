#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/X.h>

#include "client.h"
#include "hints.h"
#include "log.h"
#include "manage.h"
#include "monitor.h"
#include "stack.h"
#include "x11.h"

void
Spawn(char **args)
{
    if (fork() == 0) {
        if (stDisplay)
              close(ConnectionNumber(stDisplay));
        setsid();
        execvp((char *)args[0], (char **)args);
        ELog("%s: failed", args[0]);
        exit(EXIT_SUCCESS);
    }
}

void
Quit()
{
    stRunning = False;
}

void
MaximizeActiveClientVertically()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientVertically(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientHorizontally()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientHorizontally(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientLeft()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientLeft(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientRight()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientRight(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientTop()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientTop(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClientBottom()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientBottom(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
MaximizeActiveClient()
{
    if (stActiveClient && !(stActiveClient->types & NetWMTypeFixed)) {
        MaximizeClientHorizontally(stActiveClient);
        MaximizeClientVertically(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
RestoreActiveClient()
{
    if (stActiveClient) {
        RestoreClient(stActiveClient);
        SetNetWMState(stActiveClient->window, stActiveClient->states);
    }
}

void
CycleActiveMonitorForward()
{
    if (stActiveClient) {
        Client *nc = NULL;
        for (nc = NextClient(stActiveClient); nc != stActiveClient
                    && (nc->desktop != stActiveClient->desktop
                    || !(nc->types & NetWMTypeNormal)
                    ||  nc->states & NetWMStateHidden);
                nc = NextClient(nc));

        if (nc)
            SetActiveClient(nc);
    }
}

void
CycleActiveMonitorBackward()
{
    if (stActiveClient) {
        Client *pc = NULL;
        for (pc = PreviousClient(stActiveClient);
                pc != stActiveClient
                    && (pc->desktop != stActiveClient->desktop
                    || !(pc->types & NetWMTypeNormal)
                    || pc->states & NetWMStateHidden);
                pc = PreviousClient(pc));
        if (pc)
            SetActiveClient(pc);
    }
}

void
ShowActiveMonitorDesktop(int desktop)
{
    if (desktop >= 0 && desktop < DesktopCount)
        SetActiveDesktop(stActiveMonitor, desktop);
}

