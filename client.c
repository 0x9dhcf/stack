#include "client.h"
#include "X11/X.h"
#include "X11/Xlib.h"
#include "X11/Xutil.h"

#include "config.h"
#include "font.h"
#include "log.h"
#include "output.h"
#include "stack.h"

#include <string.h>
#include <limits.h>


#define DoNotPropagateMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define HandleEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | ButtonMotionMask)

#define ButtonEventMask (\
          ButtonPressMask\
        | ButtonReleaseMask\
        | EnterWindowMask\
        | LeaveWindowMask)

#define FrameEvenMask (\
          StructureNotifyMask       /* …the frame gets destroyed */                 \
        | SubstructureRedirectMask  /* …the application tries to resize itself */   \
        | SubstructureNotifyMask    /* …subwindows get notifies */                  \
        | ButtonPressMask           /* …mouse is pressed/released */                \
        | ButtonReleaseMask                                                         \
        | ButtonMotionMask          /* …mouse is moved   */                         \
        | ExposureMask              /* …our window needs to be redrawn */           \
        | EnterWindowMask                                                           \
        | LeaveWindowMask)

#define ClientEventMask (\
          PropertyChangeMask\
        | StructureNotifyMask\
        | EnterWindowMask                                                           \
        | LeaveWindowMask)
        //| ButtonPressMask)
        //| FocusChangeMask)

#define DLogClient(cptr) {\
    DLog("window:\t%ld @ (%d, %d) [%d x %d]\n"\
         "frame:\t%ld@ (%d, %d) [%d x %d]\n"\
         "fixed: %d"\
         "decorated: %d" ,\
         cptr->window, cptr->x, cptr->y, cptr->w, cptr->h,\
         cptr->frame, cptr->fx, cptr->fy, cptr->fw, cptr->fh\
         c->fixed, c->decorated); }

static void GrabButtons(Client *c);
static void Configure(Client *c);
static void Decorate(Client *c);
static void ApplyWmNormalHints(Client *c);
static void Notify(Client *c);
static void SendClientMessage(Client *c, Atom proto);

static void UpdateNetWMName(Client *c);
static void UpdateWMHints(Client *c);
static void UpdateWMNormalHints(Client *c);
static void UpdateWMClass(Client *c);
static void UpdateWMState(Client *c);
static void UpdateWMProtocols(Client *c);

