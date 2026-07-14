/* Cross-platform gamepad/controller support for the unity client.
   Windows: XInput when available, plus WinMM joystick fallback for generic/PS adapters.
   Linux/SDL2: SDL_GameController when available, plus SDL_Joystick fallback. */

static int pex_str_contains_i(const char *hay, const char *needle) {
    if (!hay || !needle || !*needle) return 0;
    size_t nlen = strlen(needle);
    for (const char *p = hay; *p; ++p) {
        size_t i = 0;
        while (i < nlen && p[i]) {
            char a = p[i];
            char b = needle[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) break;
            ++i;
        }
        if (i == nlen) return 1;
    }
    return 0;
}

static float pex_deadzone(float v) {
    if (v > -PEX_GAMEPAD_DEADZONE && v < PEX_GAMEPAD_DEADZONE) return 0.0f;
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

#define PEX_TRIGGER_PRESS_THRESHOLD 0.35f
#define PEX_TRIGGER_RELEASE_THRESHOLD 0.18f
#define PEX_TRIGGER_RELEASE_GRACE_FRAMES 3

/* Browser/SDL gamepad trigger axes can occasionally report one or two zero
   samples while the physical trigger is still held.  Keep a small hysteresis
   and release grace so mining/using does not get cancelled by a transient
   Gamepad API sample.  A real release still takes effect within roughly 50 ms
   at 60 fps. */
static void pex_gamepad_filter_trigger(float value, int old_down, int old_release_frames,
                                       int *down, int *release_frames) {
    if (!down || !release_frames) return;
    if (!old_down) {
        *down = value >= PEX_TRIGGER_PRESS_THRESHOLD;
        *release_frames = 0;
        return;
    }
    if (value > PEX_TRIGGER_RELEASE_THRESHOLD) {
        *down = 1;
        *release_frames = 0;
        return;
    }

    int frames = old_release_frames + 1;
    if (frames >= PEX_TRIGGER_RELEASE_GRACE_FRAMES) {
        *down = 0;
        *release_frames = 0;
    } else {
        *down = 1;
        *release_frames = frames;
    }
}

static void pex_gamepad_update_trigger_buttons(PexGamepadState *pad, const PexGamepadState *old) {
    if (!pad || !old) return;
    pex_gamepad_filter_trigger(pad->lt, old->lt_down, old->lt_release_frames,
                               &pad->lt_down, &pad->lt_release_frames);
    pex_gamepad_filter_trigger(pad->rt, old->rt_down, old->rt_release_frames,
                               &pad->rt_down, &pad->rt_release_frames);
}

static void pex_input_focus_set(PexInputFocusMode mode) {
    if (g_input_focus_mode == mode) return;
    g_input_focus_mode = mode;
    if (mode != PEX_INPUT_FOCUS_GAMEPAD) {
        g_gamepad_virtual_cursor_active = 0;
        g_gamepad_menu_index = -1;
    }
}

static int gamepad_has_input(const PexGamepadState *p) {
    if (!p || !p->connected) return 0;
    if (fabsf(p->lx) > 0.35f || fabsf(p->ly) > 0.35f ||
        fabsf(p->rx) > 0.35f || fabsf(p->ry) > 0.35f ||
        p->lt_down || p->rt_down) return 1;
    return p->a || p->b || p->x || p->y || p->lb || p->rb ||
           p->back || p->start || p->guide || p->ls || p->rs ||
           p->dpad_up || p->dpad_down || p->dpad_left || p->dpad_right;
}

static void pex_gamepad_classify(PexGamepadState *pad, const char *name) {
    snprintf(pad->name, sizeof(pad->name), "%s", (name && *name) ? name : "Unknown controller");
    pad->is_xbox = pex_str_contains_i(pad->name, "xbox") ||
                   pex_str_contains_i(pad->name, "xinput") ||
                   pex_str_contains_i(pad->name, "x-box") ||
                   pex_str_contains_i(pad->name, "360 controller");
    pad->is_dualshock = pex_str_contains_i(pad->name, "dualshock") ||
                        pex_str_contains_i(pad->name, "dualsense") ||
                        pex_str_contains_i(pad->name, "playstation") ||
                        pex_str_contains_i(pad->name, "sony") ||
                        pex_str_contains_i(pad->name, "ps2") ||
                        pex_str_contains_i(pad->name, "ps3") ||
                        pex_str_contains_i(pad->name, "ps4") ||
                        pex_str_contains_i(pad->name, "ps5");
    if (pad->is_xbox) snprintf(pad->kind, sizeof(pad->kind), "Xbox/XInput");
    else if (pad->is_dualshock) snprintf(pad->kind, sizeof(pad->kind), "DualShock/PlayStation");
    else snprintf(pad->kind, sizeof(pad->kind), "Generic gamepad");
}

static void pex_gamepad_copy_edges(PexGamepadState *dst, const PexGamepadState *old) {
    dst->prev_a = old->a; dst->prev_b = old->b; dst->prev_x = old->x; dst->prev_y = old->y;
    dst->prev_lb = old->lb; dst->prev_rb = old->rb; dst->prev_back = old->back; dst->prev_start = old->start;
    dst->prev_lt = old->lt_down; dst->prev_rt = old->rt_down;
    dst->prev_dpad_up = old->dpad_up; dst->prev_dpad_down = old->dpad_down;
    dst->prev_dpad_left = old->dpad_left; dst->prev_dpad_right = old->dpad_right;
}

#if defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS) || defined(PEX_PLATFORM_XBOX_UWP)
static void pex_tv_remote_append_remote_pad(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    if (!g_opts.tv_remote_mapped && !pex_tv_remote_any_action_down()) return;
    if (!g_tv_remote_seen && !pex_tv_remote_any_action_down()) return;
    if (g_gamepad_count >= PEX_GAMEPAD_MAX) return;

    int slot = g_gamepad_count;
    PexGamepadState *p = &g_gamepads[slot];
    memset(p, 0, sizeof(*p));
    pex_gamepad_copy_edges(p, &oldpads[slot]);
    p->connected = 1;
    p->slot = slot;
    p->is_tv_remote = 1;
#if defined(PEX_PLATFORM_LGWEBOS)
    snprintf(p->name, sizeof(p->name), "LG webOS remote");
#elif defined(PEX_PLATFORM_XBOX_UWP)
    snprintf(p->name, sizeof(p->name), "Xbox media remote");
#else
    snprintf(p->name, sizeof(p->name), "Android TV remote");
#endif
    snprintf(p->kind, sizeof(p->kind), "TV remote");

    p->dpad_up = pex_tv_remote_action_down(PEX_TV_REMOTE_UP);
    p->dpad_down = pex_tv_remote_action_down(PEX_TV_REMOTE_DOWN);
    p->dpad_left = pex_tv_remote_action_down(PEX_TV_REMOTE_LEFT);
    p->dpad_right = pex_tv_remote_action_down(PEX_TV_REMOTE_RIGHT);
    /* Remote arrows are gameplay movement, never the Xbox-only chat/F5/fly
       shortcuts. Keep D-pad bits for menu focus and mirror them to the virtual
       left stick only while the game is in-world. */
    p->lx = p->dpad_left ? -1.0f : (p->dpad_right ? 1.0f : 0.0f);
    p->ly = p->dpad_up ? -1.0f : (p->dpad_down ? 1.0f : 0.0f);
    p->a = pex_tv_remote_action_down(PEX_TV_REMOTE_OK);
    p->back = pex_tv_remote_action_down(PEX_TV_REMOTE_BACK);
    p->rt = pex_tv_remote_action_down(PEX_TV_REMOTE_BREAK) ? 1.0f : 0.0f;
    p->lt = pex_tv_remote_action_down(PEX_TV_REMOTE_PLACE) ? 1.0f : 0.0f;
    p->y = pex_tv_remote_action_down(PEX_TV_REMOTE_INVENTORY);
    p->b = pex_tv_remote_action_down(PEX_TV_REMOTE_SNEAK);
    p->x = pex_tv_remote_action_down(PEX_TV_REMOTE_DROP);
    p->lb = pex_tv_remote_action_down(PEX_TV_REMOTE_HOTBAR_PREV);
    p->rb = pex_tv_remote_action_down(PEX_TV_REMOTE_HOTBAR_NEXT);

    g_gamepad_count++;
    g_gamepad_primary = slot;
}
#endif

#ifdef PEX_PLATFORM_SDL2
static SDL_GameController *g_sdl2_pads[PEX_GAMEPAD_MAX];
static SDL_Joystick *g_sdl2_joys[PEX_GAMEPAD_MAX];
static int g_sdl2_pad_is_controller[PEX_GAMEPAD_MAX];
static int g_sdl2_pad_device_index[PEX_GAMEPAD_MAX];
static int g_sdl2_pad_open_count = 0;
static int g_sdl2_last_device_count = -1;

static float sdl_axis_norm(Sint16 v) {
    float f = v < 0 ? (float)v / 32768.0f : (float)v / 32767.0f;
    return pex_deadzone(f);
}


static int gamepad_sdl2_should_ignore_device(int device_index) {
    const char *name = SDL_JoystickNameForIndex(device_index);
#ifdef PEX_PLATFORM_ANDROID
    /* Android phones expose the accelerometer as a joystick on some SDL builds.
       It is not a real controller and its changing axes make the camera drift. */
    if (pex_str_contains_i(name, "accelerometer")) return 1;
    if (pex_str_contains_i(name, "android accelerometer")) return 1;
#endif
    return 0;
}

static void gamepad_sdl2_close_all(void) {
    for (int i = 0; i < PEX_GAMEPAD_MAX; i++) {
        if (g_sdl2_pads[i]) { SDL_GameControllerClose(g_sdl2_pads[i]); g_sdl2_pads[i] = NULL; }
        else if (g_sdl2_joys[i]) { SDL_JoystickClose(g_sdl2_joys[i]); g_sdl2_joys[i] = NULL; }
        g_sdl2_pad_is_controller[i] = 0;
        g_sdl2_pad_device_index[i] = -1;
    }
    g_sdl2_pad_open_count = 0;
}

static int gamepad_sdl2_all_attached(void) {
    for (int i = 0; i < g_sdl2_pad_open_count; i++) {
        if (g_sdl2_pad_is_controller[i]) {
            if (!g_sdl2_pads[i] || !SDL_GameControllerGetAttached(g_sdl2_pads[i])) return 0;
        } else {
            if (!g_sdl2_joys[i] || !SDL_JoystickGetAttached(g_sdl2_joys[i])) return 0;
        }
    }
    return 1;
}

static void pex_gamepad_scan_devices(void) {
    double now = now_seconds();
    int attached = gamepad_sdl2_all_attached();

    /* Do not close and reopen live controllers on every one-second probe.
       That old behavior caused SDL/Emscripten to emit a zero trigger sample,
       which looked like an LT/RT release and cancelled held actions. */
    if (attached && g_sdl2_last_device_count >= 0 &&
        now - g_gamepad_probe_last_time < 1.0) return;

    int total = SDL_NumJoysticks();
    g_gamepad_probe_last_time = now;
    if (attached && total == g_sdl2_last_device_count) return;

    gamepad_sdl2_close_all();
    g_sdl2_last_device_count = total;
    for (int i = 0; i < total && g_sdl2_pad_open_count < PEX_GAMEPAD_MAX; i++) {
        if (gamepad_sdl2_should_ignore_device(i)) continue;
        int slot = g_sdl2_pad_open_count;
        if (SDL_IsGameController(i)) {
            SDL_GameController *gc = SDL_GameControllerOpen(i);
            if (!gc) continue;
            g_sdl2_pads[slot] = gc;
            g_sdl2_pad_is_controller[slot] = 1;
            g_sdl2_pad_device_index[slot] = i;
            g_sdl2_pad_open_count++;
        } else {
            SDL_Joystick *joy = SDL_JoystickOpen(i);
            if (!joy) continue;
            g_sdl2_joys[slot] = joy;
            g_sdl2_pad_is_controller[slot] = 0;
            g_sdl2_pad_device_index[slot] = i;
            g_sdl2_pad_open_count++;
        }
    }
}

static void pex_gamepad_platform_poll(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    pex_gamepad_scan_devices();
    SDL_GameControllerUpdate();
    SDL_JoystickUpdate();
    g_gamepad_count = 0;
    g_gamepad_primary = -1;
    memset(g_gamepads, 0, sizeof(g_gamepads));
    for (int i = 0; i < g_sdl2_pad_open_count && g_gamepad_count < PEX_GAMEPAD_MAX; i++) {
        PexGamepadState *p = &g_gamepads[g_gamepad_count];
        p->connected = 1;
        p->slot = i;
        pex_gamepad_copy_edges(p, &oldpads[i]);
        if (g_sdl2_pad_is_controller[i] && g_sdl2_pads[i]) {
            SDL_GameController *gc = g_sdl2_pads[i];
            pex_gamepad_classify(p, SDL_GameControllerName(gc));
            p->lx = sdl_axis_norm(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX));
            p->ly = sdl_axis_norm(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY));
            p->rx = sdl_axis_norm(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX));
            p->ry = sdl_axis_norm(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY));
            p->lt = (float)SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f;
            p->rt = (float)SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f;
            if (p->lt < 0.0f) p->lt = 0.0f; if (p->rt < 0.0f) p->rt = 0.0f;
            p->a = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A);
            p->b = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B);
            p->x = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X);
            p->y = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y);
            p->lb = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            p->rb = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            p->back = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_BACK);
            p->start = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_START);
            p->guide = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_GUIDE);
            p->ls = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSTICK);
            p->rs = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
            p->dpad_up = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP);
            p->dpad_down = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            p->dpad_left = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            p->dpad_right = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        } else if (g_sdl2_joys[i]) {
            SDL_Joystick *joy = g_sdl2_joys[i];
            pex_gamepad_classify(p, SDL_JoystickName(joy));
            int axes = SDL_JoystickNumAxes(joy);
            int buttons = SDL_JoystickNumButtons(joy);
            p->lx = axes > 0 ? sdl_axis_norm(SDL_JoystickGetAxis(joy, 0)) : 0.0f;
            p->ly = axes > 1 ? sdl_axis_norm(SDL_JoystickGetAxis(joy, 1)) : 0.0f;
            p->rx = axes > 2 ? sdl_axis_norm(SDL_JoystickGetAxis(joy, 2)) : 0.0f;
            p->ry = axes > 3 ? sdl_axis_norm(SDL_JoystickGetAxis(joy, 3)) : 0.0f;
            p->lt = axes > 4 ? (sdl_axis_norm(SDL_JoystickGetAxis(joy, 4)) + 1.0f) * 0.5f : 0.0f;
            p->rt = axes > 5 ? (sdl_axis_norm(SDL_JoystickGetAxis(joy, 5)) + 1.0f) * 0.5f : 0.0f;
            p->a = buttons > 0 ? SDL_JoystickGetButton(joy, 0) : 0;
            p->b = buttons > 1 ? SDL_JoystickGetButton(joy, 1) : 0;
            p->x = buttons > 2 ? SDL_JoystickGetButton(joy, 2) : 0;
            p->y = buttons > 3 ? SDL_JoystickGetButton(joy, 3) : 0;
            p->lb = buttons > 4 ? SDL_JoystickGetButton(joy, 4) : 0;
            p->rb = buttons > 5 ? SDL_JoystickGetButton(joy, 5) : 0;
            p->back = buttons > 6 ? SDL_JoystickGetButton(joy, 6) : 0;
            p->start = buttons > 7 ? SDL_JoystickGetButton(joy, 7) : 0;
            int hats = SDL_JoystickNumHats(joy);
            if (hats > 0) {
                Uint8 h = SDL_JoystickGetHat(joy, 0);
                p->dpad_up = (h & SDL_HAT_UP) != 0;
                p->dpad_down = (h & SDL_HAT_DOWN) != 0;
                p->dpad_left = (h & SDL_HAT_LEFT) != 0;
                p->dpad_right = (h & SDL_HAT_RIGHT) != 0;
            }
        }
        if (g_gamepad_primary < 0) g_gamepad_primary = g_gamepad_count;
        g_gamepad_count++;
    }
