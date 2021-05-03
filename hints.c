#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xutil.h>

#include "atoms.h"
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

    if (!XGetTextProperty(stDisplay, w, &p, stAtoms[AtomNetWMName]) || !p.nitems)
        if (!XGetTextProperty(stDisplay, w, &p, XA_WM_NAME) || !p.nitems) {
            *name = strdup("Error");
            return;
        }

    if (p.encoding == XA_STRING) {
        *name = strdup((char*)p.value);
    } else {
        char **list = NULL;
        int n;
        if (XmbTextPropertyToTextList(stDisplay, &p, &list, &n) == Success && n > 0 && *list) {
            *name = strdup((char*)*list);
            XFreeStringList(list);
        }
    }
    XFree(p.value);

    if (!*name)
        *name = strdup("None");
}

void
GetWMHints(Window w, int *h)
{
    XWMHints *hints = XGetWMHints(stDisplay, w);
    if (hints) {
        /* urgency */
        if (hints->flags & XUrgencyHint)
            *h |= HintsUrgent;
        else
            *h &= ~HintsUrgent;

        /* focusable */
        if (hints->flags & InputHint && hints->input)
            *h |= HintsFocusable;
        else
            *h &= ~HintsFocusable;

        XFree(hints);
    }
}

void
GetWMProtocols(Window w, int *h)
{
    Atom *protocols;
    int n;
    if (XGetWMProtocols(stDisplay, w, &protocols, &n)) {
        for (int i = 0; i < n; ++i) {
            if (protocols[i] == stAtoms[AtomWMTakeFocus])
                *h |= NetWMProtocolTakeFocus;
            if (protocols[i] == stAtoms[AtomWMDeleteWindow])
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

    if (XGetWMNormalHints(stDisplay, w, &hints, &supplied)) {
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

    if (XGetClassHint(stDisplay, w, &hints)) {
        klass->cname = strdup(hints.res_class);
        klass->iname = strdup(hints.res_name);
        XFree(hints.res_class);
        XFree(hints.res_name);
    }
}

void
GetWMStrut(Window w, WMStrut *strut)
{
    /* TODO */
    (void)w;
    strut->left = strut->right = strut->top = strut->bottom = 0;
}

void
GetNetWMWindowType(Window w, int *h)
{
    Atom type;

    int format;
    unsigned long i, num_items, bytes_after;
    Atom *atoms;

    atoms = NULL;

    XGetWindowProperty(stDisplay, w, stAtoms[AtomNetWMWindowType], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&atoms);

    DLog("nb atoms: %ld", num_items);
    for (i = 0; i < num_items; ++i) {
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeNormal])
            *h |= NetWMTypeNormal;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeDialog])
            *h |= NetWMTypeDialog;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeDesktop])
            *h |= NetWMTypeDesktop;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeDock])
            *h |= NetWMTypeDock;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeMenu])
            *h |= NetWMTypeMenu;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeNotification])
            *h |= NetWMTypeNotification;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeSplash])
            *h |= NetWMTypeSplash;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeToolbar])
            *h |= NetWMTypeToolbar;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeUtility])
            *h |= NetWMTypeUtility;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeCombo])
            *h |= NetWMTypeCombo;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypePopupMenu])
            *h |= NetWMTypePopupMenu;
        if (atoms[i] == stAtoms[AtomNetWMWindowTypeTooltip])
            *h |= NetWMTypeTooltip;
    }
    XFree(atoms);
}

