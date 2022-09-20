#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "hints.h"
#include "log.h"
#include "x11.h"

void
GetWMName(Window w, char **name)
{
    if (*name)
        free(*name);
    *name = NULL;

    XTextProperty p;

    if (!XGetTextProperty(display, w, &p, atoms[AtomNetWMName])
            || !p.nitems)
        if (!XGetTextProperty(display, w, &p, XA_WM_NAME) || !p.nitems) {
            *name = strdup("Error");
            return;
        }

    if (p.encoding == XA_STRING) {
        *name = strdup((char*)p.value);
    } else {
        char **list = NULL;
        int n;
        if (XmbTextPropertyToTextList(display, &p, &list, &n) >= Success
                && n > 0 && *list) {
            if (n > 1) {
                XTextProperty p2;
                if (XmbTextListToTextProperty(display, list, n,
                            XStringStyle, &p2) == Success) {
                    *name = strdup((char *)p2.value);
                    XFree(p2.value);
                }
            } else {
                *name = strdup((char*)*list);
            }
            XFreeStringList(list);
        }
    }
    XFree(p.value);

    if (!*name)
        *name = strdup("None");
}

void
GetWMHints(Window w, WMHints *h)
{
    /* the default is to be focusable */
    *h = HintsFocusable;
    XWMHints *hints = XGetWMHints(display, w);
    if (hints) {
        /* urgency */
        if (hints->flags & XUrgencyHint)
            *h |= HintsUrgent;

        /* focusable */
        if (hints->flags & InputHint && ! hints->input)
            *h &= ~HintsFocusable;

        XFree(hints);
    }
}

void
GetWMProtocols(Window w, WMProtocols *h)
{
    Atom *protocols;
    int n;
    if (XGetWMProtocols(display, w, &protocols, &n)) {
        for (int i = 0; i < n; ++i) {
            if (protocols[i] == atoms[AtomWMTakeFocus])
                *h |= NetWMProtocolTakeFocus;
            if (protocols[i] == atoms[AtomWMDeleteWindow])
                *h |= NetWMProtocolDeleteWindow;
        }
        XFree(protocols);
    }
}

void
GetWMNormals(Window w, WMNormals *h)
{
    XSizeHints hints;
    long supplied;

    h->bw = h->bh = h->incw = h->inch = 0;
    h->minw = h->minh = 0;
    h->maxw = h->maxh = INT_MAX;
    h->mina = h->maxa = 0.0;

    if (XGetWMNormalHints(display, w, &hints, &supplied)) {
        if (supplied & PBaseSize) {
            h->bw = hints.base_width;
            h->bh = hints.base_height;
        }
        if (supplied & PResizeInc) {
            h->incw = hints.width_inc;
            h->inch = hints.height_inc;
        }
        if (supplied & PMinSize && hints.min_width && hints.min_height) {
            h->minw = hints.min_width;
            h->minh = hints.min_height;
        }
        if (supplied & PMaxSize && hints.max_width && hints.max_height ) {
            h->maxw = hints.max_width;
            h->maxh = hints.max_height;
        }
        if (supplied & PAspect && hints.min_aspect.y && hints.min_aspect.x) {
            h->mina = (float)hints.min_aspect.y / (float)hints.min_aspect.x;
            h->maxa = (float)hints.max_aspect.x / (float)hints.max_aspect.y;
        }
    }
}

void
GetWMClass(Window w, WMClass *klass)
{
    XClassHint hints;

    if (klass->cname)
        free(klass->cname);
    klass->cname = NULL;

    if (klass->iname)
        free(klass->iname);
    klass->iname = NULL;

    if (XGetClassHint(display, w, &hints)) {
        klass->cname = strdup(hints.res_class);
        klass->iname = strdup(hints.res_name);
        XFree(hints.res_class);
        XFree(hints.res_name);
    }
}

void
GetWMStrut(Window w, WMStrut *strut)
{
    Atom type;
    int format, status;
    unsigned long num_items, bytes_after;
    Atom *prop;

    prop = NULL;
    strut->left = strut->right = strut->top = strut->bottom = 0;
    status = XGetWindowProperty(display, w, atoms[AtomNetWMStrutpartial],
            0, 12, False, XA_CARDINAL, &type, &format, &num_items,
            &bytes_after, (unsigned char**)&prop);

    if ((status == Success) && prop) {
        strut->left = prop[0];
        strut->right = prop[1];
        strut->top = prop[2];
        strut->bottom = prop[3];
        XFree(prop);
    }
}