#if defined(PEX_PLATFORM_ANDROID_TV) || defined(PEX_PLATFORM_LGWEBOS)
    if (g_opts.tv_remote_mapped) pex_tv_remote_append_remote_pad(oldpads);
    else {
#ifdef PEX_PLATFORM_ANDROID_TV
        pex_android_tv_append_remote_pad(oldpads);
#endif
    }
#endif
#ifdef PEX_PLATFORM_ANDROID
    (void)oldpads; /* Android touch is handled as touch/mouse focus, not as a controller. */
#endif
}


#elif defined(PEX_PLATFORM_WII)
static void pex_gamepad_platform_poll(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    wii_poll_gamepads(oldpads);
}

#elif defined(PEX_PLATFORM_PSP)
static unsigned int g_psp_buttons_prev_raw = 0;

static float psp_axis_deadzone(int v) {
    /* PSP analog range is 0..255, center around 128. */
    float f = ((float)v - 128.0f) / 127.0f;
    return pex_deadzone(f);
}

static void pex_gamepad_platform_poll(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    sceCtrlPeekBufferPositive(&pad, 1);
    g_gamepad_count = 0;
    g_gamepad_primary = -1;
    memset(g_gamepads, 0, sizeof(g_gamepads));

    int slot = 0;
    PexGamepadState *p = &g_gamepads[slot];
    pex_gamepad_copy_edges(p, &oldpads[slot]);
    p->connected = 1;
    p->slot = 0;
    snprintf(p->name, sizeof(p->name), "PSP built-in controls");
    snprintf(p->kind, sizeof(p->kind), "PSP controls");

    unsigned int b = pad.Buttons;
    p->lx = psp_axis_deadzone((int)pad.Lx);
    p->ly = psp_axis_deadzone((int)pad.Ly);

    /* PSP has no right analog stick.  The four face buttons emulate it while held. */
    p->rx = 0.0f;
    p->ry = 0.0f;
    if (b & PSP_CTRL_SQUARE)   p->rx -= 1.0f; /* look left */
    if (b & PSP_CTRL_CIRCLE)   p->rx += 1.0f; /* look right */
    if (b & PSP_CTRL_TRIANGLE) p->ry -= 1.0f; /* look up */
    if (b & PSP_CTRL_CROSS)    p->ry += 1.0f; /* look down */

    /* Menu/cursor abstraction only.  Gameplay ignores A/B/X/Y so face buttons
       remain camera-only in-game, while menus/inventory still work normally. */
    p->a = (b & PSP_CTRL_CROSS) != 0;
    p->b = (b & PSP_CTRL_CIRCLE) != 0;
    p->x = (b & PSP_CTRL_SQUARE) != 0;
    p->y = (b & PSP_CTRL_TRIANGLE) != 0;
    p->lb = (b & PSP_CTRL_LTRIGGER) != 0;
    p->rb = (b & PSP_CTRL_RTRIGGER) != 0;
    p->back = (b & PSP_CTRL_SELECT) != 0;  /* gameplay jump */
    p->start = (b & PSP_CTRL_START) != 0;  /* pause */
    p->lt = p->lb ? 1.0f : 0.0f;
    p->rt = p->rb ? 1.0f : 0.0f;
    p->dpad_up = (b & PSP_CTRL_UP) != 0;
    p->dpad_down = (b & PSP_CTRL_DOWN) != 0;
    p->dpad_left = (b & PSP_CTRL_LEFT) != 0;
    p->dpad_right = (b & PSP_CTRL_RIGHT) != 0;

    g_gamepad_primary = 0;
    g_gamepad_count = 1;
    g_psp_buttons_prev_raw = b;
}

#elif defined(PEX_PLATFORM_XBOX_UWP)
extern int pex_xbox_uwp_gamepad_count(void);
extern int pex_xbox_uwp_gamepad_read(int index, float *lx, float *ly, float *rx, float *ry,
                                     float *lt, float *rt, unsigned int *buttons);

#define PEX_UWP_PAD_A          (1u << 0)
#define PEX_UWP_PAD_B          (1u << 1)
#define PEX_UWP_PAD_X          (1u << 2)
#define PEX_UWP_PAD_Y          (1u << 3)
#define PEX_UWP_PAD_LB         (1u << 4)
#define PEX_UWP_PAD_RB         (1u << 5)
#define PEX_UWP_PAD_BACK       (1u << 6)
#define PEX_UWP_PAD_START      (1u << 7)
#define PEX_UWP_PAD_LS         (1u << 8)
#define PEX_UWP_PAD_RS         (1u << 9)
#define PEX_UWP_PAD_DPAD_UP    (1u << 10)
#define PEX_UWP_PAD_DPAD_DOWN  (1u << 11)
#define PEX_UWP_PAD_DPAD_LEFT  (1u << 12)
#define PEX_UWP_PAD_DPAD_RIGHT (1u << 13)

