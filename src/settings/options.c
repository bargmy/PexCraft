/* Split from original monolithic main.c. Included by src/main.c unity build. */

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static void set_default_options(void) {
    g_opts.music = 1.0f;
    g_opts.sound = 1.0f;
    g_opts.sensitivity = 0.5f;
    g_opts.invert_mouse = 0;
    g_opts.render_distance = 0;
    g_opts.view_bobbing = 1;
    g_opts.anaglyph = 0;
    g_opts.limit_framerate = 0;
    g_opts.difficulty = 2;
    g_opts.fancy_graphics = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "Default");
    g_opts.last_server[0] = 0;
    for (int i = 0; i < 10; i++) g_opts.keys[i] = default_keys[i];
}

static float parse_float_java(const char *s) {
    if (!strcmp(s, "true")) return 1.0f;
    if (!strcmp(s, "false")) return 0.0f;
    return (float)atof(s);
}

static void load_options(void) {
    set_default_options();
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "options.txt");
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = strchr(line, ':');
        if (!p) continue;
        *p++ = 0;
        char *k = trim(line);
        char *v = trim(p);
        if (!strcmp(k, "music")) g_opts.music = parse_float_java(v);
        else if (!strcmp(k, "sound")) g_opts.sound = parse_float_java(v);
        else if (!strcmp(k, "mouseSensitivity")) g_opts.sensitivity = parse_float_java(v);
        else if (!strcmp(k, "invertYMouse")) g_opts.invert_mouse = !strcmp(v, "true");
        else if (!strcmp(k, "viewDistance")) g_opts.render_distance = atoi(v) & 3;
        else if (!strcmp(k, "bobView")) g_opts.view_bobbing = !strcmp(v, "true");
        else if (!strcmp(k, "anaglyph3d")) g_opts.anaglyph = !strcmp(v, "true");
        else if (!strcmp(k, "limitFramerate")) g_opts.limit_framerate = !strcmp(v, "true");
        else if (!strcmp(k, "difficulty")) g_opts.difficulty = atoi(v) & 3;
        else if (!strcmp(k, "fancyGraphics")) g_opts.fancy_graphics = !strcmp(v, "true");
        else if (!strcmp(k, "skin")) snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", v);
        else if (!strcmp(k, "lastServer")) snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s", v);
        else if (!strncmp(k, "key_", 4)) {
            const char *name = k + 4;
            for (int i = 0; i < 10; i++) {
                const char *orig[10] = {"key.forward","key.left","key.back","key.right","key.jump","key.sneak","key.drop","key.inventory","key.chat","key.fog"};
                if (!strcmp(name, orig[i])) g_opts.keys[i] = lwjgl_to_vk(atoi(v));
            }
        }
    }
    fclose(f);
}

static void save_options(void) {
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "options.txt");
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "music:%g\n", g_opts.music);
    fprintf(f, "sound:%g\n", g_opts.sound);
    fprintf(f, "invertYMouse:%s\n", g_opts.invert_mouse ? "true" : "false");
    fprintf(f, "mouseSensitivity:%g\n", g_opts.sensitivity);
    fprintf(f, "viewDistance:%d\n", g_opts.render_distance);
    fprintf(f, "bobView:%s\n", g_opts.view_bobbing ? "true" : "false");
    fprintf(f, "anaglyph3d:%s\n", g_opts.anaglyph ? "true" : "false");
    fprintf(f, "limitFramerate:%s\n", g_opts.limit_framerate ? "true" : "false");
    fprintf(f, "difficulty:%d\n", g_opts.difficulty);
    fprintf(f, "fancyGraphics:%s\n", g_opts.fancy_graphics ? "true" : "false");
    fprintf(f, "skin:%s\n", g_opts.skin);
    fprintf(f, "lastServer:%s\n", g_opts.last_server);
    const char *orig[10] = {"key.forward","key.left","key.back","key.right","key.jump","key.sneak","key.drop","key.inventory","key.chat","key.fog"};
    for (int i = 0; i < 10; i++) fprintf(f, "key_%s:%d\n", orig[i], vk_to_lwjgl(g_opts.keys[i]));
    fclose(f);
}

static float get_option_float(OptionId opt) {
    if (opt == OPT_MUSIC) return g_opts.music;
    if (opt == OPT_SOUND) return g_opts.sound;
    if (opt == OPT_SENSITIVITY) return g_opts.sensitivity;
    return 0.0f;
}

static void set_option_float(OptionId opt, float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (opt == OPT_MUSIC) g_opts.music = v;
    else if (opt == OPT_SOUND) g_opts.sound = v;
    else if (opt == OPT_SENSITIVITY) g_opts.sensitivity = v;
    save_options();
}

