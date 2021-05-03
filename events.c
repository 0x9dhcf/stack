#include <X11/XKBlib.h>

#include "atoms.h"
#include "client.h"
#include "config.h"
#include "cursors.h"
#include "events.h"
#include "font.h"
#include "hints.h"
#include "manage.h"
#include "stack.h"
#include "log.h"
#include "x11.h"

static int  lastSeenPointerX;
static int  lastSeenPointerY;
static Time lastSeenPointerTime;
static int  motionStartX;
static int  motionStartY;
static int  motionStartW;
static int  motionStartH;

static void (*callbacks[ShortcutCount])() = {
    [ShortcutQuit]                  = Quit,
    [ShortcutMaximizeVertically]    = MaximizeVertically,
    [ShortcutMaximizeHorizontally]  = MaximizeHorizontally,
    [ShortcutMaximizeLeft]          = MaximizeLeft,
    [ShortcutMaximizeRight]         = MaximizeRight,
    [ShortcutMaximizeTop]           = MaximizeTop,
    [ShortcutMaximizeBottom]        = MaximizeBottom,
    [ShortcutMaximize]              = Maximize,
    [ShortcutRestore]               = Restore,
    [ShortcutCycleForward]          = CycleForward,
    [ShortcutCycleBackward]         = CycleBackward
};

static void OnConfigureRequest(XConfigureRequestEvent *e);
static void OnMapRequest(XMapRequestEvent *e);
static void OnUnmapNotify(XUnmapEvent *e);
static void OnDestroyNotify(XDestroyWindowEvent *e);
static void OnExpose(XExposeEvent *e);
static void OnEnter(XCrossingEvent *e);
static void OnLeave(XCrossingEvent *e);
static void OnPropertyNotify(XPropertyEvent *e);
static void OnButtonPress(XButtonEvent *e);
static void OnButtonRelease(XButtonEvent *e);
static void OnMotionNotify(XMotionEvent *e);
static void OnMessage(XClientMessageEvent *e);
static void OnKeyPress(XKeyPressedEvent *e);

void
DispatchEvent(XEvent *e)
{
    switch(e->type) {
        case MapRequest:
            OnMapRequest(&e->xmaprequest);
        break;
        case UnmapNotify:
            OnUnmapNotify(&e->xunmap);
        break;
        case DestroyNotify:
            OnDestroyNotify(&e->xdestroywindow);
        break;
        case ConfigureRequest:
            OnConfigureRequest(&e->xconfigurerequest);
        break;
        case Expose:
            OnExpose(&e->xexpose);
        break;
        case PropertyNotify:
            OnPropertyNotify(&e->xproperty);
        break;
        case ButtonPress:
            OnButtonPress(&e->xbutton);
        break;
        case ButtonRelease:
            OnButtonRelease(&e->xbutton);
        break;
        case MotionNotify:
            OnMotionNotify(&e->xmotion);
        break;
        case EnterNotify:
            OnEnter(&e->xcrossing);
        break;
        case LeaveNotify:
            OnLeave(&e->xcrossing);
        break;
        case ClientMessage:
            OnMessage(&e->xclient);
        break;
        case KeyPress:
            OnKeyPress(&e->xkey);
        break;
    }
}

void
OnConfigureRequest(XConfigureRequestEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);

    if (!c) {
        XWindowChanges xwc;
        xwc.x = e->x;
        xwc.y = e->y;
        xwc.width = e->width;
        xwc.height = e->height;
        xwc.border_width = e->border_width;
        xwc.sibling = e->above;
        xwc.stack_mode = e->detail;
        XConfigureWindow(stDisplay, e->window, e->value_mask, &xwc);
        XSync(stDisplay, False);
    } else {
        Bool notify = False;

        if (e->value_mask & CWBorderWidth)
            c->sbw = e->border_width;

        if (e->value_mask & (CWX|CWY|CWWidth|CWHeight)
                && (c->states & StateMaximized) != StateMaximized)
                {
            int x, y, w, h;

            x = c->wx;
            y = c->wy;
            w = c->ww;
            h = c->wh;

            if (e->value_mask & CWX)
                x = e->x;
            if (e->value_mask & CWY)
                y = e->y;
            if (e->value_mask & CWWidth)
                w = e->width;
            if (e->value_mask & CWHeight)
                h = e->height;

            MoveResizeClientWindow(c, x, y, w, h, False);
            notify = True;
        }

        /* As the window is reparented, XRaiseWindow and XLowerWindow
           will generate a ConfigureRequest. */
        if (e->value_mask & CWStackMode) {
            if (e->detail == Above || e->detail == TopIf)
                RaiseClient(c, False);
            if (e->detail == Below || e->detail == BottomIf)
                LowerClient(c, False);
            notify = True;
        }

        if (notify) {
            XConfigureEvent xce;
            xce.event = e->window;
            xce.window = e->window;
            xce.type = ConfigureNotify;
            xce.x = c->wx;
            xce.y = c->wy;
            xce.width = c->ww;
            xce.height = c->wh;
            xce.border_width = 0;
            xce.above = None;
            xce.override_redirect = 0;
            XSendEvent(stDisplay, e->window, False, SubstructureNotifyMask, (XEvent*)&e);
            XSync(stDisplay, False);
        }
    }
}