Client *
CreateClient(Window w)
{
    DLog("%ld", w);
    Window r;
    int fx, fy, format;
    unsigned int fw, fh, d, b;
    XSetWindowAttributes attrs;
    unsigned long i, num_items, bytes_after;
    Atom type;
    Atom *atoms;
    Bool fixed, decorated;

    Client *c = malloc(sizeof(Client));
    if (!c)
        return NULL;

    c->window = w;

    /* get informations about the client */
    XGetGeometry(st_dpy, w, &r, &fx, &fy, &fw, &fh, &b, &d);
    fixed = False;
    decorated = True;
    atoms = NULL;
    if (XGetWindowProperty(st_dpy, c->w, st_atm[AtomNetWMWindowType], 0, 1024,
                False, XA_ATOM, &type, &format, &num_items, &bytes_after,
                (unsigned char**)&atoms) == Success) {
        for(i = 0; i < num_items; ++i) {
            if (atoms[i] == st_atm[AtomNetWMWindowTypeDesktop]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeDock]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeSplash]){
                fixed = True;
            }
            if (atoms[i] == st_atm[AtomNetWMWindowTypeDesktop]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeDock]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeMenu]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeNotification]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeSplash]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeToolbar]
                    || atoms[i] == st_atm[AtomNetWMWindowTypePopupMenu]
                    || atoms[i] == st_atm[AtomNetWMWindowTypeTooltip]){
                decorated = False;
            }
        }
        if (atoms)
            XFree(atoms);
    }
    // TODO transient for

    /* create and configure windows */
    /* frame */
    attrs.event_mask = FrameEvenMask;
    attrs.backing_store = WhenMapped;
    c->frame = XCreateWindow(st_dpy, r, 0, 0, 1, 1, 0, CopyFromParent,
            InputOutput, CopyFromParent, CWEventMask|CWBackingStore, &attrs);
    XMapWindow(st_dpy, c->frame);

    /* topbar */
    attrs.event_mask = HandleEventMask;
    attrs.cursor = st_cur[CursorNormal];
    c->topbar = XCreateWindow(st_dpy, c->frame, 0, 0, 1, 1, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWEventMask|CWCursor, &attrs);
    XMapWindow(st_dpy, c->topbar);

    /* buttons */
    attrs.event_mask = ButtonEventMask;
    for (int i = 0; i < ButtonCount; ++i) {
        c->buttons[i] = XCreateWindow(st_dpy, c->topbar, 0, 0, 1, 1, 0,
                CopyFromParent, InputOutput, CopyFromParent,
                CWEventMask|CWCursor, &attrs);
        XMapWindow(st_dpy, c->buttons[i]);
    }

    /* handles */
    attrs.event_mask = HandleEventMask;
    for (int i = 0; i < HandleCount; ++i) {
        attrs.cursor = st_cur[CursorResizeNorth + i];
        c->handles[i] = XCreateWindow(st_dpy, r, 0, 0, 1, 1, 0,
                CopyFromParent, InputOnly, CopyFromParent,
                CWEventMask | CWCursor, &attrs);
        XMapWindow(st_dpy, c->handles[i]);
    }

    /* client */
    attrs.event_mask = ClientEventMask;
    //attrs.do_not_propagate_mask = DoNotPropagateMask;
    XChangeWindowAttributes(st_dpy, c->window, CWEventMask /*| CWDontPropagate */, &attrs);
    XSetWindowBorderWidth(st_dpy, c->window, 0);
    XReparentWindow(st_dpy, c->window, c->frame, 0, 0);
    XMapWindow(st_dpy, c->window);

    /* finalize */
    c->x = c->mx = /* c->hx = */ c->fx = c->px = fx;
    c->y = c->my = /* c->hy = */ c->fy = c->py = fy;
    c->w = c->mw = /* c->hw = */ c->fw = c->pw = fw;
    c->h = c->mh = /* c->hh = */ c->fh = c->ph = fh;
    c->sbw = b;
    c->decorated = decorated;
    c->fdecorated = decorated;
    c->active = False;
    c->urgent = False;
    //c->minimized = False;
    c->vmaximixed = False;
    c->hmaximixed = False;
    c->fullscreen = False;
    c->name = NULL;
    c->focusable = True;
    c->deletable = True;
    c->fixed = fixed;
    c->class = NULL;
    c->instance = NULL;
    c->bw = c->bh = c->incw = c->inch = 0;
    c->minw = c->minh = 0;
    c->maxw = c->maxh = INT_MAX;
    c->mina = c->maxa = 0.0;
    c->left = c->right = c->top = c->bottom = 0;
    c->tags = 0;
    c->lx = c->ly = 0;
    c->lt = 0;
    c->prev = NULL;
    c->next = NULL;

    UpdateNetWMName(c);
    UpdateWMHints(c);
    UpdateWMNormalHints(c);
    UpdateWMClass(c);
    UpdateWMState(c);
    UpdateWMProtocols(c);

    Configure(c);
    //DLog("Created Client for : %ld @ (%d, %d) [%d x %d], frame: %ld, fixed: %d, decorated: %d",
    //        c->window, c->x, c->y, c->w, c->h, c->frame, c->fixed, c->decorated);

    XAddToSaveSet(st_dpy, w);
    return c;
}

void
DestroyClient(Client *c)
{
    if (c->name)
        free(c->name);

    if (c->class)
        free(c->class);

    if (c->instance)
        free(c->instance);

    /* the client window might be destroyed already */
    XReparentWindow(st_dpy, c->window, st_root, c->x, c->y);
    XSetWindowBorderWidth(st_dpy, c->window, c->sbw);

    for (int i = 0; i < ButtonCount; ++i)
        XDestroyWindow(st_dpy, c->buttons[i]);
    XDestroyWindow(st_dpy, c->topbar);

    for (int i = 0; i < HandleCount; ++i)
        XDestroyWindow(st_dpy, c->handles[i]);

    XDestroyWindow(st_dpy, c->frame);

    XRemoveFromSaveSet(st_dpy, c->window);

    free(c);
}

void
CloseClient(Client *c)
{
    if (c->deletable)
        SendClientMessage(c, st_atm[AtomWMDeleteWindow]);
    else
        XKillClient(st_dpy, c->window);
}