void
GetNetWMWindowType(Window w, NetWMWindowType *h)
{
    Atom type;
    int format;
    unsigned long i, num_items, bytes_after;
    Atom *wtypes;

    wtypes = NULL;

    XGetWindowProperty(display, w, atoms[AtomNetWMWindowType], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&wtypes);

    *h = 0;
    for (i = 0; i < num_items; ++i) {
        if (wtypes[i] == atoms[AtomNetWMWindowTypeNormal])
            *h |= NetWMTypeNormal;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeDialog])
            *h |= NetWMTypeDialog;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeDesktop])
            *h |= NetWMTypeDesktop;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeDock])
            *h |= NetWMTypeDock;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeMenu])
            *h |= NetWMTypeMenu;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeNotification])
            *h |= NetWMTypeNotification;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeSplash])
            *h |= NetWMTypeSplash;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeToolbar])
            *h |= NetWMTypeToolbar;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeUtility])
            *h |= NetWMTypeUtility;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeCombo])
            *h |= NetWMTypeCombo;
        if (wtypes[i] == atoms[AtomNetWMWindowTypePopupMenu])
            *h |= NetWMTypePopupMenu;
        if (wtypes[i] == atoms[AtomNetWMWindowTypeTooltip])
            *h |= NetWMTypeTooltip;
    }
    XFree(wtypes);
    if (!*h)
        *h |= NetWMTypeNormal;
}

void
GetNetWMStates(Window w, NetWMStates *h)
{
    Atom type;
    int format;
    unsigned long i, num_items, bytes_after;
    Atom *states;

    states = NULL;

    XGetWindowProperty(display, w, atoms[AtomNetWMState], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&states);

    *h = NetWMStateNone;

    for(i = 0; i < num_items; ++i) {
        if (states[i] == atoms[AtomNetWMStateModal])
            *h |= NetWMStateModal;
        if (states[i] == atoms[AtomNetWMStateSticky])
            *h |= NetWMStateSticky;
        if (states[i] == atoms[AtomNetWMStateMaximizedVert])
            *h |= NetWMStateMaximizedVert;
        if (states[i] == atoms[AtomNetWMStateMaximizedHorz])
            *h |= NetWMStateMaximizedHorz;
        if (states[i] == atoms[AtomNetWMStateShaded])
            *h |= NetWMStateShaded;
        if (states[i] == atoms[AtomNetWMStateSkipTaskbar])
            *h |= NetWMStateSkipTaskbar;
        if (states[i] == atoms[AtomNetWMStateSkipPager])
            *h |= NetWMStateSkipPager;
        if (states[i] == atoms[AtomNetWMStateHidden])
            *h |= NetWMStateHidden;
        if (states[i] == atoms[AtomNetWMStateFullscreen])
            *h |= NetWMStateFullscreen;
        if (states[i] == atoms[AtomNetWMStateAbove])
            *h |= NetWMStateAbove;
        if (states[i] == atoms[AtomNetWMStateBelow])
            *h |= NetWMStateBelow;
        if (states[i] == atoms[AtomNetWMStateSticky])
            *h |= NetWMStateSticky;
        if (states[i] == atoms[AtomNetWMStateDemandsAttention])
            *h |= NetWMStateDemandsAttention;
    }
    XFree(states);
}

void
SetNetWMAllowedActions(Window w, NetWMActions a)
{
    int i, n, count;
    Atom *actions;

    /* count the atoms we will set */
    n = a;
    count = 0;
    while (n) {
        n &= (n - 1);
        count++;
    }

    /* now we can allocate a new set of atoms */
    actions = malloc(count * sizeof(Atom));
    if (! actions) {
        ELog("can't alloc atoms");
        XFree(atoms);
        return;
    }

    i = 0;
    if (a & NetWMActionMove)
        actions[i++] = atoms[AtomNetWMActionMove];
    if (a & NetWMActionResize)
        actions[i++] = atoms[AtomNetWMActionResize];
    if (a & NetWMActionMinimize)
        actions[i++] = atoms[AtomNetWMActionMinimize];
    if (a & NetWMActionShade)
        actions[i++] = atoms[AtomNetWMActionShade];
    if (a & NetWMActionStick)
        actions[i++] = atoms[AtomNetWMActionStick];
    if (a & NetWMActionMaximizeHorz)
        actions[i++] = atoms[AtomNetWMActionMaximizeHorz];
    if (a & NetWMActionMaximizeVert)
        actions[i++] = atoms[AtomNetWMActionMaximizeVert];
    if (a & NetWMActionFullscreen)
        actions[i++] = atoms[AtomNetWMActionFullscreen];
    if (a & NetWMActionChangeDesktop)
        actions[i++] = atoms[AtomNetWMActionChangeDesktop];
    if (a & NetWMActionClose)
        actions[i++] = atoms[AtomNetWMActionClose];
    if (a & NetWMActionAbove)
        actions[i++] = atoms[AtomNetWMActionAbove];
    if (a & NetWMActionBelow)
        actions[i++] = atoms[AtomNetWMActionBelow];

    /* finally set them */
    XChangeProperty(display, w, atoms[AtomNetWMAllowedActions], XA_ATOM, 32,
            PropModeReplace, (unsigned char*)actions, count);

    XFree(actions);
}


