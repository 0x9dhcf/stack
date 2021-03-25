static const Config default_config = {
    .border_width = 1,
    .topbar_height = 32,
    .handle_width = 6,
    .button_size = 22,
    .button_gap = 5,

    .active_background                      = 0xd8dee9,
    .active_foreground                      = 0x434c5e,

    .inactive_background                    = 0xd8dee9,
    .inactive_foreground                    = 0x5e81ac,

    .urgent_background                      = 0xbf616a,
    .urgent_foreground                      = 0x2e3440,

    .active_button_background               = 0xd8dee9,
    .active_button_foreground               = 0x5e81ac,

    .inactive_button_background             = 0xd8dee9,
    .inactive_button_foreground             = 0x5e81ac,

    .active_button_hovered_background       = 0xd8dee9,
    .active_button_hovered_foreground       = 0x434c5e,

    .inactive_button_hovered_background     = 0xd8dee9,
    .inactive_button_hovered_foreground     = 0x434c5e,

    .button_icons = { "\uf057", "\uf065" },
    .label_fontname = "DejaVu Sans Mono:antialias=true:size=11",
    .icon_fontname = "Font Awesome 5 Free Solid:size=12",
    .shortcuts = {
        {Modkey | ShiftMask,    XK_q,   CallbackVoid,       {StopMainLoop}},
        {Modkey | ShiftMask,    XK_v,   CallbackClient,     {MaximizeClientVertically}},
        {Modkey | ShiftMask,    XK_h,   CallbackClient,     {MaximizeClientHorizontally}},
        {Modkey | ShiftMask,    XK_m,   CallbackClient,     {MaximizeClient}},
        //{Modkey | ShiftMask, XK_w, CallbackClient,  {MinimizeClient}},
        //{Modkey | ShiftMask, XK_f, CallbackClient,  {FullscreenClient}},
        {Modkey | ShiftMask,    XK_r,   CallbackClient,     {RestoreClient}},
        {Modkey,                XK_Tab, CallbackVoid,       {CycleForward}},
        {Modkey | ShiftMask,    XK_Tab, CallbackVoid,       {CycleBackward}}
    },
    .terminal = {"uxterm", NULL}
};

