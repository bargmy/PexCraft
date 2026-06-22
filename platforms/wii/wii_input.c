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
    if (sens < 0.0f) sens = 0.0f; if (sens > 1.0f) sens = 1.0f;
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

static void wii_input_init(void) {
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    PAD_Init();
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

static void wii_poll_classic(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    for (int ch = 0; ch < WPAD_MAX_WIIMOTES && g_gamepad_count < PEX_GAMEPAD_MAX; ++ch) {
        WPADData *wd = WPAD_Data(ch);
        if (!wd || wd->err != WPAD_ERR_NONE || wd->exp.type != EXP_CLASSIC) continue;
        const classic_ctrl_t *cc = &wd->exp.classic;
        PexGamepadState *p = wii_begin_pad(oldpads, "Wii Classic Controller", "Classic Controller");
        if (!p) return;
        u32 b = cc->btns;
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
    }
}

static void wii_poll_nunchuk(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    for (int ch = 0; ch < WPAD_MAX_WIIMOTES && g_gamepad_count < PEX_GAMEPAD_MAX; ++ch) {
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
    if (g_gamepad_primary >= 0 && g_gamepad_primary < g_gamepad_count) {
        PexGamepadState *p = &g_gamepads[g_gamepad_primary];
        if (p->guide) g_running = 0;
    }
}