static void pex_gamepad_platform_poll(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    g_gamepad_count = 0;
    g_gamepad_primary = -1;
    memset(g_gamepads, 0, sizeof(g_gamepads));

    int count = pex_xbox_uwp_gamepad_count();
    if (count > PEX_GAMEPAD_MAX) count = PEX_GAMEPAD_MAX;
    for (int i = 0; i < count; ++i) {
        float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f, lt = 0.0f, rt = 0.0f;
        unsigned int buttons = 0;
        if (!pex_xbox_uwp_gamepad_read(i, &lx, &ly, &rx, &ry, &lt, &rt, &buttons)) continue;
        int slot = g_gamepad_count;
        PexGamepadState *p = &g_gamepads[slot];
        pex_gamepad_copy_edges(p, &oldpads[slot]);
        p->connected = 1;
        p->slot = i;
        pex_gamepad_classify(p, "Xbox UWP Gamepad");
        p->lx = pex_deadzone(lx); p->ly = pex_deadzone(ly);
        p->rx = pex_deadzone(rx); p->ry = pex_deadzone(ry);
        p->lt = lt; p->rt = rt;
        p->a = (buttons & PEX_UWP_PAD_A) != 0;
        p->b = (buttons & PEX_UWP_PAD_B) != 0;
        p->x = (buttons & PEX_UWP_PAD_X) != 0;
        p->y = (buttons & PEX_UWP_PAD_Y) != 0;
        p->lb = (buttons & PEX_UWP_PAD_LB) != 0;
        p->rb = (buttons & PEX_UWP_PAD_RB) != 0;
        p->back = (buttons & PEX_UWP_PAD_BACK) != 0;
        p->start = (buttons & PEX_UWP_PAD_START) != 0;
        p->ls = (buttons & PEX_UWP_PAD_LS) != 0;
        p->rs = (buttons & PEX_UWP_PAD_RS) != 0;
        p->dpad_up = (buttons & PEX_UWP_PAD_DPAD_UP) != 0;
        p->dpad_down = (buttons & PEX_UWP_PAD_DPAD_DOWN) != 0;
        p->dpad_left = (buttons & PEX_UWP_PAD_DPAD_LEFT) != 0;
        p->dpad_right = (buttons & PEX_UWP_PAD_DPAD_RIGHT) != 0;
        if (g_gamepad_primary < 0) g_gamepad_primary = slot;
        g_gamepad_count++;
    }
    pex_tv_remote_append_remote_pad(oldpads);
}
#else
/* Minimal dynamic XInput declarations: avoids an xinput import dependency. */
typedef struct PexXInputGamepad {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    short sThumbLX;
    short sThumbLY;
    short sThumbRX;
    short sThumbRY;
} PexXInputGamepad;
typedef struct PexXInputState { DWORD dwPacketNumber; PexXInputGamepad Gamepad; } PexXInputState;
typedef DWORD (WINAPI *PexXInputGetStateFn)(DWORD, PexXInputState*);
static HMODULE g_xinput_dll = NULL;
static PexXInputGetStateFn g_xinput_get_state = NULL;
#define PEX_XINPUT_DPAD_UP        0x0001
#define PEX_XINPUT_DPAD_DOWN      0x0002
#define PEX_XINPUT_DPAD_LEFT      0x0004
#define PEX_XINPUT_DPAD_RIGHT     0x0008
#define PEX_XINPUT_START          0x0010
#define PEX_XINPUT_BACK           0x0020
#define PEX_XINPUT_LEFT_THUMB     0x0040
#define PEX_XINPUT_RIGHT_THUMB    0x0080
#define PEX_XINPUT_LEFT_SHOULDER  0x0100
#define PEX_XINPUT_RIGHT_SHOULDER 0x0200
#define PEX_XINPUT_A              0x1000
#define PEX_XINPUT_B              0x2000
#define PEX_XINPUT_X              0x4000
#define PEX_XINPUT_Y              0x8000

static float xinput_axis(short v) {
    float f = v < 0 ? (float)v / 32768.0f : (float)v / 32767.0f;
    return pex_deadzone(f);
}

static void pex_gamepad_load_xinput(void) {
    if (g_xinput_get_state) return;
    const char *dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
    for (int i = 0; i < ARRAY_COUNT(dlls); i++) {
        g_xinput_dll = LoadLibraryA(dlls[i]);
        if (g_xinput_dll) {
            g_xinput_get_state = (PexXInputGetStateFn)GetProcAddress(g_xinput_dll, "XInputGetState");
            if (g_xinput_get_state) return;
            FreeLibrary(g_xinput_dll);
            g_xinput_dll = NULL;
        }
    }
}

static float joy_axis(DWORD v, DWORD minv, DWORD maxv) {
    if (maxv <= minv) return 0.0f;
    float f = ((float)(v - minv) / (float)(maxv - minv)) * 2.0f - 1.0f;
    return pex_deadzone(f);
}

static void pex_gamepad_platform_poll(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    static double xinput_retry_after[4] = {0, 0, 0, 0};
    static int xinput_connected[4] = {0, 0, 0, 0};
    static double winmm_retry_after = 0.0;
    static UINT winmm_last_device_count = 0;
    static double winmm_slot_retry_after[16];
    static int winmm_slot_connected[16];
    double now = now_seconds();

    pex_gamepad_load_xinput();
    g_gamepad_count = 0;
    g_gamepad_primary = -1;
    memset(g_gamepads, 0, sizeof(g_gamepads));
    if (g_xinput_get_state) {
        for (DWORD i = 0; i < 4 && g_gamepad_count < PEX_GAMEPAD_MAX; i++) {
            PexXInputState st;
            DWORD xr;
            /* XInputGetState can be surprisingly expensive for empty slots on
               some Windows machines/drivers.  Poll connected pads every frame,
               but only re-probe known-empty slots once per second. */
            if (!xinput_connected[i] && now < xinput_retry_after[i]) {
                if (g_loggy_enabled) g_loggy_xinput_skipped++;
                continue;
            }
            memset(&st, 0, sizeof(st));
            if (g_loggy_enabled) g_loggy_xinput_calls++;
            xr = g_xinput_get_state(i, &st);
            if (xr != 0) {
                if (g_loggy_enabled) g_loggy_xinput_fail++;
                xinput_connected[i] = 0;
                xinput_retry_after[i] = now + 1.0;
                continue;
            }
            if (g_loggy_enabled) g_loggy_xinput_ok++;
            xinput_connected[i] = 1;
            xinput_retry_after[i] = now;
            {
                int slot = g_gamepad_count;
                PexGamepadState *p = &g_gamepads[slot];
                pex_gamepad_copy_edges(p, &oldpads[slot]);
                p->connected = 1; p->slot = (int)i;
                char nm[64]; snprintf(nm, sizeof(nm), "Xbox/XInput Controller %lu", (unsigned long)i + 1);
                pex_gamepad_classify(p, nm);
                WORD b = st.Gamepad.wButtons;
                p->lx = xinput_axis(st.Gamepad.sThumbLX);
                p->ly = -xinput_axis(st.Gamepad.sThumbLY);
                p->rx = xinput_axis(st.Gamepad.sThumbRX);
                p->ry = -xinput_axis(st.Gamepad.sThumbRY);
                p->lt = (float)st.Gamepad.bLeftTrigger / 255.0f;
                p->rt = (float)st.Gamepad.bRightTrigger / 255.0f;
                p->a = (b & PEX_XINPUT_A) != 0; p->b = (b & PEX_XINPUT_B) != 0;
                p->x = (b & PEX_XINPUT_X) != 0; p->y = (b & PEX_XINPUT_Y) != 0;
                p->lb = (b & PEX_XINPUT_LEFT_SHOULDER) != 0; p->rb = (b & PEX_XINPUT_RIGHT_SHOULDER) != 0;
                p->back = (b & PEX_XINPUT_BACK) != 0; p->start = (b & PEX_XINPUT_START) != 0;
                p->ls = (b & PEX_XINPUT_LEFT_THUMB) != 0; p->rs = (b & PEX_XINPUT_RIGHT_THUMB) != 0;
                p->dpad_up = (b & PEX_XINPUT_DPAD_UP) != 0; p->dpad_down = (b & PEX_XINPUT_DPAD_DOWN) != 0;
                p->dpad_left = (b & PEX_XINPUT_DPAD_LEFT) != 0; p->dpad_right = (b & PEX_XINPUT_DPAD_RIGHT) != 0;
                if (g_gamepad_primary < 0) g_gamepad_primary = slot;
                g_gamepad_count++;
            }
        }
    }
    /* WinMM joystick fallback for DirectInput-like pads, DualShock adapters, and old USB converters.
       When no fallback pad is present, scanning every frame can cost several ms. */
    UINT n = winmm_last_device_count;
    if (now >= winmm_retry_after) {
        if (g_loggy_enabled) g_loggy_winmm_numdev_calls++;
        n = joyGetNumDevs();
        winmm_last_device_count = n;
        winmm_retry_after = now + 1.0;
    }
    for (UINT i = 0; i < n && g_gamepad_count < PEX_GAMEPAD_MAX; i++) {
        JOYINFOEX ji;
        MMRESULT jr;
        if (i < 16 && !winmm_slot_connected[i] && now < winmm_slot_retry_after[i]) {
            if (g_loggy_enabled) g_loggy_winmm_slots_skipped++;
            continue;
        }
        memset(&ji, 0, sizeof(ji)); ji.dwSize = sizeof(ji); ji.dwFlags = JOY_RETURNALL;
        if (g_loggy_enabled) g_loggy_winmm_pos_calls++;
        jr = joyGetPosEx(i, &ji);
        if (jr != JOYERR_NOERROR) {
            if (g_loggy_enabled) g_loggy_winmm_pos_fail++;
            if (i < 16) {
                winmm_slot_connected[i] = 0;
                winmm_slot_retry_after[i] = now + 1.0;
            }
            continue;
        }
        if (g_loggy_enabled) g_loggy_winmm_pos_ok++;
        if (i < 16) {
            winmm_slot_connected[i] = 1;
            winmm_slot_retry_after[i] = now;
        }
        JOYCAPSA jc; memset(&jc, 0, sizeof(jc));
        if (g_loggy_enabled) g_loggy_winmm_caps_calls++;
        joyGetDevCapsA(i, &jc, sizeof(jc));
        int slot = g_gamepad_count;
        PexGamepadState *p = &g_gamepads[slot];
        pex_gamepad_copy_edges(p, &oldpads[slot]);
        p->connected = 1; p->slot = (int)i;
        pex_gamepad_classify(p, jc.szPname[0] ? jc.szPname : "WinMM joystick");
        p->lx = joy_axis(ji.dwXpos, jc.wXmin, jc.wXmax);
        p->ly = joy_axis(ji.dwYpos, jc.wYmin, jc.wYmax);
        p->rx = joy_axis(ji.dwRpos, jc.wRmin, jc.wRmax);
        p->ry = joy_axis(ji.dwUpos, jc.wUmin, jc.wUmax);
        p->lt = 0.0f; p->rt = 0.0f;
        DWORD btn = ji.dwButtons;
        p->a = (btn & (1u << 0)) != 0; p->b = (btn & (1u << 1)) != 0;
        p->x = (btn & (1u << 2)) != 0; p->y = (btn & (1u << 3)) != 0;
        p->lb = (btn & (1u << 4)) != 0; p->rb = (btn & (1u << 5)) != 0;
        p->back = (btn & (1u << 6)) != 0; p->start = (btn & (1u << 7)) != 0;
        if (ji.dwPOV != JOY_POVCENTERED) {
            DWORD pov = ji.dwPOV;
            p->dpad_up = (pov >= 31500 || pov <= 4500);
            p->dpad_right = (pov >= 4500 && pov <= 13500);
            p->dpad_down = (pov >= 13500 && pov <= 22500);
            p->dpad_left = (pov >= 22500 && pov <= 31500);
        }
        if (g_gamepad_primary < 0) g_gamepad_primary = slot;
        g_gamepad_count++;
    }
}
#endif