void
MoveClient(Client *c, int x, int y)
{
    if (c->fixed || c->fullscreen || c->hmaximixed || c->vmaximixed)
        return;

    c->x = x;
    c->y = y;
    Configure(c);
}

void
ResizeClient(Client *c, int w, int h, Bool sh)
{
    if (c->fixed || c->fullscreen || c->hmaximixed || c->vmaximixed)
        return;

    /* check the minimum size */
    if (c->decorated
            && (w < 2 * st_cfg.border_width + 1
                || h < 2 * st_cfg.border_width + st_cfg.topbar_height + 1))
        return;

    c->w = w;
    c->h = h;
    if (sh)
        ApplyWmNormalHints(c);
    Configure(c);
}

void
MoveResizeClient(Client *c, int x, int y, int w, int h, Bool sh)
{
    if (c->fixed || c->fullscreen || c->hmaximixed || c->vmaximixed)
        return;

    /* check the minimum size */
    if (c->decorated
            && (w < 2 * st_cfg.border_width + 1
                || h < 2 * st_cfg.border_width + st_cfg.topbar_height + 1))
        return;

    //DLog("(%d, %d), [%d x %d], apply hints: %d", x, y, w, h, sh);
    c->x = x;
    c->y = y;
    c->w = w;
    c->h = h;
    if (sh)
        ApplyWmNormalHints(c);
    Configure(c);
}

void
MaximizeClientHorizontally(Client *c)
{
    if (c->fixed || !c->output || c->hmaximixed)
        return;

    if (!c->vmaximixed && !c->hmaximixed) {
        c->mx = c->x;
        c->my = c->y;
        c->mw = c->w;
        c->mh = c->h;
    }

    MoveResizeClient(c, c->output->x + WXOffset(c), c->y,
            c->output->w - WWOffset(c), c->h, False);

    c->hmaximixed = True;
}

void
MaximizeClientVertically(Client *c)
{
    if (c->fixed || !c->output || c->vmaximixed)
        return;

    if (!c->vmaximixed && !c->hmaximixed) {
        c->mx = c->x;
        c->my = c->y;
        c->mw = c->w;
        c->mh = c->h;
    }

    MoveResizeClient(c, c->x, c->output->y + WYOffset(c),
            c->w, c->output->h - WHOffset(c), False);

    c->vmaximixed = True;
}

void
MaximizeClient(Client *c)
{
    if (c->fixed || !c->output || (c->hmaximixed && c->vmaximixed))
        return;

    if (!c->vmaximixed && !c->hmaximixed) {
        c->mx = c->x;
        c->my = c->y;
        c->mw = c->w;
        c->mh = c->h;
    }

    MoveResizeClient(c,
            c->output->x + WXOffset(c),
            c->output->y + WYOffset(c),
            c->output->w - WWOffset(c),
            c->output->h - WHOffset(c), False);

    c->hmaximixed = c->vmaximixed = True;
}

/*
void
MinimizeClient(Client *c)
{
    if (c->fixed || !c->output || c->state & StateMinimized)
        return;

    c->hx = c->x;
    c->hy = c->y;
    c->hw = c->w;
    c->hh = c->h;
    c->state |= StateMinimized;

    MoveClient(c, c->output->x - c->w, c->output->y - c->h);
}
*/

void
FullscreenClient(Client *c, Bool b)
{
    if (c->fixed || !c->output)
        return;

    if (b && !c->fullscreen) {
        XChangeProperty(st_dpy, c->window, st_atm[AtomNetWMState], XA_ATOM, 32,
                PropModeReplace, (unsigned char*)&st_atm[AtomNetWMStateFullscreen],
                1);
        c->fx = c->x;
        c->fy = c->y;
        c->fw = c->w;
        c->fh = c->h;
        c->fdecorated = c->decorated;
        c->decorated = False;
        MoveResizeClient(c, c->output->x, c->output->y, c->output->w,
                c->output->h, False);
        c->fullscreen = b;
    }

    if(!b && c->fullscreen) {
        XChangeProperty(st_dpy, c->window, st_atm[AtomNetWMState], XA_ATOM, 32,
              PropModeReplace, (unsigned char*)0, 0);
        c->fullscreen = False;
        c->decorated = c->fdecorated;
        MoveResizeClient(c, c->fx, c->fy, c->fw, c->fh, False);
    }
}

void
RaiseClient(Client *c)
{
    XRaiseWindow(st_dpy, c->frame);
    for (int i = 0; i < HandleCount; ++i)
        XRaiseWindow(st_dpy, c->handles[i]);
}