void
OnMapRequest(XMapRequestEvent *e)
{
    DLog();
    ManageWindow(e->window, False);
}

void
OnUnmapNotify(XUnmapEvent *e)
{
    DLog();
    /* ignore UnmapNotify for reparented pre-existing window */
    if (e->event == stRoot || e->event == None) {
        DLog("Reparenting forget that!");
        return;
    }
    ForgetWindow(e->window);
}

void
OnDestroyNotify(XDestroyWindowEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);
    if (c) { ELog("Found a client"); }
}

void
OnExpose(XExposeEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);
    if (c && e->window == c->frame)
        RefreshClient(c);
}

// TODO: move update to hints and use setter
void
OnPropertyNotify(XPropertyEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);
    if (!c)
        return;

    if (!c || e->window != c->window)
        return;

    if (e->atom == XA_WM_NAME || e->atom == stAtoms[AtomNetWMName])
        UpdateClientName(c);

    if (e->atom == XA_WM_HINTS)
        UpdateClientHints(c);

    if (e->atom == stAtoms[AtomNetWMState])
        UpdateClientState(c);
}

void
OnButtonPress(XButtonEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);
    if (!c)
        return;

    lastSeenPointerX = e->x_root;
    lastSeenPointerY = e->y_root;
    motionStartX = c->fx;
    motionStartY = c->fy;
    motionStartW = c->fw;
    motionStartH = c->fh;
    
    if (e->window == c->topbar || (e->window == c->window && e->state == Modkey))
        XDefineCursor(stDisplay, e->window, stCursors[CursorMove]);

    if (e->window != c->buttons[ButtonClose])
        SetActiveClient(c);

    if (e->window == c->buttons[ButtonMaximize]) {
        if ((c->states & StateMaximized) == StateMaximized)
            RestoreClient(c, True);
        else
            MaximizeClient(c, StateMaximized, True);
    }

    /* TODO: when we know how to restore
    if (e->window == c->buttons[ButtonMinimize]) {
        MinimizeClient(c, True);
    }
    */

    if (e->window == c->buttons[ButtonClose]) {
        if (c->states & StateDeleteWindow)
            SendMessage(c->window, stAtoms[AtomWMDeleteWindow]);
        else
            XKillClient(stDisplay, c->window);
    }

    //XAllowEvents(stDisplay, ReplayPointer, CurrentTime);
    if (e->window == c->window)
        XAllowEvents(stDisplay, ReplayPointer, e->time);
}

void
OnButtonRelease(XButtonEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);
    if (!c)
        return;

    for (int i = 0; i < HandleCount; ++i) {
        if (e->window == c->handles[i]) {
            /* apply the size hints */
            MoveResizeClientFrame(c, c->fx, c->fy, c->fw, c->fh, True);
            break;
        }
    }

    if (e->window == c->topbar || e->window == c->window)
        XDefineCursor(stDisplay, e->window, stCursors[CursorNormal]);

    if (e->window == c->window)
        XAllowEvents(stDisplay, ReplayPointer, e->time);
}

void
OnMotionNotify(XMotionEvent *e)
{
    Client *c = Lookup(e->window);
    if (!c)
        return;

    int vx = e->x_root - lastSeenPointerX;
    int vy = e->y_root - lastSeenPointerY;

    if ((e->time - lastSeenPointerTime) <= (1000 / 60))
        return;
    lastSeenPointerTime = e->time;

    /* we do not apply normal hints during motion but when button is released
     * to make the resizing visually smoother. Some client apply normals by
     * themselves anway (e.g gnome-terminal) */
    if (e->window == c->topbar || e->window == c->window)
        MoveClientFrame(c, motionStartX + vx, motionStartY + vy);
    else if (e->window == c->handles[HandleNorth])
        MoveResizeClientFrame(c, motionStartX, motionStartY + vy,
                motionStartW, motionStartH - vy , False);
    else if (e->window == c->handles[HandleWest])
        ResizeClientFrame(c, motionStartW + vx, motionStartH, False);
    else if (e->window == c->handles[HandleSouth])
        ResizeClientFrame(c, motionStartW, motionStartH + vy, False);
    else if (e->window == c->handles[HandleEast])
        MoveResizeClientFrame(c, motionStartX + vx, motionStartY,
                motionStartW - vx, motionStartH, False);
    else if (e->window == c->handles[HandleNorthEast])
        MoveResizeClientFrame(c, motionStartX + vx, motionStartY + vy,
                motionStartW - vx, motionStartH - vy, False);
    else if (e->window == c->handles[HandleNorthWest])
        MoveResizeClientFrame(c, motionStartX, motionStartY + vy,
                motionStartW + vx, motionStartH - vy, False);
    else if (e->window == c->handles[HandleSouthWest])
        ResizeClientFrame(c, motionStartW + vx, motionStartH + vy, False);
    else if (e->window == c->handles[HandleSouthEast])
        MoveResizeClientFrame(c, motionStartX + vx, motionStartY,
                motionStartW - vx, motionStartH + vy, False);
    else
      return;
}