static int pex_gamepad_menu_screen(void) {
    switch (g_screen) {
        case SCREEN_TITLE:
        case SCREEN_OPTIONS:
        case SCREEN_OPTIONS_MORE:
        case SCREEN_STIVUFINE:
        case SCREEN_STIVUFINE_DETAILS:
        case SCREEN_STIVUFINE_QUALITY:
        case SCREEN_STIVUFINE_ANIMATIONS:
        case SCREEN_STIVUFINE_PERFORMANCE:
        case SCREEN_STIVUFINE_OTHER:
        case SCREEN_ASSETS:
        case SCREEN_LANGUAGE:
        case SCREEN_SET_NAME:
        case SCREEN_TV_REMOTE_MAP:
        case SCREEN_SYSTEM_INFO:
        case SCREEN_SKINS:
        case SCREEN_CONTROLS:
        case SCREEN_WORLD_SELECT:
        case SCREEN_CREATE_WORLD:
        case SCREEN_WORLD_TYPE:
        case SCREEN_WORLD_DELETE:
        case SCREEN_RENAME_WORLD:
        case SCREEN_CONFIRM_DELETE:
        case SCREEN_MULTIPLAYER:
        case SCREEN_CONNECTING:
        case SCREEN_TEXPACK:
        case SCREEN_TEXPACK_INSTALL:
        case SCREEN_SAVING_QUIT:
        case SCREEN_PAUSE:
        case SCREEN_ACHIEVEMENTS:
        case SCREEN_STATISTICS:
        case SCREEN_DEATH:
        case SCREEN_VIRTUAL_KEYBOARD:
        case SCREEN_NOTICE:
        case SCREEN_RENDERER_RESTART_PROMPT:
        case SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT:
        case SCREEN_CLASSIC_PACK_WARNING:
            return 1;
        default:
            return 0;
    }
}

static int pex_gamepad_inventory_screen(void) {
    return g_screen == SCREEN_INVENTORY || g_screen == SCREEN_CREATIVE ||
           g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE ||
           g_screen == SCREEN_CHEST;
}

static PexGamepadState *pex_gamepad_primary_pad(void) {
    if (g_gamepad_primary >= 0 && g_gamepad_primary < g_gamepad_count) return &g_gamepads[g_gamepad_primary];
    return NULL;
}

static int pex_xinput_hud_active(void) {
    for (int i = 0; i < g_gamepad_count && i < PEX_GAMEPAD_MAX; ++i) {
        if (g_gamepads[i].connected && g_gamepads[i].is_xbox) return 1;
    }
    return 0;
}

static int pex_xinput_hud_bottom_offset(void) {
    if (g_screen != SCREEN_INGAME && g_screen != SCREEN_CHAT) return 0;
    /* At 640x480 this places the hotbar at y=428 and leaves the y=455 prompt
       row below it, exactly matching the supplied controller HUD layout. */
    return pex_xinput_hud_active() ? 30 : 0;
}

static void pex_gamepad_rebuild_virtual_keys(PexGamepadState *p) {
    memset(g_gamepad_vk_state, 0, sizeof(g_gamepad_vk_state));
    if (g_player_dead) {
        g_gamepad_sneak_toggled = 0;
        return;
    }
    if (!p || g_screen != SCREEN_INGAME) return;
#ifdef PEX_PLATFORM_PSP
    /* PSP gameplay mapping is intentionally different from menu/inventory mapping.
       Menus still use Cross=select, Circle=back, D-pad/analog navigation.
       In-game, the four face buttons are camera ONLY, so never use A/B/X/Y
       here for jump, sneak, inventory, attack, or UI. */
    if (p->ly < -PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[0] & 511] = 1; /* analog forward */
    if (p->ly >  PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[2] & 511] = 1; /* analog back */
    if (p->lx < -PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[1] & 511] = 1; /* analog left */
    if (p->lx >  PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[3] & 511] = 1; /* analog right */
    if (p->back) g_gamepad_vk_state[g_opts.keys[4] & 511] = 1;             /* Select = jump */
    if (p->dpad_down) g_gamepad_vk_state[g_opts.keys[5] & 511] = 1;        /* D-pad Down = sneak */
    if (p->rt_down) g_gamepad_vk_state[VK_LBUTTON] = 1;                  /* R = break/attack */
    if (p->lb) g_gamepad_vk_state[VK_RBUTTON] = 1;                        /* L = place/use */
#else
    /* Minecraft controller movement belongs exclusively to the left stick.
       The D-pad must never synthesize forward/back/strafe input.  On the
       legacy-console control scheme it only doubles as fly up/down while the
       player is already flying in Creative mode; menus and inventories keep
       their separate D-pad navigation paths below. */
    if (p->ly < -PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[0] & 511] = 1;
    if (p->ly >  PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[2] & 511] = 1;
    if (p->lx < -PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[1] & 511] = 1;
    if (p->lx >  PEX_GAMEPAD_DEADZONE) g_gamepad_vk_state[g_opts.keys[3] & 511] = 1;
    if (p->b && !p->prev_b) g_gamepad_sneak_toggled = !g_gamepad_sneak_toggled;
    /* Legacy-console D-pad shortcuts belong only to a real Xbox/XInput pad.
       Android TV remotes and generic keyboard/navigation sources may use arrow
       directions for movement, but must never trigger fly/chat/F5 actions. */
    if (p->a || (p->is_xbox && g_creative_flying && p->dpad_up)) g_gamepad_vk_state[g_opts.keys[4] & 511] = 1; /* jump/fly up */
    if (g_gamepad_sneak_toggled || p->ls || (p->is_xbox && g_creative_flying && p->dpad_down))
        g_gamepad_vk_state[g_opts.keys[5] & 511] = 1; /* B toggle, LS hold, or fly down */
    /* RT is break/attack. RB must not mirror RT; RB is reserved for hotbar next. */
    if (p->rt_down) g_gamepad_vk_state[VK_LBUTTON] = 1;         /* break/attack */
    if (p->lt_down) g_gamepad_vk_state[VK_RBUTTON] = 1;         /* place/use */
#endif
}

static ScreenId g_gamepad_last_ui_screen = (ScreenId)-1;
static int g_gamepad_language_index = -1;
static int g_gamepad_texpack_index = -1;
static int g_gamepad_statistics_sort_focus = 0;
static double g_gamepad_ui_scroll_last_time = 0.0;

static int pex_gamepad_pressed(int down, int prev) { return down && !prev; }

static void pex_gamepad_back_action(void) {
    handle_keydown((WPARAM)VK_ESCAPE);
}

static void pex_gamepad_click_button(Button *b) {
    if (!b || !b->enabled) return;
    g_mouse_x = b->x + b->w / 2;
    g_mouse_y = b->y + b->h / 2;
    mouse_down(g_mouse_x, g_mouse_y);
    mouse_up(g_mouse_x, g_mouse_y);
}

static int pex_gamepad_button_focusable(const Button *b) {
    return b && b->visible && b->enabled;
}

static Button *pex_gamepad_selected_button(void) {
    if (g_button_count <= 0) return NULL;
    if (g_gamepad_menu_index < 0 || g_gamepad_menu_index >= g_button_count)
        g_gamepad_menu_index = 0;
    for (int pass = 0; pass < g_button_count; pass++) {
        int idx = (g_gamepad_menu_index + pass) % g_button_count;
        if (pex_gamepad_button_focusable(&g_buttons[idx])) {
            g_gamepad_menu_index = idx;
            return &g_buttons[idx];
        }
    }
    return NULL;
}

static float pex_gamepad_interval_gap(float a0, float a1, float b0, float b1) {
    if (a1 < b0) return b0 - a1;
    if (b1 < a0) return a0 - b1;
    return 0.0f;
}

/* Navigate by actual screen geometry, not button IDs or construction order.
   Controls whose horizontal/vertical spans overlap are strongly preferred,
   so Down selects the button visually underneath instead of a closer diagonal
   control.  Edges intentionally do not wrap. */
static void pex_gamepad_nav_spatial(int dx, int dy) {
    Button *cur = pex_gamepad_selected_button();
    if (!cur || (!dx && !dy)) return;
    float cx = (float)cur->x + (float)cur->w * 0.5f;
    float cy = (float)cur->y + (float)cur->h * 0.5f;
    int best = -1;
    double best_score = 1.0e30;

    for (int i = 0; i < g_button_count; ++i) {
        Button *b = &g_buttons[i];
        if (b == cur || !pex_gamepad_button_focusable(b)) continue;
        float bx = (float)b->x + (float)b->w * 0.5f;
        float by = (float)b->y + (float)b->h * 0.5f;
        float forward;
        float cross_gap;
        float cross_center;
        if (dy != 0) {
            if ((dy > 0 && by <= cy + 1.0f) || (dy < 0 && by >= cy - 1.0f)) continue;
            forward = dy > 0
                ? (float)b->y - (float)(cur->y + cur->h)
                : (float)cur->y - (float)(b->y + b->h);
            if (forward < 0.0f) forward = 0.0f;
            cross_gap = pex_gamepad_interval_gap((float)cur->x, (float)(cur->x + cur->w),
                                                 (float)b->x, (float)(b->x + b->w));
            cross_center = fabsf(bx - cx);
        } else {
            if ((dx > 0 && bx <= cx + 1.0f) || (dx < 0 && bx >= cx - 1.0f)) continue;
            forward = dx > 0
                ? (float)b->x - (float)(cur->x + cur->w)
                : (float)cur->x - (float)(b->x + b->w);
            if (forward < 0.0f) forward = 0.0f;
            cross_gap = pex_gamepad_interval_gap((float)cur->y, (float)(cur->y + cur->h),
                                                 (float)b->y, (float)(b->y + b->h));
            cross_center = fabsf(by - cy);
        }
        /* Crossing out of the current row/column costs far more than moving
           farther along it; center distance breaks ties between overlaps. */
        double score = (double)cross_gap * 100000.0 +
                       (double)forward * 100.0 +
                       (double)cross_center;
        if (score < best_score) { best_score = score; best = i; }
    }

    if (best >= 0) {
        g_gamepad_menu_index = best;
        g_mouse_x = g_buttons[best].x + g_buttons[best].w / 2;
        g_mouse_y = g_buttons[best].y + g_buttons[best].h / 2;
        pex_sound_play("random.click", 0.35f, 1.8f);
    }
}