void
LowerClient(Client *c)
{
    XLowerWindow(st_dpy, c->frame);
    for (int i = 0; i < HandleCount; ++i)
        XLowerWindow(st_dpy, c->handles[i]);
}

void
RestoreClient(Client *c)
{
    /* some states take precedence over the others */
    //if ((c->state & StateMinimized) == StateMinimized) {
    //    MoveResizeClient(c, c->hx, c->hy, c->hw, c->hh, 0);
    //    c->state &= ~StateMinimized;
    //} else if ((c->state & StateFullscreen) == StateFullscreen) {
    if (c->fullscreen) {
        FullscreenClient(c, False);
    } else if (c->hmaximixed || c->vmaximixed) {
        c->hmaximixed = False;
        c->vmaximixed = False;
        MoveResizeClient(c, c->mx, c->my, c->mw, c->mh, False);
    }
}

void
SetClientActive(Client *c, Bool b)
{
    DLog("%ld: %d", c->window, b);
    if (b) {
        if (c->focusable) {
            XSetInputFocus(st_dpy, c->window, RevertToPointerRoot, CurrentTime);
        //    SendClientMessage(c, st_atm[AtomWMTakeFocus]);
        }
        c->urgent = False;
        c->active = True;
        Decorate(c);
        RaiseClient(c);
        XChangeProperty(st_dpy, st_root, st_atm[AtomNetActiveWindow],
                XA_WINDOW, 32, PropModeReplace, (unsigned char *) &(c->window), 1);
    } else {
        c->active = False;
        Decorate(c);
        XDeleteProperty(st_dpy, st_root, st_atm[AtomNetActiveWindow]);
    }
    GrabButtons(c);
}

void
OnClientExpose(Client *c, XExposeEvent *e)
{
    if (e->window == c->frame)
        Decorate(c);
}

void
OnClientEnter(Client *c, XCrossingEvent *e)
{
    /* redraw button if hovered */
    int bg, fg;
    if (c->active) {
        bg = st_cfg.active_button_hovered_background;
        fg = st_cfg.active_button_hovered_foreground;
    } else {
        bg = st_cfg.inactive_button_hovered_background;
        fg = st_cfg.inactive_button_hovered_foreground;
    }


    for (int i = 0; i < ButtonCount; ++i) {
        if (e->window == c->buttons[i]) {
            int x, y;
            XSetWindowBackground(st_dpy, c->buttons[i], bg);
            XClearWindow(st_dpy, c->buttons[i]);
            GetTextPosition(st_cfg.button_icons[i], st_ift,
                    AlignCenter, AlignMiddle, st_cfg.button_size,
                    st_cfg.button_size, &x, &y);
            WriteText(c->buttons[i], st_cfg.button_icons[i], st_ift, fg, x, y);
        }
    }
}

void OnClientLeave(Client *c, XCrossingEvent *e)
{
    /* redraw button if hovered */
    int bg, fg;
    if (c->active) {
        bg = st_cfg.active_button_background;
        fg = st_cfg.active_button_foreground;
    } else {
        bg = st_cfg.inactive_button_background;
        fg = st_cfg.inactive_button_foreground;
    }

    for (int i = 0; i < ButtonCount; ++i) {
        if (e->window == c->buttons[i]) {
            int x, y;
            XSetWindowBackground(st_dpy, c->buttons[i], bg);
            XClearWindow(st_dpy, c->buttons[i]);
            GetTextPosition(st_cfg.button_icons[i], st_ift,
                    AlignCenter, AlignMiddle, st_cfg.button_size,
                    st_cfg.button_size, &x, &y);
            WriteText(c->buttons[i], st_cfg.button_icons[i], st_ift, fg, x, y);
        }
    }
}