static void get_option_label(OptionId opt, char *out, size_t cap) {
    const char *name = opt_names[opt];
    if (opt_is_slider[opt]) {
        float v = get_option_float(opt);
        if (opt == OPT_SENSITIVITY) {
            if (v <= 0.0f) snprintf(out, cap, "%s: *yawn*", name);
            else if (v >= 1.0f) snprintf(out, cap, "%s: HYPERSPEED!!!", name);
            else snprintf(out, cap, "%s: %d%%", name, (int)(v * 200.0f));
        } else {
            if (v <= 0.0f) snprintf(out, cap, "%s: OFF", name);
            else snprintf(out, cap, "%s: %d%%", name, (int)(v * 100.0f));
        }
    } else if (opt_is_boolean[opt]) {
        int val = 0;
        if (opt == OPT_INVERT_MOUSE) val = g_opts.invert_mouse;
        else if (opt == OPT_VIEW_BOBBING) val = g_opts.view_bobbing;
        else if (opt == OPT_ANAGLYPH) val = g_opts.anaglyph;
        else if (opt == OPT_LIMIT_FRAMERATE) val = g_opts.limit_framerate;
        snprintf(out, cap, "%s: %s", name, val ? "ON" : "OFF");
    } else if (opt == OPT_RENDER_DISTANCE) {
        snprintf(out, cap, "%s: %s", name, render_distance_names[g_opts.render_distance & 3]);
    } else if (opt == OPT_DIFFICULTY) {
        snprintf(out, cap, "%s: %s", name, difficulty_names[g_opts.difficulty & 3]);
    } else if (opt == OPT_GRAPHICS) {
        snprintf(out, cap, "%s: %s", name, g_opts.fancy_graphics ? "Fancy" : "Fast");
    } else snprintf(out, cap, "%s: ", name);
}

static void bump_option(OptionId opt, int delta) {
    if (opt == OPT_INVERT_MOUSE) g_opts.invert_mouse = !g_opts.invert_mouse;
    else if (opt == OPT_RENDER_DISTANCE) g_opts.render_distance = (g_opts.render_distance + delta) & 3;
    else if (opt == OPT_VIEW_BOBBING) g_opts.view_bobbing = !g_opts.view_bobbing;
    else if (opt == OPT_ANAGLYPH) g_opts.anaglyph = !g_opts.anaglyph;
    else if (opt == OPT_LIMIT_FRAMERATE) g_opts.limit_framerate = !g_opts.limit_framerate;
    else if (opt == OPT_DIFFICULTY) g_opts.difficulty = (g_opts.difficulty + delta) & 3;
    else if (opt == OPT_GRAPHICS) g_opts.fancy_graphics = !g_opts.fancy_graphics;
    save_options();
}


static int lwjgl_to_vk(int code) {
    switch (code) {
        case 17: return 'W';
        case 30: return 'A';
        case 31: return 'S';
        case 32: return 'D';
        case 57: return VK_SPACE;
        case 42: return VK_SHIFT;
        case 16: return 'Q';
        case 23: return 'I';
        case 20: return 'T';
        case 33: return 'F';
        case 1: return VK_ESCAPE;
        case 14: return VK_BACK;
        case 28: return VK_RETURN;
        default: return code;
    }
}

static int vk_to_lwjgl(int vk) {
    switch (vk) {
        case 'W': return 17;
        case 'A': return 30;
        case 'S': return 31;
        case 'D': return 32;
        case VK_SPACE: return 57;
        case VK_SHIFT: return 42;
        case 'Q': return 16;
        case 'I': return 23;
        case 'T': return 20;
        case 'F': return 33;
        case VK_ESCAPE: return 1;
        case VK_BACK: return 14;
        case VK_RETURN: return 28;
        default: return vk;
    }
}

static const char *key_name(int vk, char *buf, size_t cap) {
    switch (vk) {
        case VK_SPACE: return "SPACE";
        case VK_SHIFT: return "LSHIFT";
        case VK_CONTROL: return "LCONTROL";
        case VK_MENU: return "LMENU";
        case VK_ESCAPE: return "ESCAPE";
        case VK_RETURN: return "RETURN";
        case VK_BACK: return "BACK";
        case VK_TAB: return "TAB";
        case VK_LEFT: return "LEFT";
        case VK_RIGHT: return "RIGHT";
        case VK_UP: return "UP";
        case VK_DOWN: return "DOWN";
        default: break;
    }
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        snprintf(buf, cap, "%c", (char)vk);
        return buf;
    }
    UINT scan = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC) << 16;
    LONG lparam = (LONG)scan;
    if (GetKeyNameTextA(lparam, buf, (int)cap) > 0) return buf;
    snprintf(buf, cap, "%d", vk);
    return buf;
}