static void pex_gamepad_adjust_slider(Button *b, float delta) {
    if (!b || b->kind != BUTTON_SLIDER || !b->enabled) return;
    float v = b->slider_value + delta;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    int mx = b->x + 4 + (int)(v * (float)(b->w - 8));
    update_slider(b, mx);
    g_mouse_x = mx;
    g_mouse_y = b->y + b->h / 2;
}

static void pex_gamepad_reset_ui_state_if_needed(void) {
    if (g_gamepad_last_ui_screen == g_screen) return;
    g_gamepad_last_ui_screen = g_screen;
    g_gamepad_menu_index = 0;
    g_gamepad_virtual_cursor_active = 0;
    if (g_screen == SCREEN_LANGUAGE) g_gamepad_language_index = pex_current_language_index();
    if (g_screen == SCREEN_TEXPACK) g_gamepad_texpack_index = g_selected_texpack;
    if (g_screen == SCREEN_STATISTICS) g_gamepad_statistics_sort_focus = 0;
}

static int pex_gamepad_world_screen_update(PexGamepadState *p, double now) {
    if (!p || (g_screen != SCREEN_WORLD_SELECT && g_screen != SCREEN_WORLD_DELETE)) return 0;

    if (g_world_save_count <= 0) scan_world_saves();
    if (g_world_save_count > 0 && (g_selected_world_index < 0 || g_selected_world_index >= g_world_save_count)) {
        world_save_select_index(0);
        rebuild_screen();
    }

    int nav_down = p->dpad_down || p->ly > 0.55f || p->ry > 0.55f;
    int nav_up = p->dpad_up || p->ly < -0.55f || p->ry < -0.55f;
    if ((nav_down || nav_up) && now - g_gamepad_nav_last_time > 0.16) {
        world_save_select_relative(nav_down ? 1 : -1);
        g_gamepad_nav_last_time = now;
    }

    int cx, cy;
    if (world_save_selected_row_center(&cx, &cy)) { g_mouse_x = cx; g_mouse_y = cy; }

    if (pex_gamepad_pressed(p->a, p->prev_a) && now - g_gamepad_click_last_time > 0.10) {
        if (g_screen == SCREEN_WORLD_SELECT) start_selected_world_save();
        else confirm_delete_world_save();
        g_gamepad_click_last_time = now;
    }
    if (pex_gamepad_pressed(p->x, p->prev_x) && g_screen == SCREEN_WORLD_SELECT) {
        create_world_prepare_defaults();
        set_screen(SCREEN_CREATE_WORLD);
    }
    if (pex_gamepad_pressed(p->y, p->prev_y) && g_screen == SCREEN_WORLD_SELECT &&
        g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count) {
        WorldSaveEntry *e = &g_world_saves[g_selected_world_index];
        snprintf(g_rename_world_text, sizeof(g_rename_world_text), "%s",
                 e->display_name[0] ? e->display_name : e->dir_name);
        set_screen(SCREEN_RENAME_WORLD);
    }
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) world_save_select_relative(-5);
    if (pex_gamepad_pressed(p->rb, p->prev_rb)) world_save_select_relative(5);
    if (pex_gamepad_pressed(p->lt_down, p->prev_lt) && g_screen == SCREEN_WORLD_SELECT &&
        g_selected_world_index >= 0 && g_selected_world_index < g_world_save_count) {
        set_screen(SCREEN_CONFIRM_DELETE);
    }
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back))
        pex_gamepad_back_action();
    return 1;
}

static void pex_gamepad_language_ensure_visible(int idx) {
    int view = pex_language_view_height() - 4;
    int y0 = idx * pex_language_slot_height();
    int y1 = y0 + pex_language_slot_height();
    if (y0 < g_language_scroll) g_language_scroll = y0;
    if (y1 > g_language_scroll + view) g_language_scroll = y1 - view;
    language_clamp_scroll();
}

static int pex_gamepad_language_update(PexGamepadState *p, double now) {
    if (g_screen != SCREEN_LANGUAGE || !pex_language_runtime_files_available()) return 0;
    int count = pex_language_count();
    if (count <= 0) return 0;
    if (g_gamepad_language_index < 0 || g_gamepad_language_index >= count)
        g_gamepad_language_index = pex_current_language_index();

    int delta = 0;
    if ((p->dpad_down || p->ly > 0.55f || p->ry > 0.55f) && now - g_gamepad_nav_last_time > 0.15) delta = 1;
    else if ((p->dpad_up || p->ly < -0.55f || p->ry < -0.55f) && now - g_gamepad_nav_last_time > 0.15) delta = -1;
    if (pex_gamepad_pressed(p->rb, p->prev_rb)) delta = pex_language_visible_rows() - 1;
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) delta = -(pex_language_visible_rows() - 1);
    if (delta) {
        g_gamepad_language_index += delta;
        if (g_gamepad_language_index < 0) g_gamepad_language_index = 0;
        if (g_gamepad_language_index >= count) g_gamepad_language_index = count - 1;
        pex_gamepad_language_ensure_visible(g_gamepad_language_index);
        rebuild_screen();
        g_gamepad_nav_last_time = now;
    }

    g_mouse_x = g_gui_w / 2;
    g_mouse_y = pex_language_row_y(g_gamepad_language_index) + 7;
    if (pex_gamepad_pressed(p->a, p->prev_a)) {
        const char *code = pex_language_code_at(g_gamepad_language_index);
        snprintf(g_opts.language, sizeof(g_opts.language), "%s", code);
        pex_set_language_code(g_opts.language);
        save_options();
        rebuild_screen();
    }
    if (pex_gamepad_pressed(p->y, p->prev_y)) {
        g_gamepad_language_index = pex_current_language_index();
        pex_gamepad_language_ensure_visible(g_gamepad_language_index);
        rebuild_screen();
    }
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back)) {
        save_options();
        set_screen(g_language_return_screen);
    }
    return 1;
}

static void pex_gamepad_texpack_ensure_visible(int idx) {
    int top = 32;
    int bottom = g_gui_h - 58 + 4;
    int y0 = idx * 36;
    int y1 = y0 + 36;
    int view = bottom - top - 4;
    if (y0 < g_texpack_scroll) g_texpack_scroll = y0;
    if (y1 > g_texpack_scroll + view) g_texpack_scroll = y1 - view;
    clamp_texpack_scroll();
}

static int pex_gamepad_texpack_update(PexGamepadState *p, double now) {
    if (g_screen != SCREEN_TEXPACK) return 0;
    if (g_texpack_count <= 0) scan_texture_packs();
    if (g_texpack_count <= 0) return 0;
    if (g_gamepad_texpack_index < 0 || g_gamepad_texpack_index >= g_texpack_count)
        g_gamepad_texpack_index = g_selected_texpack;

    int delta = 0;
    if ((p->dpad_down || p->ly > 0.55f || p->ry > 0.55f) && now - g_gamepad_nav_last_time > 0.15) delta = 1;
    else if ((p->dpad_up || p->ly < -0.55f || p->ry < -0.55f) && now - g_gamepad_nav_last_time > 0.15) delta = -1;
    if (pex_gamepad_pressed(p->rb, p->prev_rb)) delta = 5;
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) delta = -5;
    if (delta) {
        g_gamepad_texpack_index += delta;
        if (g_gamepad_texpack_index < 0) g_gamepad_texpack_index = 0;
        if (g_gamepad_texpack_index >= g_texpack_count) g_gamepad_texpack_index = g_texpack_count - 1;
        pex_gamepad_texpack_ensure_visible(g_gamepad_texpack_index);
        g_gamepad_nav_last_time = now;
    }
    g_mouse_x = g_gui_w / 2;
    g_mouse_y = 36 + g_gamepad_texpack_index * 36 - g_texpack_scroll + 14;

    if (pex_gamepad_pressed(p->a, p->prev_a)) {
        texpack_mouse_down(g_mouse_x, g_mouse_y);
        g_gamepad_texpack_index = g_selected_texpack;
    }
    if (pex_gamepad_pressed(p->x, p->prev_x)) {
        for (int i = 0; i < g_button_count; ++i)
            if (g_buttons[i].id == 5) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->start, p->prev_start) || pex_gamepad_pressed(p->y, p->prev_y))
        set_screen(g_parent_screen);
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back))
        set_screen(g_parent_screen);
    return 1;
}

static int pex_gamepad_achievements_update(PexGamepadState *p, double now) {
    if (g_screen != SCREEN_ACHIEVEMENTS) return 0;
    int dx = 0, dy = 0;
    if (p->dpad_left || p->lx < -0.45f) dx = -1;
    else if (p->dpad_right || p->lx > 0.45f) dx = 1;
    if (p->dpad_up || p->ly < -0.45f) dy = -1;
    else if (p->dpad_down || p->ly > 0.45f) dy = 1;
    if ((dx || dy) && now - g_gamepad_nav_last_time > 0.045) {
        int step = (p->lb || p->rb) ? 24 : 8;
        g_pex_achievement_map_x += dx * step;
        g_pex_achievement_map_y += dy * step;
        pex_achievement_map_clamp();
        g_gamepad_nav_last_time = now;
    }
    if (pex_gamepad_pressed(p->y, p->prev_y)) {
        g_pex_achievement_map_x = -82;
        g_pex_achievement_map_y = -70;
        pex_achievement_map_clamp();
    }
    if (pex_gamepad_pressed(p->a, p->prev_a) || pex_gamepad_pressed(p->b, p->prev_b) ||
        pex_gamepad_pressed(p->back, p->prev_back)) set_screen(SCREEN_INGAME);
    return 1;
}

static int pex_gamepad_statistics_update(PexGamepadState *p, double now) {
    if (g_screen != SCREEN_STATISTICS) return 0;
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) pex_statistics_set_tab((g_pex_statistics_tab + 2) % 3);
    if (pex_gamepad_pressed(p->rb, p->prev_rb)) pex_statistics_set_tab((g_pex_statistics_tab + 1) % 3);

    if ((p->dpad_down || p->ly > 0.55f || p->ry > 0.55f) && now - g_gamepad_nav_last_time > 0.12) {
        pex_statistics_scroll_by(1); g_gamepad_nav_last_time = now;
    } else if ((p->dpad_up || p->ly < -0.55f || p->ry < -0.55f) && now - g_gamepad_nav_last_time > 0.12) {
        pex_statistics_scroll_by(-1); g_gamepad_nav_last_time = now;
    }
    if (g_pex_statistics_tab != 0) {
        if (pex_gamepad_pressed(p->dpad_left, p->prev_dpad_left))
            g_gamepad_statistics_sort_focus = (g_gamepad_statistics_sort_focus + 2) % 3;
        if (pex_gamepad_pressed(p->dpad_right, p->prev_dpad_right))
            g_gamepad_statistics_sort_focus = (g_gamepad_statistics_sort_focus + 1) % 3;
        int base_x = g_gui_w / 2 - 108;
        int offsets[3] = {106, 156, 206};
        g_mouse_x = base_x + offsets[g_gamepad_statistics_sort_focus];
        g_mouse_y = 42;
        if (pex_gamepad_pressed(p->a, p->prev_a)) pex_statistics_mouse_down(g_mouse_x, g_mouse_y);
    }
    if (pex_gamepad_pressed(p->y, p->prev_y)) { g_pex_statistics_scroll = 0; }
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back) ||
        pex_gamepad_pressed(p->start, p->prev_start)) set_screen(SCREEN_PAUSE);
    return 1;
}