void
SetNetWMStates(Window w, NetWMStates h)
{
    Atom type;
    int format, count, n;
    unsigned long i, j, num_items, bytes_after;
    Atom *cstates, *nstates;

    /* get the current state atoms */
    cstates = NULL;
    XGetWindowProperty(display, w, atoms[AtomNetWMState], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&cstates);

    /* count the number of atoms we don't honor but we will keep
     * for whatever the app wants to do wit it */
    count = 0;
    for(i = 0; i < num_items; ++i) {
        if (cstates[i] == atoms[AtomNetWMStateModal]
                || cstates[i] == atoms[AtomNetWMStateShaded]
                || cstates[i] == atoms[AtomNetWMStateSkipTaskbar]
                || cstates[i] == atoms[AtomNetWMStateSkipPager]) {
            count++;
        }
    }

    /* count the atoms we will set */
    n = h;
    while (n) {
        n &= (n - 1);
        count++;
    }

    /* now we can allocate a new set of atoms */
    nstates = malloc(count * sizeof(Atom));
    if (! nstates) {
        ELog("can't alloc atoms");
        XFree(cstates);
        return;
    }

    /* populate it with existing atoms not honored */
    j = 0;
    for(i = 0; i < num_items; ++i) {
        if (cstates[i] == atoms[AtomNetWMStateModal]
                || cstates[i] == atoms[AtomNetWMStateShaded]
                || cstates[i] == atoms[AtomNetWMStateSkipTaskbar]
                || cstates[i] == atoms[AtomNetWMStateSkipPager]) {
            nstates[j++] = atoms[i];
        }
    }

    /* add ours */
    if (h & NetWMStateMaximizedVert)
        nstates[j++] = atoms[AtomNetWMStateMaximizedVert];
    if (h & NetWMStateMaximizedHorz)
        nstates[j++] = atoms[AtomNetWMStateMaximizedHorz];
    if (h & NetWMStateHidden)
        nstates[j++] = atoms[AtomNetWMStateHidden];
    if (h & NetWMStateFullscreen)
        nstates[j++] = atoms[AtomNetWMStateFullscreen];
    if (h & NetWMStateAbove)
        nstates[j++] = atoms[AtomNetWMStateAbove];
    if (h & NetWMStateBelow)
        nstates[j++] = atoms[AtomNetWMStateBelow];
    if (h & NetWMStateDemandsAttention)
        nstates[j++] = atoms[AtomNetWMStateDemandsAttention];

    /* finally set them */
    XChangeProperty(display, w, atoms[AtomNetWMState], XA_ATOM, 32,
            PropModeReplace, (unsigned char*)nstates, count);

    XFree(cstates);
    XFree(nstates);
}

void
GetMotifHints(Window w, MotifHints *h)
{
    Atom type;
    int format, status;
    unsigned long num_items, bytes_after;
    Atom *prop;

    memset(h, 0, sizeof(MotifHints));
    prop = NULL;
    status = XGetWindowProperty(display, w, atoms[AtomMotifWMHints], 0, 5,
            False, atoms[AtomMotifWMHints], &type, &format, &num_items, &bytes_after,
            (unsigned char**)&prop);

    if ((status == Success) && prop && num_items > 4) {
        h->flags = prop[0];
        h->functions = prop[1];
        h->decorations = prop[2];
        h->input_mode = prop[3];
        h->state = prop[4];
        XFree(prop);
    }
}

void
SendMessage(Window w, Atom a)
{
    XEvent e;

    e.type = ClientMessage;
    e.xclient.window = w;
    e.xclient.message_type = atoms[AtomWMProtocols];
    e.xclient.format = 32;
    e.xclient.data.l[0] = a;
    e.xclient.data.l[1] = CurrentTime;

    XSendEvent(display, w, False, NoEventMask, &e);
}

