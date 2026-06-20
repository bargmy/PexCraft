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

static void pex_gamepad_classify(PexGamepadState *pad, const char *name) {
    snprintf(pad->name, sizeof(pad->name), "%s", (name && *name) ? name : "Unknown controller");
    pad->is_xbox = pex_str_contains_i(pad->name, "xbox") ||
                   pex_str_contains_i(pad->name, "xinput") ||
                   pex_str_contains_i(pad->name, "360") ||
                   pex_str_contains_i(pad->name, "series");
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
    dst->prev_lt = old->lt > 0.35f; dst->prev_rt = old->rt > 0.35f;
    dst->prev_dpad_up = old->dpad_up; dst->prev_dpad_down = old->dpad_down;
    dst->prev_dpad_left = old->dpad_left; dst->prev_dpad_right = old->dpad_right;
}

#ifdef PEX_PLATFORM_SDL2
static SDL_GameController *g_sdl2_pads[PEX_GAMEPAD_MAX];
static SDL_Joystick *g_sdl2_joys[PEX_GAMEPAD_MAX];
static int g_sdl2_pad_is_controller[PEX_GAMEPAD_MAX];
static int g_sdl2_pad_device_index[PEX_GAMEPAD_MAX];
static int g_sdl2_pad_open_count = 0;

static float sdl_axis_norm(Sint16 v) {
    float f = v < 0 ? (float)v / 32768.0f : (float)v / 32767.0f;
    return pex_deadzone(f);
}

static void pex_gamepad_sdl2_close_all(void) {
    for (int i = 0; i < PEX_GAMEPAD_MAX; i++) {
        if (g_sdl2_pads[i]) { SDL_GameControllerClose(g_sdl2_pads[i]); g_sdl2_pads[i] = NULL; }
        else if (g_sdl2_joys[i]) { SDL_JoystickClose(g_sdl2_joys[i]); g_sdl2_joys[i] = NULL; }
        g_sdl2_pad_is_controller[i] = 0;
        g_sdl2_pad_device_index[i] = -1;
    }
    g_sdl2_pad_open_count = 0;
}

