/* Android TV remote-to-gamepad bridge.
   Java dispatches raw Android KeyEvent keyCode values here.  We convert the
   TV remote into the same PexGamepadState abstraction used by Xbox/XInput,
   SDL_GameController, PSP, and generic joystick paths. */

#define PEX_ATV_KEY_BACK        4
#define PEX_ATV_KEY_0           7
#define PEX_ATV_KEY_1           8
#define PEX_ATV_KEY_2           9
#define PEX_ATV_KEY_3           10
#define PEX_ATV_KEY_4           11
#define PEX_ATV_KEY_5           12
#define PEX_ATV_KEY_6           13
#define PEX_ATV_KEY_7           14
#define PEX_ATV_KEY_8           15
#define PEX_ATV_KEY_9           16
#define PEX_ATV_DPAD_UP         19
#define PEX_ATV_DPAD_DOWN       20
#define PEX_ATV_DPAD_LEFT       21
#define PEX_ATV_DPAD_RIGHT      22
#define PEX_ATV_DPAD_CENTER     23
#define PEX_ATV_KEY_ENTER       66
#define PEX_ATV_KEY_NUMPAD_ENT  160
#define PEX_ATV_PROG_RED        183
#define PEX_ATV_PROG_GREEN      184
#define PEX_ATV_PROG_YELLOW     185
#define PEX_ATV_PROG_BLUE       186

static void pex_gamepad_copy_edges(PexGamepadState *dst, const PexGamepadState *old);
static void pex_gamepad_menu_update(PexGamepadState *p);
static void pex_input_focus_set(PexInputFocusMode mode);

static volatile int g_android_tv_key_state[256];
static volatile int g_android_tv_digit_edge[10];
static volatile int g_android_tv_remote_seen = 0;
static volatile int g_android_tv_remote_active = 0;
static int g_android_tv_prev_key_state[256];
static int g_android_tv_prev_action_state[PEX_TV_REMOTE_ACTION_COUNT];

#ifndef PEX_ATV_BACK
#define PEX_ATV_BACK PEX_ATV_KEY_BACK
#endif

static int pex_android_tv_supported_key(int key) {
    return (key >= PEX_ATV_KEY_0 && key <= PEX_ATV_KEY_9) ||
           key == PEX_ATV_BACK ||
           key == PEX_ATV_DPAD_UP || key == PEX_ATV_DPAD_DOWN ||
           key == PEX_ATV_DPAD_LEFT || key == PEX_ATV_DPAD_RIGHT ||
           key == PEX_ATV_DPAD_CENTER || key == PEX_ATV_KEY_ENTER ||
           key == PEX_ATV_KEY_NUMPAD_ENT || key == PEX_ATV_PROG_RED ||
           key == PEX_ATV_PROG_GREEN || key == PEX_ATV_PROG_YELLOW ||
           key == PEX_ATV_PROG_BLUE;
}

static int pex_android_tv_key_down(int key) {
    if (key < 0 || key >= (int)ARRAY_COUNT(g_android_tv_key_state)) return 0;
    return g_android_tv_key_state[key] != 0;
}

static int pex_android_tv_ok_down(void) {
    return pex_android_tv_key_down(PEX_ATV_DPAD_CENTER) ||
           pex_android_tv_key_down(PEX_ATV_KEY_ENTER) ||
           pex_android_tv_key_down(PEX_ATV_KEY_NUMPAD_ENT);
}

static int pex_android_tv_any_down(void) {
    for (int i = 0; i < (int)ARRAY_COUNT(g_android_tv_key_state); ++i) {
        if (g_android_tv_key_state[i]) return 1;
    }
    return 0;
}

static int pex_android_tv_digit_to_slot(int digit) {
    if (digit >= 1 && digit <= 9) return digit - 1;
    return -1;
}

static int android_tv_consume_hotbar_slot(void) {
    for (int digit = 1; digit <= 9; ++digit) {
        if (g_android_tv_digit_edge[digit]) {
            g_android_tv_digit_edge[digit] = 0;
            return pex_android_tv_digit_to_slot(digit);
        }
    }
    if (g_android_tv_digit_edge[0]) {
        g_android_tv_digit_edge[0] = 0;
        /* There are nine hotbar slots.  Leave 0 as a safe extra shortcut. */
        third_person_view_cycle();
    }
    return -1;
}

/* Android TV remotes are deliberately kept out of PexGamepadState. Android
   already provides keyboard/IME behavior, and treating a remote as a generic
   controller changes prompts, cursor behavior and text entry. Poll the raw key
   state on the game thread and expose only ordinary keyboard-style actions. */
static int pex_android_tv_key_pressed(int key) {
    if (key < 0 || key >= (int)ARRAY_COUNT(g_android_tv_key_state)) return 0;
    return g_android_tv_key_state[key] && !g_android_tv_prev_key_state[key];
}