void
GetNetWMState(Window w, int *h)
{
    Atom type;
    int format;
    unsigned long i, num_items, bytes_after;
    Atom *atoms;

    atoms = NULL;

    XGetWindowProperty(stDisplay, w, stAtoms[AtomNetWMState], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&atoms);

    *h = NetWMStateNone;

    for(i = 0; i < num_items; ++i) {
        if (atoms[i] == stAtoms[AtomNetWMStateModal])
            *h |= NetWMStateModal;
        if (atoms[i] == stAtoms[AtomNetWMStateSticky])
            *h |= NetWMStateSticky;
        if (atoms[i] == stAtoms[AtomNetWMStateMaximizedVert])
            *h |= NetWMStateMaximizedVert;
        if (atoms[i] == stAtoms[AtomNetWMStateMaximizedHorz])
            *h |= NetWMStateMaximizedHorz;
        if (atoms[i] == stAtoms[AtomNetWMStateShaded])
            *h |= NetWMStateShaded;
        if (atoms[i] == stAtoms[AtomNetWMStateSkipTaskbar])
            *h |= NetWMStateSkipTaskbar;
        if (atoms[i] == stAtoms[AtomNetWMStateSkipPager])
            *h |= NetWMStateSkipPager;
        if (atoms[i] == stAtoms[AtomNetWMStateHidden])
            *h |= NetWMStateHidden;
        if (atoms[i] == stAtoms[AtomNetWMStateFullscreen])
            *h |= NetWMStateFullscreen;
        if (atoms[i] == stAtoms[AtomNetWMStateAbove])
            *h |= NetWMStateAbove;
        if (atoms[i] == stAtoms[AtomNetWMStateBelow])
            *h |= NetWMStateBelow;
        if (atoms[i] == stAtoms[AtomNetWMStateSticky])
            *h |= NetWMStateSticky;
        if (atoms[i] == stAtoms[AtomNetWMStateDemandsAttention])
            *h |= NetWMStateDemandsAttention;
    }
    XFree(atoms);
}

void
SetNetWMState(Window w, int h)
{
    Atom type;
    int format, count, n;
    unsigned long i, j, num_items, bytes_after;
    Atom *atoms, *natoms;

    /* get the current state atoms */
    atoms = NULL;
    XGetWindowProperty(stDisplay, w, stAtoms[AtomNetWMState], 0, 1024,
            False, XA_ATOM, &type, &format, &num_items, &bytes_after,
            (unsigned char**)&atoms);

    /* count the number of atoms we don't honor but we will keep
     * for whatever the app wants to do wit it */
    count = 0;
    for(i = 0; i < num_items; ++i) {
        if (atoms[i] == stAtoms[AtomNetWMStateModal]
                || atoms[i] == stAtoms[AtomNetWMStateSticky]
                || atoms[i] == stAtoms[AtomNetWMStateShaded]
                || atoms[i] == stAtoms[AtomNetWMStateSkipTaskbar]
                || atoms[i] == stAtoms[AtomNetWMStateSkipPager]) {
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
    natoms = malloc(count * sizeof(Atom));
    if (! natoms) {
        ELog("can't alloc atoms");
        XFree(atoms);
        return;
    }

    /* populate it with existing atoms not honored */
    j = 0;
    for(i = 0; i < num_items; ++i) {
        if (atoms[i] == stAtoms[AtomNetWMStateModal]
                || atoms[i] == stAtoms[AtomNetWMStateSticky]
                || atoms[i] == stAtoms[AtomNetWMStateShaded]
                || atoms[i] == stAtoms[AtomNetWMStateSkipTaskbar]
                || atoms[i] == stAtoms[AtomNetWMStateSkipPager]) {
            natoms[j++] = atoms[i];
        }
    }

    /* add ours */
    if (h & NetWMStateMaximizedVert)
        natoms[j++] = stAtoms[AtomNetWMStateMaximizedVert];
    if (h & NetWMStateMaximizedHorz)
        natoms[j++] = stAtoms[AtomNetWMStateMaximizedHorz];
    if (h & NetWMStateHidden)
        natoms[j++] = stAtoms[AtomNetWMStateHidden];
    if (h & NetWMStateFullscreen)
        natoms[j++] = stAtoms[AtomNetWMStateFullscreen];
    if (h & NetWMStateAbove)
        natoms[j++] = stAtoms[AtomNetWMStateAbove];
    if (h & NetWMStateBelow)
        natoms[j++] = stAtoms[AtomNetWMStateBelow];
    if (h & NetWMStateDemandsAttention)
        natoms[j++] = stAtoms[AtomNetWMStateDemandsAttention];

    /* finally set them */
    XChangeProperty(stDisplay, w, stAtoms[AtomNetWMState], XA_ATOM, 32,
            PropModeReplace, (unsigned char*)natoms, count);
 
    XFree(atoms);
    XFree(natoms);
}

void
SendMessage(Window w, Atom a)
{
    XClientMessageEvent  cm;

    (void)memset(&cm, 0, sizeof(cm));
    cm.type = ClientMessage;
    cm.window = w;
    cm.message_type = stAtoms[AtomWMProtocols];
    cm.format = 32;
    cm.data.l[0] = a;
    cm.data.l[1] = CurrentTime;

    XSendEvent(stDisplay, w, False, NoEventMask, (XEvent *)&cm);
}