static int pex_gamepad_virtual_keyboard_update(PexGamepadState *p, double now) {
    if (g_screen != SCREEN_VIRTUAL_KEYBOARD) return 0;
    int dx = 0, dy = 0;
    if (p->dpad_left || p->lx < -0.55f) dx = -1;
    else if (p->dpad_right || p->lx > 0.55f) dx = 1;
    if (p->dpad_up || p->ly < -0.55f) dy = -1;
    else if (p->dpad_down || p->ly > 0.55f) dy = 1;
    if ((dx || dy) && now - g_gamepad_nav_last_time > 0.14) {
        pex_virtual_keyboard_move(dx, dy);
        g_gamepad_nav_last_time = now;
    }
    if (pex_gamepad_pressed(p->a, p->prev_a)) pex_virtual_keyboard_select();
    if (pex_gamepad_pressed(p->x, p->prev_x)) pex_virtual_keyboard_backspace();
    if (pex_gamepad_pressed(p->y, p->prev_y)) pex_virtual_keyboard_append(' ');
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) { g_virtual_keyboard_caps = !g_virtual_keyboard_caps; }
    if (pex_gamepad_pressed(p->start, p->prev_start) || pex_gamepad_pressed(p->rb, p->prev_rb)) pex_virtual_keyboard_done();
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back)) pex_virtual_keyboard_cancel();
    return 1;
}

static void pex_gamepad_creative_scroll_update(PexGamepadState *p, double now) {
    if (!p || g_screen != SCREEN_CREATIVE) return;
    if (fabsf(p->ry) > 0.45f && now - g_gamepad_ui_scroll_last_time > 0.09) {
        int rows = p->ry > 0.0f ? 1 : -1;
        if (fabsf(p->ry) > 0.85f) rows *= 2;
        creative_scroll_by(rows);
        g_gamepad_ui_scroll_last_time = now;
    }
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) creative_scroll_page(-1);
    if (pex_gamepad_pressed(p->rb, p->prev_rb)) creative_scroll_page(1);
}

static int pex_gamepad_multiplayer_list_update(PexGamepadState *p, double now) {
    if (g_screen != SCREEN_MULTIPLAYER || pex_mp_server_mode_get() != 0) return 0;
    pex_mp_server_list_ensure();
    int count = pex_mp_server_count_get();
    if (count <= 0) return 0;
    if (pex_mp_server_selected_get() < 0) { pex_mp_server_select(0); rebuild_screen(); }
    if ((p->dpad_down || p->ly > 0.55f || p->ry > 0.55f) && now - g_gamepad_nav_last_time > 0.15) {
        pex_mp_server_select(pex_mp_server_selected_get() + 1); rebuild_screen(); g_gamepad_nav_last_time = now;
    } else if ((p->dpad_up || p->ly < -0.55f || p->ry < -0.55f) && now - g_gamepad_nav_last_time > 0.15) {
        pex_mp_server_select(pex_mp_server_selected_get() - 1); rebuild_screen(); g_gamepad_nav_last_time = now;
    }
    if (pex_gamepad_pressed(p->lb, p->prev_lb)) { pex_mp_server_select(pex_mp_server_selected_get() - 5); rebuild_screen(); }
    if (pex_gamepad_pressed(p->rb, p->prev_rb)) { pex_mp_server_select(pex_mp_server_selected_get() + 5); rebuild_screen(); }
    if (pex_gamepad_pressed(p->a, p->prev_a)) {
        for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 10) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->x, p->prev_x)) {
        for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 3) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->y, p->prev_y)) {
        for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 7 && g_buttons[i].enabled) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->rt_down, p->prev_rt)) {
        for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 4) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->lt_down, p->prev_lt)) {
        for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 2 && g_buttons[i].enabled) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->start, p->prev_start)) {
        for (int i = 0; i < g_button_count; ++i) if (g_buttons[i].id == 8) { pex_gamepad_click_button(&g_buttons[i]); break; }
    }
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back)) pex_gamepad_back_action();
    return 1;
}

static void pex_gamepad_menu_update(PexGamepadState *p) {
    if (!p || !pex_gamepad_menu_screen()) return;
    double now = now_seconds();
    pex_gamepad_reset_ui_state_if_needed();
    if (pex_gamepad_virtual_keyboard_update(p, now)) return;
    if (pex_gamepad_world_screen_update(p, now)) return;
    if (pex_gamepad_language_update(p, now)) return;
    if (pex_gamepad_texpack_update(p, now)) return;
    if (pex_gamepad_achievements_update(p, now)) return;
    if (pex_gamepad_statistics_update(p, now)) return;
    if (pex_gamepad_multiplayer_list_update(p, now)) return;

    /* Key bindings are keyboard settings.  A controller must never trap the
       user in the "press a key" state; B cleanly cancels it. */
    if (g_screen == SCREEN_CONTROLS && g_waiting_key >= 0) {
        if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back) ||
            pex_gamepad_pressed(p->a, p->prev_a)) {
            g_waiting_key = -1;
            rebuild_screen();
        }
        return;
    }

    Button *sel = pex_gamepad_selected_button();
    if (sel) { g_mouse_x = sel->x + sel->w / 2; g_mouse_y = sel->y + sel->h / 2; }

    int dx = 0, dy = 0;
    if (p->dpad_left || p->lx < -0.60f) dx = -1;
    else if (p->dpad_right || p->lx > 0.60f) dx = 1;
    if (p->dpad_up || p->ly < -0.60f) dy = -1;
    else if (p->dpad_down || p->ly > 0.60f) dy = 1;

    /* Resolve diagonal stick/D-pad input to one dominant axis before spatial
       navigation.  This prevents a slightly diagonal press from skipping to
       a control in the neighbouring column. */
    if (dx && dy) {
        if (fabsf(p->lx) > fabsf(p->ly)) dy = 0;
        else dx = 0;
    }

    sel = pex_gamepad_selected_button();
    if (sel && sel->kind == BUTTON_SLIDER) {
        float delta = 0.0f;
        if (dx && now - g_gamepad_slider_last_time > 0.08) delta = dx * 0.025f;
        if (pex_gamepad_pressed(p->lb, p->prev_lb)) delta = -0.10f;
        if (pex_gamepad_pressed(p->rb, p->prev_rb)) delta = 0.10f;
        if (fabsf(p->rx) > 0.35f && now - g_gamepad_slider_last_time > 0.04)
            delta = p->rx * 0.035f;
        if (delta != 0.0f) {
            pex_gamepad_adjust_slider(sel, delta);
            g_gamepad_slider_last_time = now;
            dx = 0;
        }
    }

    if ((dx || dy) && now - g_gamepad_nav_last_time > 0.16) {
        pex_gamepad_nav_spatial(dx, dy);
        g_gamepad_nav_last_time = now;
    }

    sel = pex_gamepad_selected_button();
    if (pex_gamepad_pressed(p->a, p->prev_a) && now - g_gamepad_click_last_time > 0.10) {
        /* Controller mappings are fixed and separate from keyboard bindings;
           do not open an unusable keyboard-capture prompt from the couch. */
        if (sel && sel->kind == BUTTON_SLIDER) {
            /* A slider is adjusted with left/right or the right stick.  Clicking
               its centre with A would unexpectedly reset it to roughly 50%. */
        } else if (!(g_screen == SCREEN_CONTROLS && sel && sel->id >= 0 && sel->id < PEX_KEY_BIND_COUNT)) {
            pex_gamepad_click_button(sel);
        }
        g_gamepad_click_last_time = now;
    }
    if (pex_gamepad_pressed(p->start, p->prev_start) && g_screen == SCREEN_PAUSE) set_screen(SCREEN_INGAME);
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back)) pex_gamepad_back_action();
}

static void pex_gamepad_inventory_update(PexGamepadState *p, double dt) {
    if (!p || !pex_gamepad_inventory_screen()) { g_gamepad_virtual_cursor_active = 0; return; }
    pex_gamepad_reset_ui_state_if_needed();
    pex_gamepad_creative_scroll_update(p, now_seconds());

    g_gamepad_virtual_cursor_active = 1;
    if (!g_gamepad_virtual_cursor_x || !g_gamepad_virtual_cursor_y) {
        g_gamepad_virtual_cursor_x = (float)g_gui_w * 0.5f;
        g_gamepad_virtual_cursor_y = (float)g_gui_h * 0.5f;
    }
    float dx = p->lx;
    float dy = p->ly;
    if (p->dpad_left) dx = -1.0f; else if (p->dpad_right) dx = 1.0f;
    if (p->dpad_up) dy = -1.0f; else if (p->dpad_down) dy = 1.0f;
    float speed = 240.0f;
    if (p->rb) speed = 420.0f;
    g_gamepad_virtual_cursor_x += dx * speed * (float)dt;
    g_gamepad_virtual_cursor_y += dy * speed * (float)dt;
    if (g_gamepad_virtual_cursor_x < 2) g_gamepad_virtual_cursor_x = 2;
    if (g_gamepad_virtual_cursor_y < 2) g_gamepad_virtual_cursor_y = 2;
    if (g_gamepad_virtual_cursor_x > g_gui_w - 3) g_gamepad_virtual_cursor_x = (float)g_gui_w - 3;
    if (g_gamepad_virtual_cursor_y > g_gui_h - 3) g_gamepad_virtual_cursor_y = (float)g_gui_h - 3;
    g_mouse_x = (int)g_gamepad_virtual_cursor_x;
    g_mouse_y = (int)g_gamepad_virtual_cursor_y;
    if (pex_gamepad_pressed(p->a, p->prev_a) || pex_gamepad_pressed(p->rt_down, p->prev_rt)) {
        mouse_down(g_mouse_x, g_mouse_y); mouse_up(g_mouse_x, g_mouse_y);
    }
    if (pex_gamepad_pressed(p->x, p->prev_x) || pex_gamepad_pressed(p->lt_down, p->prev_lt))
        mouse_right_down(g_mouse_x, g_mouse_y);
    if (pex_gamepad_pressed(p->y, p->prev_y)) inventory_drop_selected_one();
    if (pex_gamepad_pressed(p->b, p->prev_b) || pex_gamepad_pressed(p->back, p->prev_back) ||
        pex_gamepad_pressed(p->start, p->prev_start)) pex_gamepad_back_action();
}