static int pex_android_tv_action_down(int action, int default_key) {
    if (g_opts.tv_remote_mapped) return pex_tv_remote_action_down(action);
    return pex_android_tv_key_down(default_key);
}

static int pex_android_tv_action_pressed(int action, int default_key) {
    if (!g_opts.tv_remote_mapped)
        return default_key > 0 ? pex_android_tv_key_pressed(default_key) : 0;
    int down = pex_android_tv_action_down(action, default_key);
    int prev = (action >= 0 && action < PEX_TV_REMOTE_ACTION_COUNT) ?
        g_android_tv_prev_action_state[action] : 0;
    return down && !prev;
}

static int pex_android_tv_action_was_down(int action, int default_key) {
    if (!g_opts.tv_remote_mapped)
        return default_key > 0 && default_key < (int)ARRAY_COUNT(g_android_tv_prev_key_state) ?
            g_android_tv_prev_key_state[default_key] : 0;
    return action >= 0 && action < PEX_TV_REMOTE_ACTION_COUNT ?
        g_android_tv_prev_action_state[action] : 0;
}

static void pex_android_tv_copy_key_edges(void) {
    for (int i = 0; i < (int)ARRAY_COUNT(g_android_tv_prev_key_state); ++i)
        g_android_tv_prev_key_state[i] = g_android_tv_key_state[i] ? 1 : 0;
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i)
        g_android_tv_prev_action_state[i] = pex_tv_remote_action_down(i) ? 1 : 0;
}

static void pex_android_tv_release_remote_actions(void) {
    memset(g_android_tv_remote_vk_state, 0, sizeof(g_android_tv_remote_vk_state));
}

static int pex_android_tv_click_controls_down(void) {
    return pex_android_tv_action_down(PEX_TV_REMOTE_BREAK, PEX_ATV_PROG_RED) ||
           pex_android_tv_action_down(PEX_TV_REMOTE_PLACE, PEX_ATV_PROG_GREEN);
}

static void pex_android_tv_remote_update(void) {
    pex_android_tv_release_remote_actions();

    /* A remote press is keyboard/navigation input. It must actively undo any
       stale generic-controller focus before prompts or virtual cursor are built. */
    if (pex_android_tv_any_down() || pex_tv_remote_any_action_down())
        pex_input_focus_set(PEX_INPUT_FOCUS_MOUSE);

    int up = pex_android_tv_action_down(PEX_TV_REMOTE_UP, PEX_ATV_DPAD_UP);
    int down = pex_android_tv_action_down(PEX_TV_REMOTE_DOWN, PEX_ATV_DPAD_DOWN);
    int left = pex_android_tv_action_down(PEX_TV_REMOTE_LEFT, PEX_ATV_DPAD_LEFT);
    int right = pex_android_tv_action_down(PEX_TV_REMOTE_RIGHT, PEX_ATV_DPAD_RIGHT);
    int ok = g_opts.tv_remote_mapped ? pex_tv_remote_action_down(PEX_TV_REMOTE_OK) : pex_android_tv_ok_down();

    if (g_screen == SCREEN_INGAME && !g_player_dead) {
        if (up) g_android_tv_remote_vk_state[g_opts.keys[0] & 511] = 1;
        if (down) g_android_tv_remote_vk_state[g_opts.keys[2] & 511] = 1;
        if (left) g_android_tv_remote_vk_state[g_opts.keys[1] & 511] = 1;
        if (right) g_android_tv_remote_vk_state[g_opts.keys[3] & 511] = 1;
        if (ok) g_android_tv_remote_vk_state[g_opts.keys[4] & 511] = 1;
        if (pex_android_tv_action_down(PEX_TV_REMOTE_SNEAK, PEX_ATV_PROG_BLUE))
            g_android_tv_remote_vk_state[g_opts.keys[5] & 511] = 1;

        /* Colored remote buttons remain optional gameplay shortcuts, but they
           are regular input edges and never activate gamepad/console focus. */
        int break_down = pex_android_tv_action_down(PEX_TV_REMOTE_BREAK, PEX_ATV_PROG_RED);
        int place_down = pex_android_tv_action_down(PEX_TV_REMOTE_PLACE, PEX_ATV_PROG_GREEN);
        if (!g_gameplay_click_suppressed_until_release) {
            if (pex_android_tv_action_pressed(PEX_TV_REMOTE_BREAK, PEX_ATV_PROG_RED))
                mouse_down(g_gui_w / 2, g_gui_h / 2);
            if (!break_down && pex_android_tv_action_was_down(PEX_TV_REMOTE_BREAK, PEX_ATV_PROG_RED))
                mouse_up(g_gui_w / 2, g_gui_h / 2);
            if (pex_android_tv_action_pressed(PEX_TV_REMOTE_PLACE, PEX_ATV_PROG_GREEN))
                mouse_right_down(g_gui_w / 2, g_gui_h / 2);
            if (!place_down && pex_android_tv_action_was_down(PEX_TV_REMOTE_PLACE, PEX_ATV_PROG_GREEN))
                mouse_right_up(g_gui_w / 2, g_gui_h / 2);
        }
        if (pex_android_tv_action_pressed(PEX_TV_REMOTE_INVENTORY, PEX_ATV_PROG_YELLOW))
            set_screen(player_is_creative() ? SCREEN_CREATIVE : SCREEN_INVENTORY);
        if (pex_android_tv_action_pressed(PEX_TV_REMOTE_DROP, 0))
            inventory_drop_selected_one();
        if (pex_android_tv_action_pressed(PEX_TV_REMOTE_HOTBAR_PREV, 0))
            g_selected_hotbar_slot = (g_selected_hotbar_slot + 8) % 9;
        if (pex_android_tv_action_pressed(PEX_TV_REMOTE_HOTBAR_NEXT, 0))
            g_selected_hotbar_slot = (g_selected_hotbar_slot + 1) % 9;

        int slot = android_tv_consume_hotbar_slot();
        if (slot >= 0 && slot < 9) g_selected_hotbar_slot = slot;
    } else {
        /* Reuse the mature menu navigator without registering this device as a
           gamepad. Buttons remain remote-navigable, but prompts, virtual cursor,
           chat keyboard choice and HUD layout stay in mouse/keyboard mode. */
        PexGamepadState nav;
        memset(&nav, 0, sizeof(nav));
        nav.connected = 1;
        nav.is_tv_remote = 1;
        nav.dpad_up = up; nav.dpad_down = down;
        nav.dpad_left = left; nav.dpad_right = right;
        nav.a = ok;
        nav.back = pex_android_tv_action_down(PEX_TV_REMOTE_BACK, PEX_ATV_BACK);
        nav.prev_dpad_up = pex_android_tv_action_was_down(PEX_TV_REMOTE_UP, PEX_ATV_DPAD_UP);
        nav.prev_dpad_down = pex_android_tv_action_was_down(PEX_TV_REMOTE_DOWN, PEX_ATV_DPAD_DOWN);
        nav.prev_dpad_left = pex_android_tv_action_was_down(PEX_TV_REMOTE_LEFT, PEX_ATV_DPAD_LEFT);
        nav.prev_dpad_right = pex_android_tv_action_was_down(PEX_TV_REMOTE_RIGHT, PEX_ATV_DPAD_RIGHT);
        nav.prev_a = g_opts.tv_remote_mapped ? g_android_tv_prev_action_state[PEX_TV_REMOTE_OK] :
            (g_android_tv_prev_key_state[PEX_ATV_DPAD_CENTER] ||
             g_android_tv_prev_key_state[PEX_ATV_KEY_ENTER] ||
             g_android_tv_prev_key_state[PEX_ATV_KEY_NUMPAD_ENT]);
        nav.prev_back = pex_android_tv_action_was_down(PEX_TV_REMOTE_BACK, PEX_ATV_BACK);
        pex_gamepad_menu_update(&nav);
    }
    pex_android_tv_copy_key_edges();
}