static void pex_gamepad_scan_devices(void) {
    double now = now_seconds();
    if (now - g_gamepad_probe_last_time < 1.0) return;
    g_gamepad_probe_last_time = now;
    pex_gamepad_sdl2_close_all();
    int total = SDL_NumJoysticks();
    for (int i = 0; i < total && g_sdl2_pad_open_count < PEX_GAMEPAD_MAX; i++) {
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
    pex_gamepad_load_xinput();
    g_gamepad_count = 0;
    g_gamepad_primary = -1;
    memset(g_gamepads, 0, sizeof(g_gamepads));
    if (g_xinput_get_state) {
        for (DWORD i = 0; i < 4 && g_gamepad_count < PEX_GAMEPAD_MAX; i++) {
            PexXInputState st;
            memset(&st, 0, sizeof(st));
            if (g_xinput_get_state(i, &st) == 0) {
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
    /* WinMM joystick fallback for DirectInput-like pads, DualShock adapters, and old USB converters. */
    UINT n = joyGetNumDevs();
    for (UINT i = 0; i < n && g_gamepad_count < PEX_GAMEPAD_MAX; i++) {
        JOYINFOEX ji; memset(&ji, 0, sizeof(ji)); ji.dwSize = sizeof(ji); ji.dwFlags = JOY_RETURNALL;
        if (joyGetPosEx(i, &ji) != JOYERR_NOERROR) continue;
        JOYCAPSA jc; memset(&jc, 0, sizeof(jc));
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
    return g_screen == SCREEN_TITLE || g_screen == SCREEN_OPTIONS || g_screen == SCREEN_OPTIONS_MORE ||
           g_screen == SCREEN_SYSTEM_INFO || g_screen == SCREEN_SKINS || g_screen == SCREEN_CONTROLS ||
           g_screen == SCREEN_WORLD_SELECT || g_screen == SCREEN_WORLD_TYPE || g_screen == SCREEN_WORLD_DELETE ||
           g_screen == SCREEN_CONFIRM_DELETE || g_screen == SCREEN_MULTIPLAYER || g_screen == SCREEN_CONNECTING ||
           g_screen == SCREEN_TEXPACK || g_screen == SCREEN_PAUSE || g_screen == SCREEN_DEATH ||
           g_screen == SCREEN_NOTICE || g_screen == SCREEN_RENDERER_RESTART_PROMPT ||
           g_screen == SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT || g_screen == SCREEN_CLASSIC_PACK_WARNING;
}

static int pex_gamepad_inventory_screen(void) {
    return g_screen == SCREEN_INVENTORY || g_screen == SCREEN_WORKBENCH || g_screen == SCREEN_FURNACE || g_screen == SCREEN_CHEST;
}

static PexGamepadState *pex_gamepad_primary_pad(void) {
    if (g_gamepad_primary >= 0 && g_gamepad_primary < g_gamepad_count) return &g_gamepads[g_gamepad_primary];
    return NULL;
}

static void pex_gamepad_rebuild_virtual_keys(PexGamepadState *p) {
    memset(g_gamepad_vk_state, 0, sizeof(g_gamepad_vk_state));
    if (!p || g_screen != SCREEN_INGAME || g_player_dead) return;
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
    if (p->rt > 0.35f) g_gamepad_vk_state[VK_LBUTTON] = 1;                /* R = break/attack */
#else
    if (p->ly < -PEX_GAMEPAD_DEADZONE || p->dpad_up) g_gamepad_vk_state[g_opts.keys[0] & 511] = 1;
    if (p->ly >  PEX_GAMEPAD_DEADZONE || p->dpad_down) g_gamepad_vk_state[g_opts.keys[2] & 511] = 1;
    if (p->lx < -PEX_GAMEPAD_DEADZONE || p->dpad_left) g_gamepad_vk_state[g_opts.keys[1] & 511] = 1;
    if (p->lx >  PEX_GAMEPAD_DEADZONE || p->dpad_right) g_gamepad_vk_state[g_opts.keys[3] & 511] = 1;
    if (p->a) g_gamepad_vk_state[g_opts.keys[4] & 511] = 1;             /* jump */
    if (p->b || p->ls) g_gamepad_vk_state[g_opts.keys[5] & 511] = 1;     /* sneak */
    /* RT is break/attack. RB must not mirror RT; RB is reserved for hotbar next. */
    if (p->rt > 0.35f) g_gamepad_vk_state[VK_LBUTTON] = 1;      /* break/attack */
#endif
}

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

static Button *pex_gamepad_selected_button(void) {
    if (g_button_count <= 0) return NULL;
    if (g_gamepad_menu_index < 0) g_gamepad_menu_index = 0;
    if (g_gamepad_menu_index >= g_button_count) g_gamepad_menu_index = g_button_count - 1;
    for (int pass = 0; pass < g_button_count; pass++) {
        int idx = (g_gamepad_menu_index + pass) % g_button_count;
        if (g_buttons[idx].visible && g_buttons[idx].enabled) {
            g_gamepad_menu_index = idx;
            return &g_buttons[idx];
        }
    }
    return NULL;
}

static void pex_gamepad_nav_move(int dir) {
    if (g_button_count <= 0) return;
    int start = g_gamepad_menu_index;
    if (start < 0 || start >= g_button_count) start = 0;
    for (int i = 0; i < g_button_count; i++) {
        start += dir;
        if (start < 0) start = g_button_count - 1;
        if (start >= g_button_count) start = 0;
        if (g_buttons[start].visible && g_buttons[start].enabled) {
            g_gamepad_menu_index = start;
            g_mouse_x = g_buttons[start].x + g_buttons[start].w / 2;
            g_mouse_y = g_buttons[start].y + g_buttons[start].h / 2;
            return;
        }
    }
}

static void pex_gamepad_menu_update(PexGamepadState *p) {
    if (!p || !pex_gamepad_menu_screen()) return;
    Button *sel = pex_gamepad_selected_button();
    if (sel) { g_mouse_x = sel->x + sel->w / 2; g_mouse_y = sel->y + sel->h / 2; }
    double now = now_seconds();
    int nav_down = p->dpad_down || p->ly > 0.55f;
    int nav_up = p->dpad_up || p->ly < -0.55f;
    if ((nav_down || nav_up) && now - g_gamepad_nav_last_time > 0.18) {
        pex_gamepad_nav_move(nav_down ? 1 : -1);
        g_gamepad_nav_last_time = now;
    }
    sel = pex_gamepad_selected_button();
    if (sel && sel->kind == BUTTON_SLIDER && fabsf(p->rx) > 0.35f && now - g_gamepad_slider_last_time > 0.04) {
        float v = sel->slider_value + p->rx * 0.035f;
        int mx = sel->x + 4 + (int)(v * (float)(sel->w - 8));
        update_slider(sel, mx);
        g_gamepad_slider_last_time = now;
    }
    if (p->a && !p->prev_a && now - g_gamepad_click_last_time > 0.10) {
        pex_gamepad_click_button(sel);
        g_gamepad_click_last_time = now;
    }
    if ((p->b && !p->prev_b) || (p->back && !p->prev_back)) pex_gamepad_back_action();
}

static void pex_gamepad_inventory_update(PexGamepadState *p, double dt) {
    if (!p || !pex_gamepad_inventory_screen()) { g_gamepad_virtual_cursor_active = 0; return; }
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
    if (p->rb || p->rt > 0.35f) speed = 420.0f;
    g_gamepad_virtual_cursor_x += dx * speed * (float)dt;
    g_gamepad_virtual_cursor_y += dy * speed * (float)dt;
    if (g_gamepad_virtual_cursor_x < 2) g_gamepad_virtual_cursor_x = 2;
    if (g_gamepad_virtual_cursor_y < 2) g_gamepad_virtual_cursor_y = 2;
    if (g_gamepad_virtual_cursor_x > g_gui_w - 3) g_gamepad_virtual_cursor_x = (float)g_gui_w - 3;
    if (g_gamepad_virtual_cursor_y > g_gui_h - 3) g_gamepad_virtual_cursor_y = (float)g_gui_h - 3;
    g_mouse_x = (int)g_gamepad_virtual_cursor_x;
    g_mouse_y = (int)g_gamepad_virtual_cursor_y;
    if (p->a && !p->prev_a) { mouse_down(g_mouse_x, g_mouse_y); mouse_up(g_mouse_x, g_mouse_y); }
    if ((p->x && !p->prev_x) || (p->lt > 0.35f && !p->prev_lt)) mouse_right_down(g_mouse_x, g_mouse_y);
    if (p->b && !p->prev_b) pex_gamepad_back_action();
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
    if (p->lb && !p->prev_lb) mouse_right_down(g_gui_w / 2, g_gui_h / 2); /* L = place/use */
    if (p->dpad_up && !p->prev_dpad_up) { set_screen(SCREEN_INVENTORY); return; }
    if (p->start && !p->prev_start) set_screen(SCREEN_PAUSE);
    if (p->dpad_left && !p->prev_dpad_left) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
    if (p->dpad_right && !p->prev_dpad_right) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
#else
    if (p->lt > 0.35f && !p->prev_lt) mouse_right_down(g_gui_w / 2, g_gui_h / 2);
    if (p->y && !p->prev_y) { set_screen(SCREEN_INVENTORY); return; }
    if (p->x && !p->prev_x) inventory_drop_selected_one();
    if (p->back && !p->prev_back) set_screen(SCREEN_PAUSE);
    if (p->lb && !p->prev_lb) g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
    if (p->rb && !p->prev_rb) g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;
#endif
}

static void pex_gamepad_update(void) {
    PexGamepadState oldpads[PEX_GAMEPAD_MAX];
    memcpy(oldpads, g_gamepads, sizeof(oldpads));
    pex_gamepad_platform_poll(oldpads);
    PexGamepadState *p = pex_gamepad_primary_pad();
    pex_gamepad_rebuild_virtual_keys(p);
    static double last = 0.0;
    double now = now_seconds();
    double dt = last > 0.0 ? now - last : 1.0 / 60.0;
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.10) dt = 0.10;
    last = now;
    if (pex_gamepad_inventory_screen()) pex_gamepad_inventory_update(p, dt);
    else if (g_screen == SCREEN_INGAME) pex_gamepad_ingame_update(p, dt);
    else pex_gamepad_menu_update(p);
}

static void pex_gamepad_shutdown(void) {
#ifdef PEX_PLATFORM_SDL2
    pex_gamepad_sdl2_close_all();
#endif
#if !defined(PEX_PLATFORM_SDL2) && !defined(PEX_PLATFORM_PSP)
    if (g_xinput_dll) { FreeLibrary(g_xinput_dll); g_xinput_dll = NULL; g_xinput_get_state = NULL; }
#endif
}

static void draw_gamepad_virtual_cursor(void) {
    if (!g_gamepad_virtual_cursor_active || !pex_gamepad_inventory_screen()) return;
    int x = (int)g_gamepad_virtual_cursor_x;
    int y = (int)g_gamepad_virtual_cursor_y;
    /* Compact virtual cursor: still a thick +, but no longer covers slots/items. */
    draw_rect(x - 8, y - 2, x + 9, y + 3, (int)0xD0FFFFFFu);
    draw_rect(x - 2, y - 8, x + 3, y + 9, (int)0xD0FFFFFFu);
    draw_rect(x - 6, y - 1, x + 7, y + 2, (int)0xFF000000u);
    draw_rect(x - 1, y - 6, x + 2, y + 7, (int)0xFF000000u);
}

static int pex_network_available(void) {
#if defined(PEX_PLATFORM_PSP)
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
    g_system_info.ram_available_mb = 0;
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
    draw_text("PSP game: analog move, Square/Circle turn, Triangle/X look, Select jump.", x, g_gui_h - 34, 10526880);
#else
    draw_text("Menu: D-pad/left stick moves, A selects, B goes back. Sliders: right stick.", x, g_gui_h - 44, 10526880);
    draw_text("Gameplay: left stick move, right stick look, RT break, LT place, LB/RB hotbar.", x, g_gui_h - 34, 10526880);
#endif
    draw_all_buttons();
}