void
OnClientConfigureRequest(Client *c, XConfigureRequestEvent *e)
{
    Bool notify = False;

    if (e->value_mask & CWBorderWidth)
        c->sbw = e->border_width;

    if (e->value_mask & (CWX|CWY|CWWidth|CWHeight)
            && !c->vmaximixed
            && !c->hmaximixed) {
        int x, y, w, h;

        x = c->x;
        y = c->y;
        w = c->w;
        h = c->h;

        if (e->value_mask & CWX)
            x = e->x;
        if (e->value_mask & CWY)
            y = e->y;
        if (e->value_mask & CWWidth)
            w = e->width;
        if (e->value_mask & CWHeight)
            h = e->height;

        MoveResizeClient(c, x, y, w, h, False);
        notify = True;
    }

    /* As the window is reparented, XRaiseWindow and XLowerWindow
       will generate a ConfigureRequest. */
    if (e->value_mask & CWStackMode) {
        if (e->detail == Above || e->detail == TopIf)
            RaiseClient(c);
        if (e->detail == Below || e->detail == BottomIf)
            LowerClient(c);
        notify = True;
    }

    if (notify) {
        Notify(c);
        XSync(st_dpy, False);
    }
}

void
OnClientPropertyNotify(Client *c, XPropertyEvent *e)
{
    if (e->window != c->window)
        return;

    if (e->atom == XA_WM_NAME || e->atom == st_atm[AtomNetWMName])
        UpdateNetWMName(c);

    if (e->atom == XA_WM_HINTS)
        UpdateWMHints(c);

    if (e->atom == XA_WM_NORMAL_HINTS)
        UpdateWMNormalHints(c);

    if (e->atom == XA_WM_CLASS)
        UpdateWMClass(c);

    if (e->atom == st_atm[AtomNetWMState])
        UpdateWMState(c);
}

void
OnClientButtonPress(Client *c, XButtonEvent *e)
{
    DLog();
    c->lx = e->x_root;
    c->ly = e->y_root;
    c->px = c->x;
    c->py = c->y;
    c->pw = c->w;
    c->ph = c->h;

    if (e->window == c->topbar || (e->window == c->window && e->state == Modkey))
        XDefineCursor(st_dpy, e->window, st_cur[CursorMove]);

    if (e->window != c->buttons[ButtonClose])
        XSetInputFocus(st_dpy, c->window, RevertToPointerRoot, CurrentTime);

    if (e->window == c->buttons[ButtonMaximize]) {
        if (c->hmaximixed || c->vmaximixed)
            RestoreClient(c);
        else
            MaximizeClient(c);
    }

    if (e->window == c->buttons[ButtonClose])
        CloseClient(c);

    if (e->window == c->window)
        XAllowEvents(st_dpy, ReplayPointer, e->time);
}

void
OnClientButtonRelease(Client *c, XButtonEvent *e)
{
    for (int i = 0; i < HandleCount; ++i) {
        if (e->window == c->handles[i]) {
            ApplyWmNormalHints(c);
            Configure(c);
            break;
        }
    }

    if (e->window == c->topbar || e->window == c->window)
        XDefineCursor(st_dpy, e->window, st_cur[CursorNormal]);

    if (e->window == c->window)
        XAllowEvents(st_dpy, ReplayPointer, e->time);
}

void
OnClientMotionNotify(Client *c, XMotionEvent *e)
{
    int vx = e->x_root - c->lx;
    int vy = e->y_root - c->ly;

    if ((e->time - c->lt) <= (1000 / 60))
        return;
    c->lt = e->time;

    /* we do not apply normal hints during motion but when button is released
     * to make the resizing visually smoother. Some client apply normals by
     * themselves anway (e.g gnome-terminal) */
    if (e->window == c->topbar || e->window == c->window)
        MoveClient(c, c->px + vx, c->py + vy);
    else if (e->window == c->handles[HandleNorth])
        MoveResizeClient(c, c->px, c->py + vy, c->pw, c->ph - vy , True);
    else if (e->window == c->handles[HandleWest])
        ResizeClient(c, c->pw + vx, c->ph, False);
    else if (e->window == c->handles[HandleSouth])
        ResizeClient(c, c->pw, c->ph + vy, False);
    else if (e->window == c->handles[HandleEast])
        MoveResizeClient(c, c->px + vx, c->py, c->pw - vx, c->ph, False);
    else if (e->window == c->handles[HandleNorthEast])
        MoveResizeClient(c, c->px + vx, c->py + vy, c->pw - vx, c->ph - vy, False);
    else if (e->window == c->handles[HandleNorthWest])
        MoveResizeClient(c, c->px, c->py + vy, c->pw + vx, c->ph - vy, False);
    else if (e->window == c->handles[HandleSouthWest])
        ResizeClient(c, c->pw + vx, c->ph + vy, False);
    else if (e->window == c->handles[HandleSouthEast])
        MoveResizeClient(c, c->px + vx, c->py, c->pw - vx, c->ph + vy, False);
    else
      return;
}

