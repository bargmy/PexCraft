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

static volatile int g_android_tv_key_state[256];
static volatile int g_android_tv_digit_edge[10];
static volatile int g_android_tv_remote_seen = 0;
static volatile int g_android_tv_remote_active = 0;

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

static void pex_android_tv_append_remote_pad(PexGamepadState oldpads[PEX_GAMEPAD_MAX]) {
    if (!g_android_tv_remote_seen && !pex_android_tv_any_down()) return;
    if (g_gamepad_count >= PEX_GAMEPAD_MAX) return;

    int slot = g_gamepad_count;
    PexGamepadState *p = &g_gamepads[slot];
    memset(p, 0, sizeof(*p));
    pex_gamepad_copy_edges(p, &oldpads[slot]);
    p->connected = 1;
    p->slot = slot;
    snprintf(p->name, sizeof(p->name), "Android TV remote");
    snprintf(p->kind, sizeof(p->kind), "TV remote");

    p->dpad_up = pex_android_tv_key_down(PEX_ATV_DPAD_UP);
    p->dpad_down = pex_android_tv_key_down(PEX_ATV_DPAD_DOWN);
    p->dpad_left = pex_android_tv_key_down(PEX_ATV_DPAD_LEFT);
    p->dpad_right = pex_android_tv_key_down(PEX_ATV_DPAD_RIGHT);

    /* Match the existing Xbox/SDL gamepad abstraction:
       A=OK/jump/select, B=blue/back/sneak, Y=yellow/inventory,
       LT=green/place/use, RT=red/break/attack, BACK=Android Back/pause. */
    p->a = pex_android_tv_ok_down();
    p->b = pex_android_tv_key_down(PEX_ATV_PROG_BLUE);
    p->y = pex_android_tv_key_down(PEX_ATV_PROG_YELLOW);
    p->back = pex_android_tv_key_down(PEX_ATV_BACK);
    p->lt = pex_android_tv_key_down(PEX_ATV_PROG_GREEN) ? 1.0f : 0.0f;
    p->rt = pex_android_tv_key_down(PEX_ATV_PROG_RED) ? 1.0f : 0.0f;

    g_gamepad_count++;
    if (pex_android_tv_any_down() || g_gamepad_primary < 0) g_gamepad_primary = slot;
}

static void pex_android_tv_ingame_update(void) {
    int slot = android_tv_consume_hotbar_slot();
    if (slot >= 0 && slot < 9) g_selected_hotbar_slot = slot;
}

JNIEXPORT jboolean JNICALL
Java_ir_hienob_pexcraft_tv_PexCraftActivity_nativeOnTvKey(JNIEnv *env, jobject thiz, jint keyCode, jboolean down) {
    (void)env;
    (void)thiz;
    int key = (int)keyCode;
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