static void pex_gamepad_ingame_update(PexGamepadState *p, double dt) {
    if (!p || g_screen != SCREEN_INGAME || g_player_dead) return;
    if (fabsf(p->rx) > 0.02f || fabsf(p->ry) > 0.02f) {
#ifdef PEX_PLATFORM_PSP
        float look = 24.0f * (float)dt * 60.0f;
#else
        float look = 34.0f * (float)dt * 60.0f;
#endif
        player_turn_from_mouse((int)(p->rx * look), (int)(-p->ry * look));
    }
#ifdef PEX_PLATFORM_PSP
    if (p->rt_down && !p->prev_rt) { mouse_down(g_gui_w / 2, g_gui_h / 2); mouse_up(g_gui_w / 2, g_gui_h / 2); }
    if (p->lb && !p->prev_lb) mouse_right_down(g_gui_w / 2, g_gui_h / 2); /* L = place/use */
    if (!p->lb && p->prev_lb) mouse_right_up(g_gui_w / 2, g_gui_h / 2);
    if (p->dpad_up && !p->prev_dpad_up) { set_screen(SCREEN_INVENTORY); return; }

    /* PSP has no keyboard F3 key.  Use the requested in-game combo:
       Select/jump + Start/pause toggles the F3 debug overlay.  Treat the
       combo as a chord edge so holding both buttons toggles only once, and
       consume Start so the game does not immediately open the pause menu. */
    {
        int debug_combo = p->back && p->start;
        int debug_combo_prev = p->prev_back && p->prev_start;
        if (debug_combo && !debug_combo_prev) {
            g_debug_menu_shown = !g_debug_menu_shown;
            return;
        }
    }

    if (p->start && !p->prev_start && !p->back) set_screen(SCREEN_PAUSE);
    if (p->dpad_left && !p->prev_dpad_left) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
    if (p->dpad_right && !p->prev_dpad_right) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
#else
    if (p->rt_down && !p->prev_rt) { mouse_down(g_gui_w / 2, g_gui_h / 2); mouse_up(g_gui_w / 2, g_gui_h / 2); }
    if (p->lt_down && !p->prev_lt) mouse_right_down(g_gui_w / 2, g_gui_h / 2);
    if (!p->lt_down && p->prev_lt) mouse_right_up(g_gui_w / 2, g_gui_h / 2);
    /* Legacy-console in-game D-pad behavior:
       Up/Down are reserved for vertical Creative-flight movement (handled in
       pex_gamepad_rebuild_virtual_keys above), Left opens chat, and Right
       cycles the third-person camera exactly like F5.  It never walks or
       changes hotbar slots. */
    if (p->is_xbox && p->dpad_left && !p->prev_dpad_left) {
        g_chat_input[0] = 0;
        g_suppress_next_chat_char = 0;
        set_screen(SCREEN_CHAT);
        return;
    }
    if (p->is_xbox && p->dpad_right && !p->prev_dpad_right) third_person_view_cycle();
    if (p->y && !p->prev_y) {
        set_screen(player_is_creative() ? SCREEN_CREATIVE : SCREEN_INVENTORY);
        return;
    }
    if (p->x && !p->prev_x) inventory_drop_selected_one();
    if (p->back && !p->prev_back) set_screen(SCREEN_PAUSE);
    if (p->lb && !p->prev_lb) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
    if (p->rb && !p->prev_rb) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
#endif
#ifdef PEX_PLATFORM_ANDROID_TV
    pex_android_tv_ingame_update();
#endif
#ifdef PEX_PLATFORM_ANDROID
    /* Android touch tick runs from pex_gamepad_update even when gamepad focus is off. */
#endif
}

static void pex_gamepad_update(void) {
    PexGamepadState oldpads[PEX_GAMEPAD_MAX];
    memcpy(oldpads, g_gamepads, sizeof(oldpads));
    pex_gamepad_platform_poll(oldpads);
    for (int i = 0; i < g_gamepad_count && i < PEX_GAMEPAD_MAX; i++) {
        pex_gamepad_update_trigger_buttons(&g_gamepads[i], &oldpads[i]);
    }
    PexGamepadState *p = pex_gamepad_primary_pad();
#if defined(PEX_PLATFORM_WII)
    /* Wii has no keyboard/mouse gameplay path.  Keep the controller path active
       as long as a Wiimote/GameCube pad is present, not only on button edges. */
    if (p) pex_input_focus_set(PEX_INPUT_FOCUS_GAMEPAD);
#else
    if (gamepad_has_input(p)) pex_input_focus_set(PEX_INPUT_FOCUS_GAMEPAD);
#if defined(PEX_PLATFORM_SDL2) || defined(_WIN32)
    /* Input focus describes the device that is actually available, not only
       the last device that produced an event.  Without this reset a removed
       controller left gamepad prompts and virtual input latched forever. */
    if (!p && g_input_focus_mode == PEX_INPUT_FOCUS_GAMEPAD)
        pex_input_focus_set(PEX_INPUT_FOCUS_MOUSE);
    if (!p) g_gamepad_sneak_toggled = 0;
#endif
#endif

    PexGamepadState *active_pad = (g_input_focus_mode == PEX_INPUT_FOCUS_GAMEPAD) ? p : NULL;
    pex_gamepad_rebuild_virtual_keys(active_pad);
#ifdef PEX_PLATFORM_ANDROID
    android_touch_apply_virtual_keys();
#endif

    static double last = 0.0;
    double now = now_seconds();
    double dt = last > 0.0 ? now - last : 1.0 / 60.0;
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.10) dt = 0.10;
    last = now;
#ifdef PEX_PLATFORM_ANDROID
    pex_android_touch_ingame_update();
#endif
    if (g_input_focus_mode != PEX_INPUT_FOCUS_GAMEPAD) {
        g_gamepad_virtual_cursor_active = 0;
        return;
    }
    if (pex_gamepad_inventory_screen()) pex_gamepad_inventory_update(active_pad, dt);
    else if (g_screen == SCREEN_INGAME) pex_gamepad_ingame_update(active_pad, dt);
    else pex_gamepad_menu_update(active_pad);
}

static void pex_gamepad_shutdown(void) {
    g_gamepad_sneak_toggled = 0;
#ifdef PEX_PLATFORM_SDL2
    gamepad_sdl2_close_all();
#endif
#if !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_PSP) && !defined(PEX_PLATFORM_WII) && !defined(PEX_PLATFORM_XBOX_UWP)
    if (g_xinput_dll) { FreeLibrary(g_xinput_dll); g_xinput_dll = NULL; g_xinput_get_state = NULL; }
#endif
}

static void draw_gamepad_virtual_cursor(void) {
    if (g_input_focus_mode != PEX_INPUT_FOCUS_GAMEPAD) return;
    if (!g_gamepad_virtual_cursor_active || !pex_gamepad_inventory_screen()) return;
    int x = (int)g_gamepad_virtual_cursor_x;
    int y = (int)g_gamepad_virtual_cursor_y;
    /* One-pixel white core with a one-pixel dark outline.  This remains clear
       over bright slots without hiding the item beneath it. */
    draw_rect(x - 7, y - 1, x + 8, y + 2, (int)0xD0000000u);
    draw_rect(x - 1, y - 7, x + 2, y + 8, (int)0xD0000000u);
    draw_rect(x - 6, y, x + 7, y + 1, (int)0xFFFFFFFFu);
    draw_rect(x, y - 6, x + 1, y + 7, (int)0xFFFFFFFFu);
}


static void pex_gamepad_draw_focus_box(int x, int y, int w, int h) {
    int c = (int)0xFFFFFF55u;
    draw_rect(x - 2, y - 2, x + w + 2, y, c);
    draw_rect(x - 2, y + h, x + w + 2, y + h + 2, c);
    draw_rect(x - 2, y, x, y + h, c);
    draw_rect(x + w, y, x + w + 2, y + h, c);
}

static void pex_gamepad_draw_hint(const char *text) {
    if (!text || !*text) return;
    draw_xinput_hint_row(text, g_gui_h - 20, 0xFFFFFF, 1);
}

/* Draw focus for controls that intentionally have no normal Button rendering,
   plus concise couch-control hints.  Normal buttons already highlight because
   the gamepad keeps g_mouse_x/g_mouse_y on the focused control. */
static void draw_gamepad_ui_focus_and_hint(void) {
    if (g_input_focus_mode != PEX_INPUT_FOCUS_GAMEPAD) return;

    if (g_screen == SCREEN_LANGUAGE && pex_language_runtime_files_available() &&
        g_gamepad_language_index >= 0 && g_gamepad_language_index < pex_language_count()) {
        int y = pex_language_row_y(g_gamepad_language_index) - 2;
        pex_gamepad_draw_focus_box(g_gui_w / 2 - 110, y, 220, 18);
        pex_gamepad_draw_hint("{LS}/D-pad Select  {A} Apply  {LB}/{RB} Page  {B} Back");
    } else if (g_screen == SCREEN_TEXPACK && g_gamepad_texpack_index >= 0 &&
               g_gamepad_texpack_index < g_texpack_count) {
        int y = 36 + g_gamepad_texpack_index * 36 - g_texpack_scroll;
        pex_gamepad_draw_focus_box(g_gui_w / 2 - 110, y, 220, 32);
        pex_gamepad_draw_hint("{LS}/D-pad Select  {A} Apply  {LB}/{RB} Page  {X} Import  {B} Back");
    } else if (g_screen == SCREEN_CREATIVE) {
        pex_gamepad_draw_hint("{LS} Cursor  {A}/{RT} Take  {X}/{LT} Split  {RS} Scroll  {B} Close");
    } else if (pex_gamepad_inventory_screen()) {
        pex_gamepad_draw_hint("{LS} Cursor  {A}/{RT} Take  {X}/{LT} Split  {Y} Drop  {B} Close");
    } else if (g_screen == SCREEN_STATISTICS) {
        if (g_pex_statistics_tab != 0) {
            int base_x = g_gui_w / 2 - 108;
            int offsets[3] = {97, 147, 197};
            pex_gamepad_draw_focus_box(base_x + offsets[g_gamepad_statistics_sort_focus], 33, 18, 18);
        }
        pex_gamepad_draw_hint("{LB}/{RB} Tabs  {RS}/D-pad Scroll  Left/Right Sort  {A} Cycle  {B} Back");
    } else if (g_screen == SCREEN_ACHIEVEMENTS) {
        pex_gamepad_draw_hint("{LS}/D-pad Pan  {LB}/{RB} Faster  {Y} Reset  {A}/{B} Close");
    } else if (g_screen == SCREEN_MULTIPLAYER && pex_mp_server_mode_get() == 0) {
        pex_gamepad_draw_hint("{A} Join  {X} Add  {Y} Edit  {RT} Direct  {LT} Delete  {B} Back");
    } else if (g_screen == SCREEN_CONTROLS) {
        pex_gamepad_draw_hint("Controller layout fixed  {LS}/D-pad Navigate  {B} Back");
    } else {
        Button *b = pex_gamepad_selected_button();
        if (b && b->kind == BUTTON_HITBOX && b->id >= 9000 && b->id <= 9005) {
            pex_gamepad_draw_focus_box(b->x, b->y, b->w, b->h);
            pex_gamepad_draw_hint("{A}/OK Edit text  {B} Back");
        }
    }
}

