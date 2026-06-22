/* Nintendo Wii input layer.  Priority:
   1) Classic Controller / Classic Controller Pro
   2) GameCube Controller
   3) Wiimote + Nunchuk */

static void center_mouse_in_window(void) { }
static void set_mouse_grabbed(int grabbed) { g_mouse_grabbed = grabbed ? 1 : 0; g_cursor_hidden = g_mouse_grabbed; }

static float lerp_angle(float a, float b, float partial) {
    float d = b - a;
    while (d < -180.0f) d += 360.0f;
    while (d >= 180.0f) d -= 360.0f;
    return a + d * partial;
}

static void player_turn_from_mouse(int dx, int dy_java) {
    if (g_screen != SCREEN_INGAME) return;
    float sens = g_opts.sensitivity;
    if (sens < 0.0f) sens = 0.0f;
    if (sens > 1.0f) sens = 1.0f;
    float f = sens * 0.6f + 0.2f;
    float mul = f * f * f * 8.0f;
    float yaw_delta = (float)dx * mul * 0.15f;
    float pitch_delta = (float)dy_java * mul * 0.15f;
    g_player_yaw += yaw_delta;
    g_player_pitch += pitch_delta;
    if (g_player_pitch < -90.0f) g_player_pitch = -90.0f;
    if (g_player_pitch > 90.0f) g_player_pitch = 90.0f;
}

static void handle_grabbed_mouse_move(int px, int py) { (void)px; (void)py; }
static int key_down_vk(int vk) { if (vk >= 0 && vk < 512 && g_gamepad_vk_state[vk]) return 1; return 0; }

#ifndef PEX_WII_MAX_WPAD
#define PEX_WII_MAX_WPAD 4
#endif

static void wii_input_init(void) {
    WPAD_Init();
    for (int ch = 0; ch < PEX_WII_MAX_WPAD; ++ch) {
#if defined(WPAD_FMT_BTNS_ACC_IR_EXT)
        /* Classic Controller data only appears when an EXT data format is enabled. */
        WPAD_SetDataFormat(ch, WPAD_FMT_BTNS_ACC_IR_EXT);
#elif defined(WPAD_FMT_BTNS_ACC_EXT)
        WPAD_SetDataFormat(ch, WPAD_FMT_BTNS_ACC_EXT);
#elif defined(WPAD_FMT_BTNS_EXT)
        WPAD_SetDataFormat(ch, WPAD_FMT_BTNS_EXT);
#else
        WPAD_SetDataFormat(ch, WPAD_FMT_BTNS_ACC_IR);
#endif
    }
    PAD_Init();
    wii_debug_logf("input init: WPAD/PAD initialized, EXT format requested, Classic Controller preferred");
}

static float wii_axis_deadzone(float v) {
    float av = v < 0.0f ? -v : v;
    return (av < PEX_GAMEPAD_DEADZONE) ? 0.0f : v;
}

static void wii_copy_edges(PexGamepadState *dst, const PexGamepadState *old) {
    if (!dst || !old) return;
    dst->prev_a = old->a;
    dst->prev_b = old->b;
    dst->prev_x = old->x;
    dst->prev_y = old->y;
    dst->prev_lb = old->lb;
    dst->prev_rb = old->rb;
    dst->prev_back = old->back;
    dst->prev_start = old->start;
    dst->prev_dpad_up = old->dpad_up;
    dst->prev_dpad_down = old->dpad_down;
    dst->prev_dpad_left = old->dpad_left;
    dst->prev_dpad_right = old->dpad_right;
    dst->prev_lt = old->lt > 0.35f;
    dst->prev_rt = old->rt > 0.35f;
}
static float wii_pad_axis_s8(s8 v) {
    float f = (float)v / 80.0f;
    if (f < -1.0f) f = -1.0f;
    if (f > 1.0f) f = 1.0f;
    return wii_axis_deadzone(f);
}
static float wii_joy_x(const joystick_t *js) {
    if (!js) return 0.0f;
    float rad = js->ang * (float)M_PI / 180.0f;
    return wii_axis_deadzone(sinf(rad) * js->mag);
}
static float wii_joy_y(const joystick_t *js) {
    if (!js) return 0.0f;
    float rad = js->ang * (float)M_PI / 180.0f;
    return wii_axis_deadzone(-cosf(rad) * js->mag);
}