// XXX to be checked
void
OnClientMessage(Client *c, XClientMessageEvent *e)
{
    if (e->message_type == st_atm[AtomNetWMState]) {
        if (e->data.l[1] == (long)st_atm[AtomNetWMStateFullscreen]
                || e->data.l[2] == (long)st_atm[AtomNetWMStateFullscreen]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                FullscreenClient(c, False);
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                FullscreenClient(c, True);
            if (e->data.l[0] == 2) /* _NET_WM_STATE_TOGGLE */
                FullscreenClient(c, !c->fullscreen);
        }
        if (e->data.l[1] == (long)st_atm[AtomNetWMStateDemandsAttention]
                || e->data.l[2] == (long)st_atm[AtomNetWMStateDemandsAttention]) {
            if (e->data.l[0] == 0) /* _NET_WM_STATE_REMOVE */
                c->urgent = False;
            if (e->data.l[0] == 1) /* _NET_WM_STATE_ADD */
                c->urgent = True;
            if (e->data.l[0] == 2) /* _NET_WM_STATE_TOGGLE */
                c->urgent = !c->urgent;
            Decorate(c);
        }
    }

    // XXX: ???
    if (e->message_type == st_atm[AtomNetActiveWindow])
        XSetInputFocus(st_dpy, c->window, RevertToPointerRoot, CurrentTime);
}

void
GrabButtons(Client *c)
{
    unsigned int modifiers[] = { 0, LockMask, st_nlm, st_nlm|LockMask };

    XUngrabButton(st_dpy, AnyButton, AnyModifier, c->window);
    if (!c->active) {
        XGrabButton(st_dpy, AnyButton, AnyModifier, c->window, False,
                ButtonPressMask | ButtonReleaseMask, GrabModeSync,
                GrabModeSync, None, None);
    } else {
        for (int i = 0; i < 4; i++)
            XGrabButton(st_dpy, Button1, Modkey | modifiers[i], c->window, False,
                    ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                    GrabModeAsync, GrabModeSync, None, None);
    }
}

void
Configure(Client *c)
{
    int fx, fy, fw, fh; /* frame geometry  */
    int cx, cy, cw, ch; /* client geometry */

    fx = c->x - WXOffset(c);
    fy = c->y - WYOffset(c);
    fw = c->w + WWOffset(c);
    fh = c->h + WHOffset(c);

    cx = WXOffset(c);
    cy = WYOffset(c);
    cw = c->w;
    ch = c->h;

    //DLog("Frame: (%d, %d) x [%d, %d]", fx, fy, fw, fh);
    //DLog("Window: (%d, %d) x [%d, %d]", cx, cy, cw, ch);

    /* apply the geometry */
    XMoveResizeWindow(st_dpy, c->frame, fx, fy, fw, fh);
    XMoveResizeWindow(st_dpy, c->window, cx, cy, cw, ch);
    if (c->decorated) {
        XMoveResizeWindow(st_dpy, c->topbar, st_cfg.border_width,
                st_cfg.border_width, c->w, st_cfg.topbar_height);
        for (int i = 0; i < ButtonCount; ++i) {
            int bx = c->w - (i + 1) * (st_cfg.button_size + st_cfg.button_gap);
            int by = (st_cfg.topbar_height - st_cfg.button_size) / 2;
            XMoveResizeWindow(st_dpy, c->buttons[i], bx, by,
                    st_cfg.button_size, st_cfg.button_size);
        }
    } else {
        XMoveWindow(st_dpy, c->topbar, st_cfg.border_width,
                -(st_cfg.border_width + st_cfg.topbar_height));

    }
    int hw = st_cfg.handle_width;
    XMoveResizeWindow(st_dpy, c->handles[HandleNorth], fx, fy - hw, fw, hw);
    XMoveResizeWindow(st_dpy, c->handles[HandleNorthWest], fx + fw, fy - hw, hw, hw);
    XMoveResizeWindow(st_dpy, c->handles[HandleWest], fx + fw, fy, hw, fh);
    XMoveResizeWindow(st_dpy, c->handles[HandleSouthWest], fx + fw, fy + fh, hw, hw);
    XMoveResizeWindow(st_dpy, c->handles[HandleSouth], fx, fy + fh, fw, hw);
    XMoveResizeWindow(st_dpy, c->handles[HandleSouthEast], fx - hw, fy + fh, hw, hw);
    XMoveResizeWindow(st_dpy, c->handles[HandleEast], fx - hw, fy, hw, fh);
    XMoveResizeWindow(st_dpy, c->handles[HandleNorthEast], fx - hw, fy - hw, hw, hw);
}