// XXX to be checked
void
OnMessage(XClientMessageEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);
    if (!c)
        return;

    if (e->message_type == stAtoms[AtomNetWMState]) {
        if (e->data.l[1] == (long)stAtoms[AtomNetWMStateFullscreen]
                || e->data.l[2] == (long)stAtoms[AtomNetWMStateFullscreen]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                FullscreenClient(c, True);
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                FullscreenClient(c, True);
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & StateFullscreen)
                    RestoreClient(c, True);
                else
                    FullscreenClient(c, True);
            }
        }
        if (e->data.l[1] == (long)stAtoms[AtomNetWMStateDemandsAttention]
                || e->data.l[2] == (long)stAtoms[AtomNetWMStateDemandsAttention]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                c->states &= ~StateUrgent;
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                c->states |= StateUrgent;
            if (e->data.l[0] == 2) { /* _NET_WM_STATE_TOGGLE */
                if (c->states & StateUrgent)
                    c->states &= ~StateUrgent;
                else
                    c->states |= StateUrgent;
            }
            RefreshClient(c);
        }
    }

    // XXX: ???
    if (e->message_type == stAtoms[AtomNetActiveWindow])
        XSetInputFocus(stDisplay, c->window, RevertToPointerRoot, CurrentTime);
}



void
OnEnter(XCrossingEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);

    if (c) {
        /* redraw button if hovered */
        int bg, fg;
        if (c->states & StateActive) {
            bg = stConfig.activeButtonHoveredBackground;
            fg = stConfig.activeButtonHoveredForeground;
        } else {
            bg = stConfig.inactiveButtonHoveredBackground;
            fg = stConfig.inactiveButtonHoveredForeground;
        }

        for (int i = 0; i < ButtonCount; ++i) {
            if (e->window == c->buttons[i]) {
                int x, y;
                XSetWindowBackground(stDisplay, c->buttons[i], bg);
                XClearWindow(stDisplay, c->buttons[i]);
                GetTextPosition(stConfig.buttonIcons[i], stIconFont,
                        AlignCenter, AlignMiddle, stConfig.buttonSize,
                        stConfig.buttonSize, &x, &y);
                WriteText(c->buttons[i], stConfig.buttonIcons[i], stIconFont, fg,
                        x, y);
            }
        }
    }
}

void OnLeave(XCrossingEvent *e)
{
    DLog();
    Client *c = Lookup(e->window);

    if (c) {
        /* redraw button if hovered */
        int bg, fg;
        if (c->states & StateActive) {
            bg = stConfig.activeButtonBackground;
            fg = stConfig.activeButtonForeground;
        } else {
            bg = stConfig.inactiveButtonBackground;
            fg = stConfig.inactiveButtonForeground;
        }

        for (int i = 0; i < ButtonCount; ++i) {
            if (e->window == c->buttons[i]) {
                int x, y;
                XSetWindowBackground(stDisplay, c->buttons[i], bg);
                XClearWindow(stDisplay, c->buttons[i]);
                GetTextPosition(stConfig.buttonIcons[i], stIconFont,
                        AlignCenter, AlignMiddle, stConfig.buttonSize,
                        stConfig.buttonSize, &x, &y);
                WriteText(c->buttons[i], stConfig.buttonIcons[i], stIconFont, fg,
                        x, y);
            }
        }
    }
}


void
OnKeyPress(XKeyPressedEvent *e)
{
    DLog();
    KeySym keysym;
    //keysym = XkbKeycodeToKeysym(stDisplay, e->keycode, 0, e->state & ShiftMask ? 1 : 0);
    keysym = XkbKeycodeToKeysym(stDisplay, e->keycode, 0, 0);


    /* TODO manage binding */
    if (keysym == (XK_Return) && CleanMask(Modkey|ShiftMask) == CleanMask(e->state))
        Spawn((char**)stConfig.terminal);

    for (int i = 0; i < ShortcutCount; ++i) {
        if (keysym == (stConfig.shortcuts[i].keysym) &&
                CleanMask(stConfig.shortcuts[i].modifier) == CleanMask(e->state)) {
            callbacks[i]();
            break;
        }
    }
}