static PexGamepadState *wii_begin_pad(PexGamepadState oldpads[PEX_GAMEPAD_MAX], const char *name, const char *kind) {
    if (g_gamepad_count >= PEX_GAMEPAD_MAX) return NULL;
    int slot = g_gamepad_count;
    PexGamepadState *p = &g_gamepads[slot];
    wii_copy_edges(p, &oldpads[slot]);
    p->connected = 1;
    p->slot = slot;
    snprintf(p->name, sizeof(p->name), "%s", name);
    snprintf(p->kind, sizeof(p->kind), "%s", kind);
    if (g_gamepad_primary < 0) g_gamepad_primary = slot;
    g_gamepad_count++;
    return p;
}

static void wii_log_pad_edges(const char *label, int ch, const PexGamepadState *p) {
    static unsigned int last_mask[4] = {0,0,0,0};
    if (!p || ch < 0 || ch >= 4) return;
    unsigned int mask = 0;
    if (p->a) mask |= 1u << 0;
    if (p->b) mask |= 1u << 1;
    if (p->x) mask |= 1u << 2;
    if (p->y) mask |= 1u << 3;
    if (p->lb) mask |= 1u << 4;
    if (p->rb) mask |= 1u << 5;
    if (p->lt > 0.35f) mask |= 1u << 6;
    if (p->rt > 0.35f) mask |= 1u << 7;
    if (p->back) mask |= 1u << 8;
    if (p->start) mask |= 1u << 9;
    if (p->guide) mask |= 1u << 10;
    if (p->dpad_up) mask |= 1u << 11;
    if (p->dpad_down) mask |= 1u << 12;
    if (p->dpad_left) mask |= 1u << 13;
    if (p->dpad_right) mask |= 1u << 14;
    if (mask != last_mask[ch]) {
        wii_debug_logf("input %s ch%d mask=%08x ls=%.2f %.2f rs=%.2f %.2f dpad=%d%d%d%d abxy=%d%d%d%d",
                       label ? label : "pad", ch, mask, p->lx, p->ly, p->rx, p->ry,
                       p->dpad_up, p->dpad_down, p->dpad_left, p->dpad_right,
                       p->a, p->b, p->x, p->y);
        last_mask[ch] = mask;
    }
}

static int wii_exp_is_classic(int exp_type) {
    if (exp_type == EXP_CLASSIC) return 1;
#ifdef EXP_CLASSIC_PRO
    if (exp_type == EXP_CLASSIC_PRO) return 1;
#endif
    return 0;
}