void
ApplyWmNormalHints(Client *c)
{
    int baseismin = (c->bw == c->minw) && (c->bh == c->minh);

    /* temporarily remove base dimensions, ICCCM 4.1.2.3 */
    if (!baseismin) {
        c->w -= c->bw;
        c->h -= c->bh;
    }

    /* adjust for aspect limits */
    if (c->mina && c->maxa) {
        if (c->maxa < (float)c->w / c->h)
            c->w = c->h * c->maxa;
        else if (c->mina < (float)c->h / c->w)
            c->h = c->w * c->mina;
    }

    if (c->incw && c->inch) {
        /* remove base dimensions for increment */
        if (baseismin) {
            c->w -= c->bw;
            c->h -= c->bh;
        }
        /* adjust for increment value */
        c->w -= c->w % c->incw;
        c->h -= c->h % c->incw;
        /* restore base dimensions */
        c->w += c->bw;
        c->h += c->bh;
    }

    /* adjust for min max width/height */
    c->w = c->w < c->minw ? c->minw : c->w;
    c->h = c->h < c->minh ? c->minh : c->h;
    c->w = c->w > c->maxw ? c->maxw : c->w;
    c->h = c->h > c->maxh ? c->maxh : c->h;
}

void
Notify(Client *c)
{
    XConfigureEvent e;
    e.event = c->window;
    e.window = c->window;
    e.type = ConfigureNotify;
    e.x = c->x;
    e.y = c->y;
    e.width = c->w;
    e.height = c->h;
    e.border_width = 0;
    e.above = None;
    e.override_redirect = 0;

    XSendEvent(st_dpy, c->window, False, SubstructureNotifyMask, (XEvent*)&e);
}

void
SendClientMessage(Client *c, Atom proto)
{
    XClientMessageEvent  cm;

    (void)memset(&cm, 0, sizeof(cm));
    cm.type = ClientMessage;
    cm.window = c->window;
    cm.message_type = st_atm[AtomWMProtocols];
    cm.format = 32;
    cm.data.l[0] = proto;
    cm.data.l[1] = CurrentTime;

    XSendEvent(st_dpy, c->window, False, NoEventMask, (XEvent *)&cm);
}

void
Decorate(Client *c)
{
    int bg, fg, bbg, bfg, x, y;

    if (!c->decorated)
        return;

    /* select the colors */
    if (c->urgent) {
        bg = bbg = st_cfg.urgent_background;
        fg = bfg = st_cfg.urgent_foreground;
    } else if (c->active) {
        bg = st_cfg.active_background;
        fg = st_cfg.active_foreground;
        bbg = st_cfg.active_button_background;
        bfg = st_cfg.active_button_foreground;
    } else {
        bg = st_cfg.inactive_background;
        fg = st_cfg.inactive_foreground;
        bbg = st_cfg.inactive_button_background;
        bfg = st_cfg.inactive_button_foreground;
    }

    XSetWindowBackground(st_dpy, c->frame, bg);
    XClearWindow(st_dpy, c->frame);
    XSetWindowBackground(st_dpy, c->topbar, bg);
    XClearWindow(st_dpy, c->topbar);
    GetTextPosition(c->name, st_lft, AlignCenter, AlignMiddle,
            c->w - 2 * st_cfg.border_width, st_cfg.topbar_height, &x, &y);
    WriteText(c->topbar, c->name, st_lft, fg, x, y);
    for (int i = 0; i < ButtonCount; ++i) {
        XSetWindowBackground(st_dpy, c->buttons[i], bbg);
        XClearWindow(st_dpy, c->buttons[i]);
        GetTextPosition(st_cfg.button_icons[i], st_ift,
                AlignCenter, AlignMiddle, st_cfg.button_size,
                st_cfg.button_size, &x, &y);
        WriteText(c->buttons[i], st_cfg.button_icons[i], st_ift, bfg, x, y);
    }
}