static int pex_network_available(void) {
#if defined(PEX_PLATFORM_PSP) || defined(PEX_PLATFORM_WII)
    return 0;
#elif defined(PEX_PLATFORM_SDL2)
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return 0;
    closesocket(s);
    return 1;
#else
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 0;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int ok = s != INVALID_SOCKET;
    if (s != INVALID_SOCKET) closesocket(s);
    WSACleanup();
    return ok;
#endif
}

static void pex_query_system_info(void) {
    double now = now_seconds();
    if (now - g_system_info_last_time < 1.0) return;
    g_system_info_last_time = now;
    memset(&g_system_info, 0, sizeof(g_system_info));
    snprintf(g_system_info.client_version, sizeof(g_system_info.client_version), "%s", VERSION_TEXT);
#ifdef PEX_PLATFORM_PSP
    snprintf(g_system_info.platform, sizeof(g_system_info.platform), "Sony PSP / PSPSDK");
    snprintf(g_system_info.display_name, sizeof(g_system_info.display_name), "PSP LCD");
    g_system_info.display_refresh_hz = 60;
    g_system_info.screen_w = 480;
    g_system_info.screen_h = 272;
    g_system_info.ram_total_mb = 32;
    /* Real PSP free memory is what matters for PSP-1000 testing; PPSSPP model
       selection does not enforce a true 32MB heap. */
    g_system_info.ram_available_mb = (unsigned long long)(sceKernelTotalFreeMemSize() / (1024 * 1024));
#elif defined(PEX_PLATFORM_WII)
    snprintf(g_system_info.platform, sizeof(g_system_info.platform), "Nintendo Wii / devkitPPC + libogc");
    snprintf(g_system_info.display_name, sizeof(g_system_info.display_name), "Wii video output");
    g_system_info.display_refresh_hz = 60;
    g_system_info.screen_w = g_render_w;
    g_system_info.screen_h = g_render_h;
    g_system_info.ram_total_mb = 88;
    g_system_info.ram_available_mb = 0;
#elif defined(PEX_PLATFORM_ANDROID)
    snprintf(g_system_info.platform, sizeof(g_system_info.platform), "Android / SDL2 Touch");
    g_system_info.display_name[0] = 0;
    int display = g_hwnd ? SDL_GetWindowDisplayIndex(g_hwnd) : 0;
    SDL_DisplayMode mode;
    if (display < 0) display = 0;
    const char *dn = SDL_GetDisplayName(display);
    snprintf(g_system_info.display_name, sizeof(g_system_info.display_name), "%s", dn ? dn : "Android display");
    if (SDL_GetCurrentDisplayMode(display, &mode) == 0) g_system_info.display_refresh_hz = mode.refresh_rate;
#elif defined(PEX_PLATFORM_ANDROID_TV)
    snprintf(g_system_info.platform, sizeof(g_system_info.platform), "Android TV / SDL2");
    g_system_info.display_name[0] = 0;
    int display = g_hwnd ? SDL_GetWindowDisplayIndex(g_hwnd) : 0;
    SDL_DisplayMode mode;
    if (display < 0) display = 0;
    const char *dn = SDL_GetDisplayName(display);
    snprintf(g_system_info.display_name, sizeof(g_system_info.display_name), "%s", dn ? dn : "Android TV display");
    if (SDL_GetCurrentDisplayMode(display, &mode) == 0) g_system_info.display_refresh_hz = mode.refresh_rate;
#elif defined(PEX_PLATFORM_SDL2)
    snprintf(g_system_info.platform, sizeof(g_system_info.platform), "Linux / SDL2");
    int display = g_hwnd ? SDL_GetWindowDisplayIndex(g_hwnd) : 0;
    SDL_DisplayMode mode;
    if (display < 0) display = 0;
    const char *dn = SDL_GetDisplayName(display);
    snprintf(g_system_info.display_name, sizeof(g_system_info.display_name), "%s", dn ? dn : "Display");
    if (SDL_GetCurrentDisplayMode(display, &mode) == 0) g_system_info.display_refresh_hz = mode.refresh_rate;
    long pages = sysconf(_SC_PHYS_PAGES);
    long avpages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) g_system_info.ram_total_mb = (unsigned long long)pages * (unsigned long long)page_size / (1024ULL * 1024ULL);
    if (avpages > 0 && page_size > 0) g_system_info.ram_available_mb = (unsigned long long)avpages * (unsigned long long)page_size / (1024ULL * 1024ULL);
#else
    snprintf(g_system_info.platform, sizeof(g_system_info.platform), "Windows / Win32");
    HDC dc = g_hdc ? g_hdc : GetDC(g_hwnd);
    if (dc) {
        g_system_info.display_refresh_hz = GetDeviceCaps(dc, VREFRESH);
        g_system_info.screen_w = GetDeviceCaps(dc, HORZRES);
        g_system_info.screen_h = GetDeviceCaps(dc, VERTRES);
        if (!g_hdc && g_hwnd) ReleaseDC(g_hwnd, dc);
    }
    snprintf(g_system_info.display_name, sizeof(g_system_info.display_name), "Primary display");
    MEMORYSTATUSEX ms; memset(&ms, 0, sizeof(ms)); ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        g_system_info.ram_total_mb = ms.ullTotalPhys / (1024ULL * 1024ULL);
        g_system_info.ram_available_mb = ms.ullAvailPhys / (1024ULL * 1024ULL);
    }
#endif
    g_system_info.screen_w = g_system_info.screen_w ? g_system_info.screen_w : g_win_w;
    g_system_info.screen_h = g_system_info.screen_h ? g_system_info.screen_h : g_win_h;
    snprintf(g_system_info.renderer_backend, sizeof(g_system_info.renderer_backend), "%s", renderer_backend_label(g_runtime_renderer_backend));
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    snprintf(g_system_info.gpu_vendor, sizeof(g_system_info.gpu_vendor), "%s", vendor ? (const char*)vendor : "Unknown");
    snprintf(g_system_info.gpu_renderer, sizeof(g_system_info.gpu_renderer), "%s", renderer ? (const char*)renderer : "Unknown");
    snprintf(g_system_info.gpu_version, sizeof(g_system_info.gpu_version), "%s", version ? (const char*)version : "Unknown");
    g_system_info.network_available = pex_network_available();
}

static void draw_system_info_screen(void) {
    pex_query_system_info();
    draw_default_bg();
    draw_centered_text("System / Controller Info", g_gui_w / 2, 16, 16777215);
    int x = g_gui_w / 2 - 190;
    if (x < 8) x = 8;
    int y = 36;
    char line[256];
    snprintf(line, sizeof(line), "Client: %s", g_system_info.client_version); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "Platform: %s", g_system_info.platform); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "Renderer: %s", g_system_info.renderer_backend); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "GPU: %.120s", g_system_info.gpu_renderer); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "GPU vendor: %.100s", g_system_info.gpu_vendor); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "GL/API version: %.90s", g_system_info.gpu_version); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "Display: %dx%d @ %dHz (%s)", g_system_info.screen_w, g_system_info.screen_h,
             g_system_info.display_refresh_hz, g_system_info.display_name); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "RAM: %lluMB total / %lluMB available", g_system_info.ram_total_mb, g_system_info.ram_available_mb); draw_text(line, x, y, 14737632); y += 10;
    snprintf(line, sizeof(line), "Network capability: %s", g_system_info.network_available ? "available" : "unavailable"); draw_text(line, x, y, 14737632); y += 14;
    snprintf(line, sizeof(line), "Controllers connected: %d", g_gamepad_count); draw_text(line, x, y, 16777215); y += 10;
    if (g_gamepad_count <= 0) {
        draw_text("No gamepad currently detected.", x, y, 10526880); y += 10;
    } else {
        for (int i = 0; i < g_gamepad_count && i < PEX_GAMEPAD_MAX; i++) {
            PexGamepadState *p = &g_gamepads[i];
            snprintf(line, sizeof(line), "%d. %s", i + 1, p->name); draw_text(line, x, y, i == g_gamepad_primary ? 16777120 : 14737632); y += 10;
            snprintf(line, sizeof(line), "   Type: %s  A:%d B:%d X:%d Y:%d LB:%d RB:%d LT:%.2f RT:%.2f", p->kind, p->a, p->b, p->x, p->y, p->lb, p->rb, p->lt, p->rt);
            draw_text(line, x, y, 10526880); y += 10;
            snprintf(line, sizeof(line), "   LS %.2f %.2f  RS %.2f %.2f  DPad %d%d%d%d", p->lx, p->ly, p->rx, p->ry, p->dpad_up, p->dpad_down, p->dpad_left, p->dpad_right);
            draw_text(line, x, y, 10526880); y += 10;
            if (y > g_gui_h - 54) break;
        }
    }
    #ifdef PEX_PLATFORM_PSP
    draw_text("PSP: D-pad menu, Cross selects, Circle backs. Sliders: Left/Right.", x, g_gui_h - 44, 10526880);
    draw_text("PSP game: analog move, face buttons look, Select jump, Start pause, Select+Start F3.", x, g_gui_h - 34, 10526880);
#elif defined(PEX_PLATFORM_ANDROID)
    draw_text("Touch: left stick moves, right side drags look, tap places/uses, hold mines.", x, g_gui_h - 44, 10526880);
    draw_text("Buttons: Jump, Sneak, Inv, Pause. Tap hotbar slots to select.", x, g_gui_h - 34, 10526880);
#elif defined(PEX_PLATFORM_ANDROID_TV)
    draw_text("TV remote: D-pad moves menus/game, OK selects/jumps, Back pauses.", x, g_gui_h - 44, 10526880);
    draw_text("Colors: Red break, Green place, Yellow inventory, Blue sneak/back. 1-9 hotbar, 0 camera.", x, g_gui_h - 34, 10526880);
#else
    draw_xinput_hint_row("Menu: {LS}/D-pad Move  {A} Select  {B} Back  {RS} Sliders",
                         g_gui_h - 45, 0xFFFFFF, 0);
    draw_xinput_hint_row("Game: {LS} Move  {RS} Look  {RT} Break  {LT} Use  {LB}/{RB} Hotbar",
                         g_gui_h - 25, 0xFFFFFF, 0);
#endif
    draw_all_buttons();
}