static void wii_poll_classic(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    static int last_probe_type[4] = {-999,-999,-999,-999};
    static int last_exp_type[4] = {-999,-999,-999,-999};
    static int last_err[4] = {-999,-999,-999,-999};

    for (int ch = 0; ch < PEX_WII_MAX_WPAD && g_gamepad_count < PEX_GAMEPAD_MAX; ++ch) {
        int probe_type = -1;
        int probe = WPAD_Probe(ch, &probe_type);
        WPADData *wd = WPAD_Data(ch);
        int exp_type = wd ? wd->exp.type : -1;
        int err = wd ? wd->err : -999;
        if (probe_type != last_probe_type[ch] || exp_type != last_exp_type[ch] || err != last_err[ch]) {
            wii_debug_logf("WPAD ch=%d probe=%d probe_type=%d data_err=%d exp_type=%d held=%08x", ch, probe, probe_type, err, exp_type, (unsigned)WPAD_ButtonsHeld(ch));
            last_probe_type[ch] = probe_type;
            last_exp_type[ch] = exp_type;
            last_err[ch] = err;
        }
        if (!wd || err != WPAD_ERR_NONE) continue;
        if (!wii_exp_is_classic(exp_type) && !wii_exp_is_classic(probe_type)) continue;

        const classic_ctrl_t *cc = &wd->exp.classic;
        PexGamepadState *p = wii_begin_pad(oldpads, "Wii Classic Controller", "Classic Controller");
        if (!p) return;

        /* Dolphin/libogc may expose Classic buttons in either exp.classic.btns or
           WPAD_ButtonsHeld(), depending on controller path. OR both together. */
        u32 b = cc->btns | WPAD_ButtonsHeld(ch);
        p->lx = wii_joy_x(&cc->ljs);
        p->ly = wii_joy_y(&cc->ljs);
        p->rx = wii_joy_x(&cc->rjs);
        p->ry = wii_joy_y(&cc->rjs);
        p->a = (b & CLASSIC_CTRL_BUTTON_A) != 0;
        p->b = (b & CLASSIC_CTRL_BUTTON_B) != 0;
        p->x = (b & CLASSIC_CTRL_BUTTON_X) != 0;
        p->y = (b & CLASSIC_CTRL_BUTTON_Y) != 0;
        p->lb = (b & CLASSIC_CTRL_BUTTON_FULL_L) != 0;
        p->rb = (b & CLASSIC_CTRL_BUTTON_FULL_R) != 0;
        p->lt = (b & CLASSIC_CTRL_BUTTON_ZL) ? 1.0f : cc->l_shoulder;
        p->rt = (b & CLASSIC_CTRL_BUTTON_ZR) ? 1.0f : cc->r_shoulder;
        p->back = (b & CLASSIC_CTRL_BUTTON_MINUS) != 0;
        p->start = (b & CLASSIC_CTRL_BUTTON_PLUS) != 0;
        p->guide = (b & CLASSIC_CTRL_BUTTON_HOME) != 0;
        p->dpad_up = (b & CLASSIC_CTRL_BUTTON_UP) != 0;
        p->dpad_down = (b & CLASSIC_CTRL_BUTTON_DOWN) != 0;
        p->dpad_left = (b & CLASSIC_CTRL_BUTTON_LEFT) != 0;
        p->dpad_right = (b & CLASSIC_CTRL_BUTTON_RIGHT) != 0;
        wii_log_pad_edges("classic", ch, p);
    }
}

static void wii_poll_gamecube(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    PADStatus pads[PAD_CHANMAX];
    memset(pads, 0, sizeof(pads));
    PAD_Read(pads);
    PAD_Clamp(pads);
    for (int ch = 0; ch < PAD_CHANMAX && g_gamepad_count < PEX_GAMEPAD_MAX; ++ch) {
        if (pads[ch].err != PAD_ERR_NONE) continue;
        PexGamepadState *p = wii_begin_pad(oldpads, "Nintendo GameCube Controller", "GameCube Controller");
        if (!p) return;
        u16 b = pads[ch].button;
        p->lx = wii_pad_axis_s8(pads[ch].stickX);
        p->ly = wii_pad_axis_s8(-pads[ch].stickY);
        p->rx = wii_pad_axis_s8(pads[ch].substickX);
        p->ry = wii_pad_axis_s8(-pads[ch].substickY);
        p->a = (b & PAD_BUTTON_A) != 0;
        p->b = (b & PAD_BUTTON_B) != 0;
        p->x = (b & PAD_BUTTON_X) != 0;
        p->y = (b & PAD_BUTTON_Y) != 0;
        p->lb = (b & PAD_BUTTON_LEFT) != 0;
        p->rb = (b & (PAD_TRIGGER_Z | PAD_BUTTON_RIGHT)) != 0;
        p->lt = (b & PAD_TRIGGER_L) ? 1.0f : ((float)pads[ch].triggerL / 255.0f);
        p->rt = (b & PAD_TRIGGER_R) ? 1.0f : ((float)pads[ch].triggerR / 255.0f);
        p->start = (b & PAD_BUTTON_START) != 0;
        p->dpad_up = (b & PAD_BUTTON_UP) != 0;
        p->dpad_down = (b & PAD_BUTTON_DOWN) != 0;
        p->dpad_left = (b & PAD_BUTTON_LEFT) != 0;
        p->dpad_right = (b & PAD_BUTTON_RIGHT) != 0;
        wii_log_pad_edges("gamecube", ch, p);
    }
}