void
UpdateNetWMName(Client *c)
{
    if (c->name)
        free(c->name);
    c->name = NULL;

    XTextProperty name;

    if (!XGetTextProperty(st_dpy, c->window, &name, st_atm[AtomNetWMName]) || !name.nitems)
        if (!XGetTextProperty(st_dpy, c->window, &name, XA_WM_NAME) || !name.nitems) {
            c->name = strdup("Error");
            return;
        }

    if (name.encoding == XA_STRING) {
        c->name = strdup((char*)name.value);
    } else {
        char **list = NULL;
        int n;
        if (XmbTextPropertyToTextList(st_dpy, &name, &list, &n) == Success && n > 0 && *list) {
            c->name = strdup((char*)*list);
            XFreeStringList(list);
        }
    }
    XFree(name.value);

    if (!c->name)
        c->name = strdup("None");
}

void
UpdateWMHints(Client *c)
{
    XWMHints *hints = XGetWMHints(st_dpy, c->window);
    if (hints) {
        if (c->urgent != (hints->flags & XUrgencyHint)) {
            c->urgent = (hints->flags & XUrgencyHint);
            Decorate(c);
        }

        c->focusable = (hints->flags & InputHint && hints->input);

        XFree(hints);
    }
}

void
UpdateWMNormalHints(Client *c)
{
    XSizeHints hints;
    long supplied;

    c->bw =c->bh = c->incw = c->inch = 0;
    c->minw = c->minh = 0;
    c->maxw = c->maxh = INT_MAX;
    c->mina = c->maxa = 0.0;

    if (XGetWMNormalHints(st_dpy, c->window, &hints, &supplied)) {
        if (supplied & PBaseSize) {
            c->bw = hints.base_width;
            c->bh = hints.base_height;
        }
        if (supplied & PResizeInc) {
            c->incw = hints.width_inc;
            c->inch = hints.height_inc;
        }
        if (supplied & PMinSize && hints.min_width && hints.min_height) {
            c->minw = hints.min_width;
            c->minh = hints.min_height;
        }
        if (supplied & PMaxSize && hints.max_width && hints.max_height ) {
            c->maxw = hints.max_width;
            c->maxh = hints.max_height;
        }
        if (supplied & PAspect && hints.min_aspect.y && hints.min_aspect.x) {
            c->mina = (float)hints.min_aspect.y / (float)hints.min_aspect.x;
            c->maxa = (float)hints.max_aspect.x / (float)hints.max_aspect.y;
        }
    }
}

void
UpdateWMClass(Client *c)
{
    XClassHint h;

    if (c->class)
        free(c->class);
    c->class = NULL;

    if (c->instance)
        free(c->instance);
    c->instance = NULL;

    if (XGetClassHint(st_dpy, c->window, &h)) {
        c->class = strdup(h.res_class);
        c->instance = strdup(h.res_name);
        XFree(h.res_class);
        XFree(h.res_name);
    }
}

void
UpdateWMState(Client *c)
{
    Atom type;
    int format;
    unsigned long i, num_items, bytes_after;
    Atom *atoms;

    atoms = NULL;

    XGetWindowProperty(st_dpy, c->window, st_atm[AtomNetWMState], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&atoms);

    for(i=0; i<num_items; ++i) {
        if(atoms[i] == st_atm[AtomNetWMStateMaximizedVert])
            MaximizeClientVertically(c);
        if(atoms[i] == st_atm[AtomNetWMStateMaximizedHorz])
            MaximizeClientHorizontally(c);
        if(atoms[i] == st_atm[AtomNetWMStateFullscreen])
            FullscreenClient(c, True);
        if(atoms[i] == st_atm[AtomNetWMStateAbove])
            RaiseClient(c);
        if(atoms[i] == st_atm[AtomNetWMStateBelow])
            LowerClient(c);
        if(atoms[i] == st_atm[AtomNetWMStateDemandsAttention]) {
            c->urgent = True;
            Decorate(c);
        }
    }
    XFree(atoms);
}

void
UpdateWMProtocols(Client *c)
{
    Atom *protocols;
    int n;
    if (XGetWMProtocols(st_dpy, c->window, &protocols, &n)) {
        for (int i = 0; i < n; ++i) {
            if (protocols[i] == st_atm[AtomWMTakeFocus])
                c->focusable = True;
            if (protocols[i] == st_atm[AtomWMDeleteWindow])
                c->deletable = True;
        }
        XFree(protocols);
    }
}