JNIEXPORT jboolean JNICALL
Java_ir_hienob_pexcraft_tv_PexCraftActivity_nativeOnTvKey(JNIEnv *env, jobject thiz, jint keyCode, jboolean down, jint source) {
    (void)env;
    (void)thiz;
    int key = (int)keyCode;
    /* Java filters mouse/stylus/keyboard events. Keep a native guard too: an
       Android mouse may synthesize DPAD_CENTER, which must never become jump. */
    if (((int)source & 0x00002002) == 0x00002002 || /* SOURCE_MOUSE */
        ((int)source & 0x00001002) == 0x00001002 || /* SOURCE_TOUCHSCREEN */
        ((int)source & 0x00000401) == 0x00000401 || /* SOURCE_GAMEPAD */
        ((int)source & 0x01000010) == 0x01000010)   /* SOURCE_JOYSTICK */
        return JNI_FALSE;
    if (!pex_android_tv_supported_key(key) && g_screen != SCREEN_TV_REMOTE_MAP) return JNI_FALSE;
    if (pex_tv_remote_handle_raw_key(key, down ? 1 : 0)) {
        g_android_tv_remote_seen = 1;
        if (g_screen == SCREEN_TV_REMOTE_MAP || g_opts.tv_remote_mapped) return JNI_TRUE;
    }
    if (key >= 0 && key < (int)ARRAY_COUNT(g_android_tv_key_state)) {
        int was_down = g_android_tv_key_state[key] != 0;
        g_android_tv_key_state[key] = down ? 1 : 0;
        if (down && !was_down && key >= PEX_ATV_KEY_0 && key <= PEX_ATV_KEY_9) {
            int digit = key - PEX_ATV_KEY_0;
            g_android_tv_digit_edge[digit] = 1;
        }
    }
    g_android_tv_remote_seen = 1;
    g_android_tv_remote_active = pex_android_tv_any_down();
    return JNI_TRUE;
}