static void wii_poll_nunchuk(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    for (int ch = 0; ch < PEX_WII_MAX_WPAD && g_gamepad_count < PEX_GAMEPAD_MAX; ++ch) {
        WPADData *wd = WPAD_Data(ch);
        if (!wd || wd->err != WPAD_ERR_NONE || wd->exp.type != EXP_NUNCHUK) continue;
        const nunchuk_t *nc = &wd->exp.nunchuk;
        PexGamepadState *p = wii_begin_pad(oldpads, "Wii Remote + Nunchuk", "Wiimote/Nunchuk");
        if (!p) return;
        u32 wb = WPAD_ButtonsHeld(ch);
        u32 nb = nc->btns;
        p->lx = wii_joy_x(&nc->js);
        p->ly = wii_joy_y(&nc->js);
        p->rx = ((wb & WPAD_BUTTON_RIGHT) ? 1.0f : 0.0f) - ((wb & WPAD_BUTTON_LEFT) ? 1.0f : 0.0f);
        p->ry = ((wb & WPAD_BUTTON_DOWN) ? 1.0f : 0.0f) - ((wb & WPAD_BUTTON_UP) ? 1.0f : 0.0f);
        p->a = (wb & WPAD_BUTTON_A) != 0;
        p->b = (nb & NUNCHUK_BUTTON_C) != 0;
        p->x = (wb & WPAD_BUTTON_2) != 0;
        p->y = (wb & WPAD_BUTTON_1) != 0;
        p->lt = (nb & NUNCHUK_BUTTON_Z) ? 1.0f : 0.0f;
        p->rt = (wb & WPAD_BUTTON_B) ? 1.0f : 0.0f;
        p->back = (wb & WPAD_BUTTON_MINUS) != 0;
        p->start = (wb & WPAD_BUTTON_PLUS) != 0;
        p->guide = (wb & WPAD_BUTTON_HOME) != 0;
        p->dpad_up = (wb & WPAD_BUTTON_UP) != 0;
        p->dpad_down = (wb & WPAD_BUTTON_DOWN) != 0;
        p->dpad_left = (wb & WPAD_BUTTON_LEFT) != 0;
        p->dpad_right = (wb & WPAD_BUTTON_RIGHT) != 0;
        wii_log_pad_edges("nunchuk", ch, p);
    }
}

static void wii_poll_wiimote_buttons(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    for (int ch = 0; ch < PEX_WII_MAX_WPAD && g_gamepad_count < PEX_GAMEPAD_MAX; ++ch) {
        int exp_type = -1;
        int probe = WPAD_Probe(ch, &exp_type);
        /* Even when Dolphin reports an extension, WPAD extension data can be
           unavailable while raw Wiimote buttons still work.  Keep this fallback
           alive so D-pad/A/+ can always drive menus. */
        if (probe != WPAD_ERR_NONE) continue;
        u32 b = WPAD_ButtonsHeld(ch);
        if (!b) continue;
        PexGamepadState *p = wii_begin_pad(oldpads, "Wii Remote", "Wiimote");
        if (!p) return;
        p->a = (b & WPAD_BUTTON_A) != 0;
        p->b = (b & WPAD_BUTTON_B) != 0;
        p->x = (b & WPAD_BUTTON_2) != 0;
        p->y = (b & WPAD_BUTTON_1) != 0;
        p->back = (b & WPAD_BUTTON_MINUS) != 0;
        p->start = (b & WPAD_BUTTON_PLUS) != 0;
        p->guide = (b & WPAD_BUTTON_HOME) != 0;
        p->dpad_up = (b & WPAD_BUTTON_UP) != 0;
        p->dpad_down = (b & WPAD_BUTTON_DOWN) != 0;
        p->dpad_left = (b & WPAD_BUTTON_LEFT) != 0;
        p->dpad_right = (b & WPAD_BUTTON_RIGHT) != 0;
        wii_log_pad_edges("wiimote", ch, p);
    }
}

static void wii_poll_gamepads(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    g_gamepad_count = 0;
    g_gamepad_primary = -1;
    memset(g_gamepads, 0, sizeof(g_gamepads));
    WPAD_ScanPads();
    PAD_ScanPads();
    wii_poll_classic(oldpads);
    wii_poll_gamecube(oldpads);
    wii_poll_nunchuk(oldpads);
    wii_poll_wiimote_buttons(oldpads);
    if (g_gamepad_primary >= 0 && g_gamepad_primary < g_gamepad_count) {
        PexGamepadState *p = &g_gamepads[g_gamepad_primary];
        if (p->guide) g_running = 0;
    }
}
