/* Split from original monolithic main.c. Included by src/main.c unity build. */

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static void stivufine_set_defaults(void);
static float get_option_float(OptionId opt);
static void set_option_float(OptionId opt, float v);

static void set_default_options(void) {
    g_opts.music = 1.0f;
    g_opts.sound = 1.0f;
    g_opts.sensitivity = 0.5f;
    g_opts.invert_mouse = 0;
    g_opts.render_distance = 8;
    g_opts.gui_scale = 0; /* Auto, matching GameSettings.guiScale */
    g_opts.view_bobbing = 1;
    g_opts.anaglyph = 0; /* V-Sync */
    g_opts.max_fps = 0;  /* unlimited */
    g_opts.fov = 70.0f;
    g_opts.difficulty = 2;
    g_opts.fancy_graphics = 1;
    g_opts.fullscreen = 0;
    g_opts.show_fps = 0;
    g_opts.render_scale_percent = 100;
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = g_opts.renderer_backend;
#ifdef PEX_PLATFORM_PSP
    g_opts.ignore_classic_resources_warning = 1;
    g_opts.download_classic_textures = 0;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
#else
    g_opts.ignore_classic_resources_warning = 0;
    g_opts.download_classic_textures = 1;
    /* legacy.json audio/language assets are managed from Options -> Assets,
       not from the startup classic texture downloader. */
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "Default");
#endif
    g_opts.skin_path[0] = 0;
    g_opts.last_server[0] = 0;
    snprintf(g_opts.username, sizeof(g_opts.username), "Player");
    g_opts.name_set = 0;
    g_opts.tv_remote_mapped = 0;
    g_resource_pack_cache_revision = 0;
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) g_opts.tv_remote_map[i] = 0;
    snprintf(g_opts.language, sizeof(g_opts.language), "en_US");
    pex_set_language_code(g_opts.language);
    for (int i = 0; i < PEX_KEY_BIND_COUNT; i++) g_opts.keys[i] = default_keys[i];
    stivufine_set_defaults();
}

static float parse_float_java(const char *s) {
    if (!strcmp(s, "true")) return 1.0f;
    if (!strcmp(s, "false")) return 0.0f;
    return (float)atof(s);
}


static int parse_renderer_backend(const char *s) {
    if (!s || !*s) return RENDERER_OPENGL;
    for (int i = 0; i < RENDERER_COUNT; i++) {
        if (!strcmp(s, renderer_backend_keys[i])) return i;
        if (!strcmp(s, renderer_backend_names[i])) return i;
    }
    /* Friendly aliases for hand-edited options.txt files. */
    if (!strcmp(s, "OpenGL half") || !strcmp(s, "OpenGL 0.5") || !strcmp(s, "opengl0.5") || !strcmp(s, "opengl_half")) return RENDERER_OPENGL;
    if (!strcmp(s, "d3d11") || !strcmp(s, "direct3d11") || !strcmp(s, "Direct3D 11")) return RENDERER_D3D11;
    if (!strcmp(s, "directx") || !strcmp(s, "d3d") || !strcmp(s, "d3d9") || !strcmp(s, "direct3d") || !strcmp(s, "direct3d9") || !strcmp(s, "Direct3D 9")) return RENDERER_D3D9;
    if (!strcmp(s, "soft") || !strcmp(s, "software_renderer") || !strcmp(s, "software")) return RENDERER_OPENGL;
    return RENDERER_OPENGL;
}

static int renderer_backend_supported(int backend) {
#if defined(PEX_PLATFORM_SDL2) || defined(PEX_PLATFORM_PSP)
    return backend == RENDERER_OPENGL;
#else
    return backend == RENDERER_OPENGL || backend == RENDERER_D3D9 || backend == RENDERER_D3D11;
#endif
}

static const char *renderer_backend_label(int backend) {
#ifdef PEX_PLATFORM_PSP
    (void)backend;
    return "PSP GU (fixed)";
#else
    if (backend < 0 || backend >= RENDERER_COUNT) backend = RENDERER_OPENGL;
    return renderer_backend_names[backend];
#endif
}


typedef enum StivuFineCategory {
    SF_CAT_MAIN = 0,
    SF_CAT_DETAILS,
    SF_CAT_QUALITY,
    SF_CAT_ANIMATIONS,
    SF_CAT_PERFORMANCE,
    SF_CAT_OTHER
} StivuFineCategory;

typedef enum StivuFineOptionKind {
    SF_KIND_BUTTON = 0,
    SF_KIND_SLIDER = 1
} StivuFineOptionKind;

typedef struct StivuFineOptionDef {
    StivuFineOptionId id;
    const char *label;
    StivuFineCategory category;
    StivuFineOptionKind kind;
    int implemented;
} StivuFineOptionDef;

static const StivuFineOptionDef stivufine_defs[] = {
    { SF_GRAPHICS, "Graphics", SF_CAT_MAIN, SF_KIND_BUTTON, 1 },
    { SF_RENDER_DISTANCE_FINE, "Render Distance", SF_CAT_MAIN, SF_KIND_SLIDER, 1 },
    { SF_AO_LEVEL, "Smooth Lighting", SF_CAT_MAIN, SF_KIND_SLIDER, 1 },
    { SF_FRAMERATE_LIMIT, "Max FPS", SF_CAT_MAIN, SF_KIND_SLIDER, 1 },
    { SF_ANAGLYPH, "3D Anaglyph", SF_CAT_MAIN, SF_KIND_BUTTON, 0 },
    { SF_VIEW_BOBBING, "View Bobbing", SF_CAT_MAIN, SF_KIND_BUTTON, 1 },
    { SF_GUI_SCALE, "GUI Scale", SF_CAT_MAIN, SF_KIND_BUTTON, 1 },
    { SF_ADVANCED_OPENGL, "Advanced OpenGL", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 0 },
    { SF_GAMMA, "Brightness", SF_CAT_QUALITY, SF_KIND_SLIDER, 1 },
        { SF_FOG_FANCY, "Fog", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },
    { SF_FOG_START, "Fog Start", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },

    { SF_CLOUDS, "Clouds", SF_CAT_DETAILS, SF_KIND_BUTTON, 1 },
    { SF_CLOUD_HEIGHT, "Cloud Height", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },
    { SF_TREES, "Trees", SF_CAT_DETAILS, SF_KIND_BUTTON, 1 },
        { SF_WATER, "Water", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },
    { SF_RAIN, "Rain & Snow", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },
    { SF_SKY, "Sky", SF_CAT_DETAILS, SF_KIND_BUTTON, 1 },
    { SF_STARS, "Stars", SF_CAT_DETAILS, SF_KIND_BUTTON, 1 },
    { SF_SUN_MOON, "Sun & Moon", SF_CAT_DETAILS, SF_KIND_BUTTON, 1 },
    { SF_SHOW_CAPES, "Show Capes", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },
    { SF_DEPTH_FOG, "Depth Fog", SF_CAT_DETAILS, SF_KIND_BUTTON, 0 },

    { SF_MIPMAP_LEVEL, "Mipmap Level", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_MIPMAP_TYPE, "Mipmap Type", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_CLEAR_WATER, "Clear Water", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_RANDOM_MOBS, "Random Mobs", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_BETTER_GRASS, "Better Grass", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_BETTER_SNOW, "Better Snow", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_CUSTOM_FONTS, "Custom Fonts", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_CUSTOM_COLORS, "Custom Colors", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_SWAMP_COLORS, "Swamp Colors", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_SMOOTH_BIOMES, "Smooth Biomes", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_CONNECTED_TEXTURES, "Connected Textures", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_NATURAL_TEXTURES, "Natural Textures", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_AA_LEVEL, "Antialiasing", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },
    { SF_AF_LEVEL, "Anisotropic Filtering", SF_CAT_QUALITY, SF_KIND_BUTTON, 1 },

    { SF_ANIMATED_WATER, "Water Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_ANIMATED_LAVA, "Lava Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_ANIMATED_FIRE, "Fire Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_ANIMATED_PORTAL, "Portal Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_ANIMATED_REDSTONE, "Redstone Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_ANIMATED_EXPLOSION, "Explosion Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_ANIMATED_FLAME, "Flame Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_ANIMATED_SMOKE, "Smoke Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_VOID_PARTICLES, "Void Particles", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_WATER_PARTICLES, "Water Particles", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_RAIN_SPLASH, "Rain Splash", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_PORTAL_PARTICLES, "Portal Particles", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_PARTICLES, "Particles", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_DRIPPING_WATER_LAVA, "Dripping Water/Lava", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_ANIMATED_TERRAIN, "Terrain Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },
    { SF_ANIMATED_ITEMS, "Items Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 0 },
    { SF_ANIMATED_TEXTURES, "Textures Animated", SF_CAT_ANIMATIONS, SF_KIND_BUTTON, 1 },

    { SF_SMOOTH_FPS, "Smooth FPS", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 0 },
    { SF_SMOOTH_INPUT, "Smooth Input", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 0 },
    { SF_LOAD_FAR, "Load Far", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 0 },
    { SF_PRELOADED_CHUNKS, "Preloaded Chunks", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 0 },
    { SF_CHUNK_UPDATES, "Chunk Updates", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 1 },
    { SF_CHUNK_UPDATES_DYNAMIC, "Dynamic Updates", SF_CAT_PERFORMANCE, SF_KIND_BUTTON, 1 },

    { SF_FAST_DEBUG_INFO, "Fast Debug Info", SF_CAT_OTHER, SF_KIND_BUTTON, 0 },
    { SF_PROFILER, "Debug Profiler", SF_CAT_OTHER, SF_KIND_BUTTON, 0 },
    { SF_WEATHER, "Weather", SF_CAT_OTHER, SF_KIND_BUTTON, 0 },
    { SF_TIME, "Time", SF_CAT_OTHER, SF_KIND_BUTTON, 0 },
    { SF_FULLSCREEN_MODE, "Fullscreen", SF_CAT_OTHER, SF_KIND_BUTTON, 0 },
    { SF_AUTOSAVE_TICKS, "Autosave", SF_CAT_OTHER, SF_KIND_BUTTON, 0 }
};

static const StivuFineOptionDef *stivufine_find_def(StivuFineOptionId id) {
    for (int i = 0; i < (int)ARRAY_COUNT(stivufine_defs); ++i) if (stivufine_defs[i].id == id) return &stivufine_defs[i];
    return NULL;
}

static int stivufine_screen_category(ScreenId screen) {
    if (screen == SCREEN_STIVUFINE) return SF_CAT_MAIN;
    if (screen == SCREEN_STIVUFINE_DETAILS) return SF_CAT_DETAILS;
    if (screen == SCREEN_STIVUFINE_QUALITY) return SF_CAT_QUALITY;
    if (screen == SCREEN_STIVUFINE_ANIMATIONS) return SF_CAT_ANIMATIONS;
    if (screen == SCREEN_STIVUFINE_PERFORMANCE) return SF_CAT_PERFORMANCE;
    if (screen == SCREEN_STIVUFINE_OTHER) return SF_CAT_OTHER;
    return -1;
}

static const char *stivufine_screen_title_key(ScreenId screen, const char **fallback) {
    if (fallback) *fallback = "StivuFine";
    if (screen == SCREEN_STIVUFINE_DETAILS) { if (fallback) *fallback = "Detail Settings"; return "sf.options.detailsTitle"; }
    if (screen == SCREEN_STIVUFINE_QUALITY) { if (fallback) *fallback = "Quality Settings"; return "sf.options.qualityTitle"; }
    if (screen == SCREEN_STIVUFINE_ANIMATIONS) { if (fallback) *fallback = "Animation Settings"; return "sf.options.animationsTitle"; }
    if (screen == SCREEN_STIVUFINE_PERFORMANCE) { if (fallback) *fallback = "Performance Settings"; return "sf.options.performanceTitle"; }
    if (screen == SCREEN_STIVUFINE_OTHER) { if (fallback) *fallback = "Other Settings"; return "sf.options.otherTitle"; }
    return NULL;
}

static const char *stivufine_screen_title(ScreenId screen) {
    static char title[96];
    const char *fallback = "StivuFine";
    const char *key = stivufine_screen_title_key(screen, &fallback);
    if (!key) return "StivuFine";
    snprintf(title, sizeof(title), "StivuFine %s", tr_key_default(key, fallback));
    return title;
}

static const char *stivufine_option_lang_key(StivuFineOptionId id) {
    switch (id) {
        case SF_GRAPHICS: return "options.graphics";
        case SF_RENDER_DISTANCE_FINE: return "options.renderDistance";
        case SF_AO_LEVEL: return "options.ao";
        case SF_FRAMERATE_LIMIT: return "options.framerateLimit";
        case SF_ANAGLYPH: return "options.anaglyph";
        case SF_VIEW_BOBBING: return "options.viewBobbing";
        case SF_GUI_SCALE: return "options.guiScale";
        case SF_GAMMA: return "options.gamma";
        case SF_FOG_FANCY: return "sf.options.FOG_FANCY";
        case SF_FOG_START: return "sf.options.FOG_START";
        case SF_CLOUDS: return "sf.options.CLOUDS";
        case SF_CLOUD_HEIGHT: return "sf.options.CLOUD_HEIGHT";
        case SF_TREES: return "sf.options.TREES";
        case SF_WATER: return "sf.options.WATER";
        case SF_RAIN: return "sf.options.RAIN";
        case SF_SKY: return "sf.options.SKY";
        case SF_STARS: return "sf.options.STARS";
        case SF_SUN_MOON: return "sf.options.SUN_MOON";
        case SF_SHOW_CAPES: return "sf.options.SHOW_CAPES";
        case SF_DEPTH_FOG: return "sf.options.DEPTH_FOG";
        case SF_MIPMAP_LEVEL: return "sf.options.MIPMAP_LEVEL";
        case SF_MIPMAP_TYPE: return "sf.options.MIPMAP_TYPE";
        case SF_CLEAR_WATER: return "sf.options.CLEAR_WATER";
        case SF_RANDOM_MOBS: return "sf.options.RANDOM_MOBS";
        case SF_BETTER_GRASS: return "sf.options.BETTER_GRASS";
        case SF_BETTER_SNOW: return "sf.options.BETTER_SNOW";
        case SF_CUSTOM_FONTS: return "sf.options.CUSTOM_FONTS";
        case SF_CUSTOM_COLORS: return "sf.options.CUSTOM_COLORS";
        case SF_SWAMP_COLORS: return "sf.options.SWAMP_COLORS";
        case SF_SMOOTH_BIOMES: return "sf.options.SMOOTH_BIOMES";
        case SF_CONNECTED_TEXTURES: return "sf.options.CONNECTED_TEXTURES";
        case SF_NATURAL_TEXTURES: return "sf.options.NATURAL_TEXTURES";
        case SF_AA_LEVEL: return "sf.options.AA_LEVEL";
        case SF_AF_LEVEL: return "sf.options.AF_LEVEL";
        case SF_ANIMATED_WATER: return "sf.options.ANIMATED_WATER";
        case SF_ANIMATED_LAVA: return "sf.options.ANIMATED_LAVA";
        case SF_ANIMATED_FIRE: return "sf.options.ANIMATED_FIRE";
        case SF_ANIMATED_PORTAL: return "sf.options.ANIMATED_PORTAL";
        case SF_ANIMATED_REDSTONE: return "sf.options.ANIMATED_REDSTONE";
        case SF_ANIMATED_EXPLOSION: return "sf.options.ANIMATED_EXPLOSION";
        case SF_ANIMATED_FLAME: return "sf.options.ANIMATED_FLAME";
        case SF_ANIMATED_SMOKE: return "sf.options.ANIMATED_SMOKE";
        case SF_VOID_PARTICLES: return "sf.options.VOID_PARTICLES";
        case SF_WATER_PARTICLES: return "sf.options.WATER_PARTICLES";
        case SF_RAIN_SPLASH: return "sf.options.RAIN_SPLASH";
        case SF_PORTAL_PARTICLES: return "sf.options.PORTAL_PARTICLES";
        case SF_PARTICLES: return "options.particles";
        case SF_DRIPPING_WATER_LAVA: return "sf.options.DRIPPING_WATER_LAVA";
        case SF_ANIMATED_TERRAIN: return "sf.options.ANIMATED_TERRAIN";
        case SF_ANIMATED_ITEMS: return "sf.options.ANIMATED_ITEMS";
        case SF_ANIMATED_TEXTURES: return "sf.options.ANIMATED_TEXTURES";
        case SF_SMOOTH_FPS: return "sf.options.SMOOTH_FPS";
        case SF_SMOOTH_INPUT: return "sf.options.SMOOTH_INPUT";
        case SF_LOAD_FAR: return "sf.options.LOAD_FAR";
        case SF_PRELOADED_CHUNKS: return "sf.options.PRELOADED_CHUNKS";
        case SF_CHUNK_UPDATES: return "sf.options.CHUNK_UPDATES";
        case SF_CHUNK_UPDATES_DYNAMIC: return "sf.options.CHUNK_UPDATES_DYNAMIC";
        case SF_FAST_DEBUG_INFO: return "sf.options.FAST_DEBUG_INFO";
        case SF_PROFILER: return "sf.options.PROFILER";
        case SF_WEATHER: return "sf.options.WEATHER";
        case SF_TIME: return "sf.options.TIME";
        case SF_FULLSCREEN_MODE: return "sf.options.FULLSCREEN_MODE";
        case SF_AUTOSAVE_TICKS: return "sf.options.AUTOSAVE_TICKS";
        default: return NULL;
    }
}

static const char *stivufine_option_localized_name(const StivuFineOptionDef *d) {
    const char *key;
    if (!d) return "StivuFine";
    key = stivufine_option_lang_key(d->id);
    return key ? tr_key_default(key, d->label) : d->label;
}

static const char *stivufine_ui_on(void) { return tr_key_default("options.on", "ON"); }
static const char *stivufine_ui_off(void) { return tr_key_default("options.off", "OFF"); }
static const char *stivufine_ui_fast(void) { return tr_key_default("options.graphics.fast", "Fast"); }
static const char *stivufine_ui_fancy(void) { return tr_key_default("options.graphics.fancy", "Fancy"); }
static const char *stivufine_ui_default(void) { return tr_key_default("generator.default", "Default"); }

static int stivufine_option_count_for_screen(ScreenId screen) {
    int cat = stivufine_screen_category(screen);
    int count = 0;
    if (cat < 0) return 0;
    for (int i = 0; i < (int)ARRAY_COUNT(stivufine_defs); ++i) if ((int)stivufine_defs[i].category == cat) count++;
    return count;
}

static StivuFineOptionId stivufine_option_at(ScreenId screen, int index) {
    int cat = stivufine_screen_category(screen);
    int count = 0;
    if (cat < 0) return SF_GRAPHICS;
    for (int i = 0; i < (int)ARRAY_COUNT(stivufine_defs); ++i) {
        if ((int)stivufine_defs[i].category != cat) continue;
        if (count == index) return stivufine_defs[i].id;
        count++;
    }
    return SF_GRAPHICS;
}

static int stivufine_option_enabled(StivuFineOptionId id) {
    const StivuFineOptionDef *d = stivufine_find_def(id);
    return d ? d->implemented : 0;
}

static int stivufine_option_is_slider(StivuFineOptionId id) {
    const StivuFineOptionDef *d = stivufine_find_def(id);
    return d && d->implemented && d->kind == SF_KIND_SLIDER;
}

static const char *stivufine_mode_default_fast_fancy_off(int v) {
    if (v == SF_FAST) return stivufine_ui_fast();
    if (v == SF_FANCY) return stivufine_ui_fancy();
    if (v == SF_OFF) return stivufine_ui_off();
    return stivufine_ui_default();
}

static const char *stivufine_mode_default_fast_fancy(int v) {
    if (v == SF_FAST) return stivufine_ui_fast();
    if (v == SF_FANCY) return stivufine_ui_fancy();
    return stivufine_ui_default();
}

static const char *stivufine_on_off(int v) { return v ? stivufine_ui_on() : stivufine_ui_off(); }

static const char *stivufine_anim_mode_label(int v) {
    if (v == SF_ANIM_DYNAMIC) return tr_key_default("sf.options.animation.dynamic", "Dynamic");
    if (v == SF_ANIM_OFF) return stivufine_ui_off();
    return stivufine_ui_on();
}

static int stivufine_parse_bool(const char *v) {
    return v && (!strcmp(v, "true") || !strcmp(v, "1") || !strcmp(v, "yes") || !strcmp(v, "on") || !strcmp(v, "ON"));
}

static void stivufine_set_defaults(void) {
    g_opts.sf_fog_type = 1;
    g_opts.sf_fog_start = 0.8f;
    g_opts.sf_ao_level = 1.0f;
    g_opts.sf_clouds = SF_DEFAULT;
    g_opts.sf_cloud_height = 0.0f;
    g_opts.sf_trees = SF_DEFAULT;
    g_opts.sf_grass = SF_OFF;
    g_opts.sf_water = SF_DEFAULT;
    g_opts.sf_rain = SF_DEFAULT;
    g_opts.sf_sky = 1;
    g_opts.sf_stars = 1;
    g_opts.sf_sun_moon = 1;
    g_opts.sf_depth_fog = 1;
    g_opts.sf_animated_water = SF_ANIM_ON;
    g_opts.sf_animated_lava = SF_ANIM_ON;
    g_opts.sf_animated_fire = 1;
    g_opts.sf_animated_portal = 1;
    g_opts.sf_animated_redstone = 1;
    g_opts.sf_animated_explosion = 1;
    g_opts.sf_animated_flame = 1;
    g_opts.sf_animated_smoke = 1;
    g_opts.sf_void_particles = 1;
    g_opts.sf_water_particles = 1;
    g_opts.sf_rain_splash = 1;
    g_opts.sf_portal_particles = 1;
    g_opts.sf_dripping_water_lava = 1;
    g_opts.sf_animated_terrain = 1;
    g_opts.sf_animated_items = 1;
    g_opts.sf_animated_textures = 1;
    g_opts.sf_particles = 0;
    g_opts.sf_chunk_updates = 1;
    g_opts.sf_chunk_updates_dynamic = 0;
    g_opts.sf_fast_debug_info = 0;
    g_opts.sf_profiler = 0;
    g_opts.sf_weather = 1;
    g_opts.sf_gamma = 0.0f;
    g_opts.sf_clear_water = 0;
    g_opts.sf_better_snow = 0;
    g_opts.sf_swamp_colors = 1;
    g_opts.sf_smooth_biomes = 1;
    g_opts.sf_custom_fonts = 1;
    g_opts.sf_custom_colors = 1;
    g_opts.sf_random_mobs = 1;
    g_opts.sf_mipmap_level = 0;
    g_opts.sf_mipmap_linear = 0;
    g_opts.sf_connected_textures = 2;
    g_opts.sf_natural_textures = 0;
    g_opts.sf_aa_level = 0;
    g_opts.sf_af_level = 1;
}

static void stivufine_clamp_options(void) {
    if (g_opts.sf_fog_type < 1 || g_opts.sf_fog_type > 3) g_opts.sf_fog_type = 1;
    if (g_opts.sf_fog_start < 0.2f) g_opts.sf_fog_start = 0.2f;
    if (g_opts.sf_fog_start > 0.8f) g_opts.sf_fog_start = 0.8f;
    if (g_opts.sf_ao_level < 0.0f) g_opts.sf_ao_level = 0.0f;
    if (g_opts.sf_ao_level > 1.0f) g_opts.sf_ao_level = 1.0f;
    if (g_opts.sf_clouds < 0 || g_opts.sf_clouds > 3) g_opts.sf_clouds = 0;
    if (g_opts.sf_cloud_height < 0.0f) g_opts.sf_cloud_height = 0.0f;
    if (g_opts.sf_cloud_height > 1.0f) g_opts.sf_cloud_height = 1.0f;
    if (g_opts.sf_trees < 0 || g_opts.sf_trees > 2) g_opts.sf_trees = 0;
    if (g_opts.sf_grass < 1 || g_opts.sf_grass > 3) g_opts.sf_grass = 3;
    if (g_opts.sf_water < 0 || g_opts.sf_water > 2) g_opts.sf_water = 0;
    if (g_opts.sf_rain < 0 || g_opts.sf_rain > 3) g_opts.sf_rain = 0;
    if (g_opts.sf_animated_water < 0 || g_opts.sf_animated_water > 2) g_opts.sf_animated_water = 0;
    if (g_opts.sf_animated_lava < 0 || g_opts.sf_animated_lava > 2) g_opts.sf_animated_lava = 0;
    if (g_opts.sf_particles < 0 || g_opts.sf_particles > 2) g_opts.sf_particles = 0;
    if (g_opts.sf_gamma < 0.0f) g_opts.sf_gamma = 0.0f;
    if (g_opts.sf_gamma > 1.0f) g_opts.sf_gamma = 1.0f;
    if (g_opts.sf_clear_water < 0 || g_opts.sf_clear_water > 1) g_opts.sf_clear_water = 0;
    if (g_opts.sf_better_snow < 0 || g_opts.sf_better_snow > 1) g_opts.sf_better_snow = 0;
    if (g_opts.sf_swamp_colors < 0 || g_opts.sf_swamp_colors > 1) g_opts.sf_swamp_colors = 1;
    if (g_opts.sf_smooth_biomes < 0 || g_opts.sf_smooth_biomes > 1) g_opts.sf_smooth_biomes = 1;
    if (g_opts.sf_custom_fonts < 0 || g_opts.sf_custom_fonts > 1) g_opts.sf_custom_fonts = 1;
    if (g_opts.sf_custom_colors < 0 || g_opts.sf_custom_colors > 1) g_opts.sf_custom_colors = 1;
    if (g_opts.sf_random_mobs < 0 || g_opts.sf_random_mobs > 1) g_opts.sf_random_mobs = 1;
    if (g_opts.sf_mipmap_level < 0) g_opts.sf_mipmap_level = 0;
    if (g_opts.sf_mipmap_level > 4) g_opts.sf_mipmap_level = 4;
    if (g_opts.sf_mipmap_linear < 0 || g_opts.sf_mipmap_linear > 1) g_opts.sf_mipmap_linear = 0;
    if (g_opts.sf_connected_textures < 1 || g_opts.sf_connected_textures > 3) g_opts.sf_connected_textures = 2;
    if (g_opts.sf_natural_textures < 0 || g_opts.sf_natural_textures > 1) g_opts.sf_natural_textures = 0;
    { int aa = g_opts.sf_aa_level; if (!(aa==0||aa==2||aa==4||aa==6||aa==8||aa==12||aa==16)) g_opts.sf_aa_level = 0; }
    { int af = g_opts.sf_af_level; if (!(af==1||af==2||af==4||af==8||af==16)) g_opts.sf_af_level = 1; }
    if (g_opts.sf_chunk_updates < 1) g_opts.sf_chunk_updates = 1;
    if (g_opts.sf_chunk_updates > 5) g_opts.sf_chunk_updates = 5;
}

static void stivufine_load_file_named(const char *name) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    (void)name;
#else
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, name);
    FILE *f = fopen(path, "r");
    if (f) {
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = strchr(line, ':');
        if (!p) continue;
        *p++ = 0;
        char *k = trim(line);
        char *v = trim(p);
        if (!strcmp(k, "sfFogType")) g_opts.sf_fog_type = atoi(v);
        else if (!strcmp(k, "sfFogStart")) g_opts.sf_fog_start = parse_float_java(v);
        else if (!strcmp(k, "sfAoLevel")) g_opts.sf_ao_level = parse_float_java(v);
        else if (!strcmp(k, "sfClouds")) g_opts.sf_clouds = atoi(v);
        else if (!strcmp(k, "sfCloudsHeight")) g_opts.sf_cloud_height = parse_float_java(v);
        else if (!strcmp(k, "sfTrees")) g_opts.sf_trees = atoi(v);
        else if (!strcmp(k, "sfGrass") || !strcmp(k, "sfBetterGrass")) { g_opts.sf_grass = atoi(v); if (g_opts.sf_grass <= 0) g_opts.sf_grass = 3; }
        else if (!strcmp(k, "sfWater")) g_opts.sf_water = atoi(v);
        else if (!strcmp(k, "sfRain")) g_opts.sf_rain = atoi(v);
        else if (!strcmp(k, "sfSky")) g_opts.sf_sky = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfStars")) g_opts.sf_stars = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfSunMoon")) g_opts.sf_sun_moon = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfDepthFog")) g_opts.sf_depth_fog = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedWater")) g_opts.sf_animated_water = atoi(v);
        else if (!strcmp(k, "sfAnimatedLava")) g_opts.sf_animated_lava = atoi(v);
        else if (!strcmp(k, "sfAnimatedFire")) g_opts.sf_animated_fire = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedPortal")) g_opts.sf_animated_portal = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedRedstone")) g_opts.sf_animated_redstone = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedExplosion")) g_opts.sf_animated_explosion = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedFlame")) g_opts.sf_animated_flame = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedSmoke")) g_opts.sf_animated_smoke = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfVoidParticles")) g_opts.sf_void_particles = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfWaterParticles")) g_opts.sf_water_particles = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfRainSplash")) g_opts.sf_rain_splash = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfPortalParticles")) g_opts.sf_portal_particles = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfDrippingWaterLava")) g_opts.sf_dripping_water_lava = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedTerrain")) g_opts.sf_animated_terrain = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedItems")) g_opts.sf_animated_items = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAnimatedTextures")) g_opts.sf_animated_textures = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfChunkUpdates")) g_opts.sf_chunk_updates = atoi(v);
        else if (!strcmp(k, "sfChunkUpdatesDynamic")) g_opts.sf_chunk_updates_dynamic = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfFastDebugInfo")) g_opts.sf_fast_debug_info = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfProfiler")) g_opts.sf_profiler = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfWeather")) g_opts.sf_weather = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfClearWater")) g_opts.sf_clear_water = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfBetterSnow")) g_opts.sf_better_snow = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfSwampColors")) g_opts.sf_swamp_colors = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfSmoothBiomes")) g_opts.sf_smooth_biomes = stivufine_parse_bool(v);
        else if (!strcmp(k, "gamma") || !strcmp(k, "sfGamma") || !strcmp(k, "sfBrightness")) g_opts.sf_gamma = parse_float_java(v);
        else if (!strcmp(k, "sfCustomFonts")) g_opts.sf_custom_fonts = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfCustomColors")) g_opts.sf_custom_colors = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfRandomMobs")) g_opts.sf_random_mobs = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfMipmapLevel")) g_opts.sf_mipmap_level = atoi(v);
        else if (!strcmp(k, "sfMipmapLinear")) g_opts.sf_mipmap_linear = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfConnectedTextures")) g_opts.sf_connected_textures = atoi(v);
        else if (!strcmp(k, "sfNaturalTextures")) g_opts.sf_natural_textures = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAaLevel")) g_opts.sf_aa_level = atoi(v);
        else if (!strcmp(k, "sfAfLevel")) g_opts.sf_af_level = atoi(v);
        else if (!strcmp(k, "particles") || !strcmp(k, "particleSetting")) g_opts.sf_particles = atoi(v);
    }
    fclose(f);
    stivufine_clamp_options();
    }
#endif
}

static void load_stivufine_options(void) {
    stivufine_load_file_named("optionsstivufine_legacy.txt");
    stivufine_load_file_named("optionsstivufine.txt");
}

static void save_stivufine_options(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    return;
#else
    stivufine_clamp_options();
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "optionsstivufine.txt");
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# StivuFine settings (StivuFine sf* keys)\n");
    fprintf(f, "sfFogType:%d\n", g_opts.sf_fog_type);
    fprintf(f, "sfFogStart:%g\n", g_opts.sf_fog_start);
    fprintf(f, "sfAoLevel:%g\n", g_opts.sf_ao_level);
    fprintf(f, "sfClouds:%d\n", g_opts.sf_clouds);
    fprintf(f, "sfCloudsHeight:%g\n", g_opts.sf_cloud_height);
    fprintf(f, "sfTrees:%d\n", g_opts.sf_trees);
    fprintf(f, "sfGrass:%d\n", g_opts.sf_grass);
    fprintf(f, "sfBetterGrass:%d\n", g_opts.sf_grass);
    fprintf(f, "sfWater:%d\n", g_opts.sf_water);
    fprintf(f, "sfRain:%d\n", g_opts.sf_rain);
    fprintf(f, "sfSky:%s\n", g_opts.sf_sky ? "true" : "false");
    fprintf(f, "sfStars:%s\n", g_opts.sf_stars ? "true" : "false");
    fprintf(f, "sfSunMoon:%s\n", g_opts.sf_sun_moon ? "true" : "false");
    fprintf(f, "sfDepthFog:%s\n", g_opts.sf_depth_fog ? "true" : "false");
    fprintf(f, "sfAnimatedWater:%d\n", g_opts.sf_animated_water);
    fprintf(f, "sfAnimatedLava:%d\n", g_opts.sf_animated_lava);
    fprintf(f, "sfAnimatedFire:%s\n", g_opts.sf_animated_fire ? "true" : "false");
    fprintf(f, "sfAnimatedPortal:%s\n", g_opts.sf_animated_portal ? "true" : "false");
    fprintf(f, "sfAnimatedRedstone:%s\n", g_opts.sf_animated_redstone ? "true" : "false");
    fprintf(f, "sfAnimatedExplosion:%s\n", g_opts.sf_animated_explosion ? "true" : "false");
    fprintf(f, "sfAnimatedFlame:%s\n", g_opts.sf_animated_flame ? "true" : "false");
    fprintf(f, "sfAnimatedSmoke:%s\n", g_opts.sf_animated_smoke ? "true" : "false");
    fprintf(f, "sfVoidParticles:%s\n", g_opts.sf_void_particles ? "true" : "false");
    fprintf(f, "sfWaterParticles:%s\n", g_opts.sf_water_particles ? "true" : "false");
    fprintf(f, "sfRainSplash:%s\n", g_opts.sf_rain_splash ? "true" : "false");
    fprintf(f, "sfPortalParticles:%s\n", g_opts.sf_portal_particles ? "true" : "false");
    fprintf(f, "sfDrippingWaterLava:%s\n", g_opts.sf_dripping_water_lava ? "true" : "false");
    fprintf(f, "sfAnimatedTerrain:%s\n", g_opts.sf_animated_terrain ? "true" : "false");
    fprintf(f, "sfAnimatedItems:%s\n", g_opts.sf_animated_items ? "true" : "false");
    fprintf(f, "sfAnimatedTextures:%s\n", g_opts.sf_animated_textures ? "true" : "false");
    fprintf(f, "particles:%d\n", g_opts.sf_particles);
    fprintf(f, "sfChunkUpdates:%d\n", g_opts.sf_chunk_updates);
    fprintf(f, "sfChunkUpdatesDynamic:%s\n", g_opts.sf_chunk_updates_dynamic ? "true" : "false");
    fprintf(f, "sfFastDebugInfo:%s\n", g_opts.sf_fast_debug_info ? "true" : "false");
    fprintf(f, "sfProfiler:%s\n", g_opts.sf_profiler ? "true" : "false");
    fprintf(f, "sfWeather:%s\n", g_opts.sf_weather ? "true" : "false");
    fprintf(f, "sfClearWater:%s\n", g_opts.sf_clear_water ? "true" : "false");
    fprintf(f, "sfBetterSnow:%s\n", g_opts.sf_better_snow ? "true" : "false");
    fprintf(f, "sfSwampColors:%s\n", g_opts.sf_swamp_colors ? "true" : "false");
    fprintf(f, "sfSmoothBiomes:%s\n", g_opts.sf_smooth_biomes ? "true" : "false");
    fprintf(f, "gamma:%g\n", g_opts.sf_gamma);
    fprintf(f, "sfGamma:%g\n", g_opts.sf_gamma);
    fprintf(f, "sfCustomFonts:%s\n", g_opts.sf_custom_fonts ? "true" : "false");
    fprintf(f, "sfCustomColors:%s\n", g_opts.sf_custom_colors ? "true" : "false");
    fprintf(f, "sfRandomMobs:%s\n", g_opts.sf_random_mobs ? "true" : "false");
    fprintf(f, "sfMipmapLevel:%d\n", g_opts.sf_mipmap_level);
    fprintf(f, "sfMipmapLinear:%s\n", g_opts.sf_mipmap_linear ? "true" : "false");
    fprintf(f, "sfConnectedTextures:%d\n", g_opts.sf_connected_textures);
    fprintf(f, "sfNaturalTextures:%s\n", g_opts.sf_natural_textures ? "true" : "false");
    fprintf(f, "sfAaLevel:%d\n", g_opts.sf_aa_level);
    fprintf(f, "sfAfLevel:%d\n", g_opts.sf_af_level);
    fclose(f);
#endif
}

static int stivufine_cloud_mode(void) {
    if (g_opts.sf_clouds == SF_FAST || g_opts.sf_clouds == SF_FANCY || g_opts.sf_clouds == SF_OFF) return g_opts.sf_clouds;
    return g_opts.fancy_graphics ? SF_FANCY : SF_FAST;
}

static int stivufine_fancy_trees_enabled(void) {
    if (g_opts.sf_trees == SF_FAST) return 0;
    if (g_opts.sf_trees == SF_FANCY) return 1;
    return g_opts.fancy_graphics;
}

static int stivufine_better_grass_enabled(void) { return g_opts.sf_grass != SF_OFF; }
static int stivufine_better_grass_fancy_enabled(void) { return g_opts.sf_grass == SF_FANCY; }
static int stivufine_fancy_grass_enabled(void) { return g_opts.fancy_graphics || stivufine_better_grass_fancy_enabled(); }

static int stivufine_custom_fonts_enabled(void) { return g_opts.sf_custom_fonts; }
static int stivufine_custom_colors_enabled(void) { return g_opts.sf_custom_colors; }
static int stivufine_better_snow_enabled(void) { return g_opts.sf_better_snow; }
static int stivufine_swamp_colors_enabled(void) { return g_opts.sf_swamp_colors; }
static int stivufine_smooth_biomes_enabled(void) { return g_opts.sf_smooth_biomes; }
static int stivufine_random_mobs_enabled(void) { return g_opts.sf_random_mobs; }
static int stivufine_mipmap_level(void) { stivufine_clamp_options(); return g_opts.sf_mipmap_level; }
static int stivufine_mipmap_linear(void) { return g_opts.sf_mipmap_linear != 0; }
static int stivufine_connected_textures_mode(void) { return g_opts.sf_connected_textures; }
static int stivufine_natural_textures_enabled(void) { return g_opts.sf_natural_textures != 0; }
static int stivufine_aa_level(void) { return g_opts.sf_aa_level; }
static int stivufine_af_level(void) { return g_opts.sf_af_level; }

static int stivufine_smooth_lighting_enabled(void) {
    return g_opts.sf_ao_level > 0.0001f;
}

static int stivufine_textures_animated(void) {
    return g_opts.sf_animated_textures && g_opts.sf_animated_terrain;
}

static int stivufine_animate_water_texture(void) {
    return stivufine_textures_animated() && g_opts.sf_animated_water != SF_ANIM_OFF;
}

static int stivufine_animate_lava_texture(void) {
    return stivufine_textures_animated() && g_opts.sf_animated_lava != SF_ANIM_OFF;
}

static int stivufine_animate_portal_texture(void) {
    return stivufine_textures_animated() && g_opts.sf_animated_portal;
}

static int stivufine_particle_allowed(void) {
    if (g_opts.sf_particles <= 0) return 1;
    if (g_opts.sf_particles == 1) return (rand() & 1) == 0;
    return (rand() & 3) == 0;
}

static int stivufine_water_particles_enabled(void) { return g_opts.sf_water_particles && stivufine_particle_allowed(); }
static int stivufine_portal_particles_enabled(void) { return g_opts.sf_portal_particles && stivufine_particle_allowed(); }
static int stivufine_chunk_updates(void) { stivufine_clamp_options(); return g_opts.sf_chunk_updates; }
static int stivufine_dynamic_chunk_updates_enabled(void) { return g_opts.sf_chunk_updates_dynamic; }

static float get_stivufine_option_float(StivuFineOptionId opt) {
    if (opt == SF_RENDER_DISTANCE_FINE) {
        int c = g_opts.render_distance;
        if (c < 2) c = 2;
        if (c > 32) c = 32;
        return (float)(c - 2) / 30.0f;
    }
    if (opt == SF_AO_LEVEL) return g_opts.sf_ao_level;
    if (opt == SF_GAMMA) return g_opts.sf_gamma;
    if (opt == SF_FRAMERATE_LIMIT) return get_option_float(OPT_LIMIT_FRAMERATE);
    return 0.0f;
}

static void set_stivufine_option_float(StivuFineOptionId opt, float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (opt == SF_RENDER_DISTANCE_FINE) set_option_float(OPT_RENDER_DISTANCE, v);
    else if (opt == SF_AO_LEVEL) {
        g_opts.sf_ao_level = v;
        flat_mark_all_chunks_dirty();
        save_options();
    } else if (opt == SF_GAMMA) {
        g_opts.sf_gamma = v;
        flat_mark_all_chunks_dirty();
        save_options();
    } else if (opt == SF_FRAMERATE_LIMIT) set_option_float(OPT_LIMIT_FRAMERATE, v);
}

static void stivufine_render_distance_label(char *out, size_t cap) {
    int blocks = g_opts.render_distance * 16;
    const char *name = tr_key_default("options.renderDistance.tiny", "Tiny");
    const char *label = tr_key_default("options.renderDistance", "Render Distance");
    int base = 32;
    if (blocks >= 64) { name = tr_key_default("options.renderDistance.short", "Short"); base = 64; }
    if (blocks >= 128) { name = tr_key_default("options.renderDistance.normal", "Normal"); base = 128; }
    if (blocks >= 256) { name = tr_key_default("options.renderDistance.far", "Far"); base = 256; }
    if (blocks >= 512) { name = tr_key_default("sf.options.renderDistance.extreme", "Extreme"); base = 512; }
    int diff = blocks - base;
    if (diff == 0) snprintf(out, cap, "%s: %s", label, name);
    else snprintf(out, cap, "%s: %s +%d", label, name, diff);
}

static void get_stivufine_option_label(StivuFineOptionId opt, char *out, size_t cap) {
    const StivuFineOptionDef *d = stivufine_find_def(opt);
    const char *name = stivufine_option_localized_name(d);
    if (!d || !d->implemented) { snprintf(out, cap, "%s: N/A", name); return; }
    switch (opt) {
        case SF_GRAPHICS: snprintf(out, cap, "%s: %s", name, g_opts.fancy_graphics ? stivufine_ui_fancy() : stivufine_ui_fast()); break;
        case SF_RENDER_DISTANCE_FINE: stivufine_render_distance_label(out, cap); break;
        case SF_AO_LEVEL:
            if (g_opts.sf_ao_level <= 0.0f) snprintf(out, cap, "%s: %s", name, stivufine_ui_off());
            else snprintf(out, cap, "%s: %d%%", name, (int)(g_opts.sf_ao_level * 100.0f + 0.5f));
            break;
        case SF_FRAMERATE_LIMIT:
            if (g_opts.max_fps <= 0) snprintf(out, cap, "%s: %s", name, tr_key_default("options.framerateLimit.max", "Unlimited"));
            else snprintf(out, cap, "%s: %d", name, g_opts.max_fps);
            break;
        case SF_GUI_SCALE: {
            static const char *keys[4] = {
                "options.guiScale.auto", "options.guiScale.small",
                "options.guiScale.normal", "options.guiScale.large"
            };
            static const char *fallback[4] = { "Auto", "Small", "Normal", "Large" };
            int v = g_opts.gui_scale;
            if (v < 0 || v > 3) v = 0;
            snprintf(out, cap, "%s: %s", name, tr_key_default(keys[v], fallback[v]));
            break;
        }
        case SF_GAMMA:
            if (g_opts.sf_gamma <= 0.0f) snprintf(out, cap, "%s: %s", name, tr_key_default("options.gamma.min", "Moody"));
            else if (g_opts.sf_gamma >= 1.0f) snprintf(out, cap, "%s: %s", name, tr_key_default("options.gamma.max", "Bright"));
            else snprintf(out, cap, "%s: +%d%%", name, (int)(g_opts.sf_gamma * 100.0f));
            break;
        case SF_CLEAR_WATER: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_clear_water)); break;
        case SF_BETTER_SNOW: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_better_snow)); break;
        case SF_SWAMP_COLORS: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_swamp_colors)); break;
        case SF_SMOOTH_BIOMES: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_smooth_biomes)); break;
        case SF_RANDOM_MOBS: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_random_mobs)); break;
        case SF_MIPMAP_LEVEL:
            if (g_opts.sf_mipmap_level <= 0) snprintf(out, cap, "%s: %s", name, stivufine_ui_off());
            else if (g_opts.sf_mipmap_level >= 4) snprintf(out, cap, "%s: Max", name);
            else snprintf(out, cap, "%s: %d", name, g_opts.sf_mipmap_level);
            break;
        case SF_MIPMAP_TYPE: snprintf(out, cap, "%s: %s", name, g_opts.sf_mipmap_linear ? "Linear" : "Nearest"); break;
        case SF_CONNECTED_TEXTURES: snprintf(out, cap, "%s: %s", name, g_opts.sf_connected_textures == SF_FAST ? stivufine_ui_fast() : (g_opts.sf_connected_textures == SF_FANCY ? stivufine_ui_fancy() : stivufine_ui_off())); break;
        case SF_NATURAL_TEXTURES: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_natural_textures)); break;
        case SF_AA_LEVEL: if (g_opts.sf_aa_level <= 0) snprintf(out, cap, "%s: %s", name, stivufine_ui_off()); else snprintf(out, cap, "%s: %dx (restart)", name, g_opts.sf_aa_level); break;
        case SF_AF_LEVEL: if (g_opts.sf_af_level <= 1) snprintf(out, cap, "%s: %s", name, stivufine_ui_off()); else snprintf(out, cap, "%s: %dx", name, g_opts.sf_af_level); break;
        case SF_ANAGLYPH: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.anaglyph)); break;
        case SF_VIEW_BOBBING: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.view_bobbing)); break;
        case SF_RENDER_CLOUDS: snprintf(out, cap, "%s: %s", name, stivufine_cloud_mode() == SF_OFF ? stivufine_ui_off() : stivufine_ui_on()); break;
        case SF_CLOUDS: snprintf(out, cap, "%s: %s", name, stivufine_mode_default_fast_fancy_off(g_opts.sf_clouds)); break;
        case SF_TREES: snprintf(out, cap, "%s: %s", name, stivufine_mode_default_fast_fancy(g_opts.sf_trees)); break;
        case SF_GRASS:
        case SF_BETTER_GRASS: snprintf(out, cap, "%s: %s", name, g_opts.sf_grass == SF_FAST ? stivufine_ui_fast() : (g_opts.sf_grass == SF_FANCY ? stivufine_ui_fancy() : stivufine_ui_off())); break;
        case SF_CUSTOM_FONTS: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_custom_fonts)); break;
        case SF_CUSTOM_COLORS: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_custom_colors)); break;
        case SF_SKY: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_sky)); break;
        case SF_STARS: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_stars)); break;
        case SF_SUN_MOON: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_sun_moon)); break;
        case SF_ANIMATED_WATER: snprintf(out, cap, "%s: %s", name, stivufine_anim_mode_label(g_opts.sf_animated_water)); break;
        case SF_ANIMATED_LAVA: snprintf(out, cap, "%s: %s", name, stivufine_anim_mode_label(g_opts.sf_animated_lava)); break;
        case SF_ANIMATED_PORTAL: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_animated_portal)); break;
        case SF_WATER_PARTICLES: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_water_particles)); break;
        case SF_PORTAL_PARTICLES: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_portal_particles)); break;
        case SF_PARTICLES:
            snprintf(out, cap, "%s: %s", name, g_opts.sf_particles == 2 ? tr_key_default("options.particles.minimal", "Minimal") : (g_opts.sf_particles == 1 ? tr_key_default("options.particles.decreased", "Decreased") : tr_key_default("options.particles.all", "All")));
            break;
        case SF_ANIMATED_TERRAIN: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_animated_terrain)); break;
        case SF_ANIMATED_TEXTURES: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_animated_textures)); break;
        case SF_CHUNK_UPDATES: snprintf(out, cap, "%s: %d", name, g_opts.sf_chunk_updates); break;
        case SF_CHUNK_UPDATES_DYNAMIC: snprintf(out, cap, "%s: %s", name, stivufine_on_off(g_opts.sf_chunk_updates_dynamic)); break;
        default: snprintf(out, cap, "%s: N/A", name); break;
    }
}

static void bump_stivufine_option(StivuFineOptionId opt, int delta) {
    (void)delta;
    if (!stivufine_option_enabled(opt)) return;
    switch (opt) {
        case SF_GRAPHICS:
            g_opts.fancy_graphics = !g_opts.fancy_graphics;
            flat_mark_all_chunks_dirty();
            break;
        case SF_ANAGLYPH:
            g_opts.anaglyph = !g_opts.anaglyph;
            apply_vsync_setting();
            break;
        case SF_VIEW_BOBBING:
            g_opts.view_bobbing = !g_opts.view_bobbing;
            break;
        case SF_GUI_SCALE:
            g_opts.gui_scale = (g_opts.gui_scale + 1) & 3;
            break;
        case SF_RENDER_CLOUDS:
            g_opts.sf_clouds = (stivufine_cloud_mode() == SF_OFF) ? SF_DEFAULT : SF_OFF;
            break;
        case SF_CLOUDS:
            g_opts.sf_clouds = (g_opts.sf_clouds + 1) & 3;
            break;
        case SF_TREES:
            g_opts.sf_trees++;
            if (g_opts.sf_trees > 2) g_opts.sf_trees = 0;
            flat_mark_all_chunks_dirty();
            break;
        case SF_GRASS:
        case SF_BETTER_GRASS:
            g_opts.sf_grass++;
            if (g_opts.sf_grass > 3) g_opts.sf_grass = 1;
            flat_mark_all_chunks_dirty();
            break;
        case SF_SKY: g_opts.sf_sky = !g_opts.sf_sky; break;
        case SF_STARS: g_opts.sf_stars = !g_opts.sf_stars; break;
        case SF_SUN_MOON: g_opts.sf_sun_moon = !g_opts.sf_sun_moon; break;
        case SF_ANIMATED_WATER:
            g_opts.sf_animated_water++;
            if (g_opts.sf_animated_water > 2) g_opts.sf_animated_water = 0;
            break;
        case SF_ANIMATED_LAVA:
            g_opts.sf_animated_lava++;
            if (g_opts.sf_animated_lava > 2) g_opts.sf_animated_lava = 0;
            break;
        case SF_ANIMATED_PORTAL: g_opts.sf_animated_portal = !g_opts.sf_animated_portal; break;
        case SF_WATER_PARTICLES: g_opts.sf_water_particles = !g_opts.sf_water_particles; break;
        case SF_PORTAL_PARTICLES: g_opts.sf_portal_particles = !g_opts.sf_portal_particles; break;
        case SF_PARTICLES:
            g_opts.sf_particles = (g_opts.sf_particles + 1) % 3;
            break;
        case SF_ANIMATED_TERRAIN: g_opts.sf_animated_terrain = !g_opts.sf_animated_terrain; break;
        case SF_ANIMATED_TEXTURES: g_opts.sf_animated_textures = !g_opts.sf_animated_textures; break;
        case SF_CUSTOM_FONTS:
            g_opts.sf_custom_fonts = !g_opts.sf_custom_fonts;
            apply_texture_pack_index(g_selected_texpack);
            break;
        case SF_CUSTOM_COLORS:
            g_opts.sf_custom_colors = !g_opts.sf_custom_colors;
            apply_texture_pack_index(g_selected_texpack);
            flat_mark_all_chunks_dirty();
            break;
        case SF_CLEAR_WATER:
            g_opts.sf_clear_water = !g_opts.sf_clear_water;
            stivufine_update_water_opacity();
            break;
        case SF_BETTER_SNOW:
            g_opts.sf_better_snow = !g_opts.sf_better_snow;
            flat_mark_all_chunks_dirty();
            break;
        case SF_SWAMP_COLORS:
            g_opts.sf_swamp_colors = !g_opts.sf_swamp_colors;
            flat_mark_all_chunks_dirty();
            break;
        case SF_SMOOTH_BIOMES:
            g_opts.sf_smooth_biomes = !g_opts.sf_smooth_biomes;
            flat_mark_all_chunks_dirty();
            break;
        case SF_RANDOM_MOBS:
            g_opts.sf_random_mobs = !g_opts.sf_random_mobs;
            apply_texture_pack_index(g_selected_texpack);
            break;
        case SF_MIPMAP_LEVEL:
            g_opts.sf_mipmap_level++;
            if (g_opts.sf_mipmap_level > 4) g_opts.sf_mipmap_level = 0;
            apply_texture_pack_index(g_selected_texpack);
            break;
        case SF_MIPMAP_TYPE:
            g_opts.sf_mipmap_linear = !g_opts.sf_mipmap_linear;
            apply_texture_pack_index(g_selected_texpack);
            break;
        case SF_CONNECTED_TEXTURES:
            g_opts.sf_connected_textures++;
            if (g_opts.sf_connected_textures > 3) g_opts.sf_connected_textures = 1;
            apply_texture_pack_index(g_selected_texpack);
            flat_mark_all_chunks_dirty();
            break;
        case SF_NATURAL_TEXTURES:
            g_opts.sf_natural_textures = !g_opts.sf_natural_textures;
            apply_texture_pack_index(g_selected_texpack);
            flat_mark_all_chunks_dirty();
            break;
        case SF_AA_LEVEL: {
            static const int levels[] = {0,2,4,6,8,12,16};
            int i = 0; while (i < (int)ARRAY_COUNT(levels) && levels[i] != g_opts.sf_aa_level) ++i;
            g_opts.sf_aa_level = levels[(i + 1) % (int)ARRAY_COUNT(levels)];
            break;
        }
        case SF_AF_LEVEL:
            if (g_opts.sf_af_level <= 1) g_opts.sf_af_level = 2;
            else if (g_opts.sf_af_level >= 16) g_opts.sf_af_level = 1;
            else g_opts.sf_af_level <<= 1;
            apply_texture_pack_index(g_selected_texpack);
            break;
        case SF_CHUNK_UPDATES:
            g_opts.sf_chunk_updates++;
            if (g_opts.sf_chunk_updates > 5) g_opts.sf_chunk_updates = 1;
            break;
        case SF_CHUNK_UPDATES_DYNAMIC:
            g_opts.sf_chunk_updates_dynamic = !g_opts.sf_chunk_updates_dynamic;
            break;
        default: break;
    }
    stivufine_clamp_options();
    save_options();
}

static void load_options(void) {
    set_default_options();
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    /* PSP has no required Memory Stick writes; options live in RAM for this run. */
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    g_opts.ignore_classic_resources_warning = 1;
    g_opts.download_classic_textures = 0;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    return;
#endif
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "options.txt");
    FILE *f = fopen(path, "r");
    if (f) {
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
        else if (!strcmp(k, "viewDistance")) g_opts.render_distance = atoi(v);
        else if (!strcmp(k, "guiScale")) g_opts.gui_scale = atoi(v);
        else if (!strcmp(k, "bobView")) g_opts.view_bobbing = !strcmp(v, "true");
        else if (!strcmp(k, "anaglyph3d")) g_opts.anaglyph = !strcmp(v, "true");
        else if (!strcmp(k, "limitFramerate")) { if (!strcmp(v, "true")) g_opts.max_fps = 60; }
        else if (!strcmp(k, "maxFps")) g_opts.max_fps = atoi(v);
        else if (!strcmp(k, "fov") || !strcmp(k, "fieldOfView")) g_opts.fov = parse_float_java(v);
        else if (!strcmp(k, "gamma")) g_opts.sf_gamma = parse_float_java(v);
        else if (!strcmp(k, "sfClearWater")) g_opts.sf_clear_water = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfBetterSnow")) g_opts.sf_better_snow = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfSwampColors")) g_opts.sf_swamp_colors = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfSmoothBiomes")) g_opts.sf_smooth_biomes = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfRandomMobs")) g_opts.sf_random_mobs = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfMipmapLevel")) g_opts.sf_mipmap_level = atoi(v);
        else if (!strcmp(k, "sfMipmapLinear")) g_opts.sf_mipmap_linear = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfConnectedTextures")) g_opts.sf_connected_textures = atoi(v);
        else if (!strcmp(k, "sfNaturalTextures")) g_opts.sf_natural_textures = stivufine_parse_bool(v);
        else if (!strcmp(k, "sfAaLevel")) g_opts.sf_aa_level = atoi(v);
        else if (!strcmp(k, "sfAfLevel")) g_opts.sf_af_level = atoi(v);
        else if (!strcmp(k, "difficulty")) g_opts.difficulty = atoi(v) & 3;
        else if (!strcmp(k, "fancyGraphics")) g_opts.fancy_graphics = !strcmp(v, "true");
        else if (!strcmp(k, "fullscreen") || !strcmp(k, "enableFullscreen")) g_opts.fullscreen = !strcmp(v, "true");
        else if (!strcmp(k, "showFps") || !strcmp(k, "showFPSCounter")) g_opts.show_fps = !strcmp(v, "true");
        else if (!strcmp(k, "renderScalePercent") || !strcmp(k, "rendererResolutionPercent")) g_opts.render_scale_percent = atoi(v);
        else if (!strcmp(k, "renderer") || !strcmp(k, "rendererBackend")) g_opts.renderer_backend = parse_renderer_backend(v);
        else if (!strcmp(k, "ignoreClassicResourcesWarning") || !strcmp(k, "ignoreClassicPackWarning")) g_opts.ignore_classic_resources_warning = !strcmp(v, "true");
        else if (!strcmp(k, "downloadClassicTextures")) g_opts.download_classic_textures = !strcmp(v, "true");
        else if (!strcmp(k, "downloadClassicSounds")) g_opts.download_classic_sounds = !strcmp(v, "true");
        else if (!strcmp(k, "classicAudioMask")) g_opts.classic_audio_mask = atoi(v);
        else if (!strcmp(k, "ignoreClassicSoundsWarning")) g_opts.ignore_classic_sounds_warning = !strcmp(v, "true");
        else if (!strcmp(k, "skin")) snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", v);
        else if (!strcmp(k, "skinPath")) snprintf(g_opts.skin_path, sizeof(g_opts.skin_path), "%s", v);
        else if (!strcmp(k, "lastServer")) snprintf(g_opts.last_server, sizeof(g_opts.last_server), "%s", v);
        else if (!strcmp(k, "username")) snprintf(g_opts.username, sizeof(g_opts.username), "%s", v);
        else if (!strcmp(k, "isnameset") || !strcmp(k, "isNameSet") || !strcmp(k, "nameSet")) g_opts.name_set = (!strcmp(v, "true") || !strcmp(v, "1") || !strcmp(v, "yes"));
        else if (!strcmp(k, "tvRemoteMapped") || !strcmp(k, "tvRemote.mapped")) g_opts.tv_remote_mapped = (!strcmp(v, "true") || !strcmp(v, "1") || !strcmp(v, "yes"));
        else if (!strcmp(k, "resourcePackCacheRevision")) {
            g_resource_pack_cache_revision = atoi(v);
            if (g_resource_pack_cache_revision < 0) g_resource_pack_cache_revision = 0;
            if (g_resource_pack_cache_revision > 2) g_resource_pack_cache_revision = 2;
        }
        else if (!strncmp(k, "tvRemote.", 9)) {
            const char *name = k + 9;
            for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) {
                if (!strcmp(name, pex_tv_remote_action_key(i))) {
                    g_opts.tv_remote_map[i] = atoi(v);
                    break;
                }
            }
        }
        else if (!strcmp(k, "lang") || !strcmp(k, "language")) snprintf(g_opts.language, sizeof(g_opts.language), "%s", v);
        else if (!strncmp(k, "key_", 4)) {
            const char *name = k + 4;
            for (int i = 0; i < PEX_KEY_BIND_COUNT; i++) {
                if (!strcmp(name, key_option_names[i])) g_opts.keys[i] = lwjgl_to_vk(atoi(v));
            }
        }
    }
    fclose(f);
    }
    load_stivufine_options();
    if (g_opts.sf_gamma < 0.0f) g_opts.sf_gamma = 0.0f;
    if (g_opts.sf_gamma > 1.0f) g_opts.sf_gamma = 1.0f;
    if (g_opts.sf_clear_water < 0 || g_opts.sf_clear_water > 1) g_opts.sf_clear_water = 0;
    if (g_opts.sf_better_snow < 0 || g_opts.sf_better_snow > 1) g_opts.sf_better_snow = 0;
    if (g_opts.sf_swamp_colors < 0 || g_opts.sf_swamp_colors > 1) g_opts.sf_swamp_colors = 1;
    if (g_opts.sf_smooth_biomes < 0 || g_opts.sf_smooth_biomes > 1) g_opts.sf_smooth_biomes = 1;
    if (g_opts.sf_random_mobs < 0 || g_opts.sf_random_mobs > 1) g_opts.sf_random_mobs = 1;
    if (g_opts.sf_mipmap_level < 0) g_opts.sf_mipmap_level = 0;
    if (g_opts.sf_mipmap_level > 4) g_opts.sf_mipmap_level = 4;
    if (g_opts.sf_mipmap_linear < 0 || g_opts.sf_mipmap_linear > 1) g_opts.sf_mipmap_linear = 0;
    if (g_opts.sf_connected_textures < 1 || g_opts.sf_connected_textures > 3) g_opts.sf_connected_textures = 2;
    if (g_opts.sf_natural_textures < 0 || g_opts.sf_natural_textures > 1) g_opts.sf_natural_textures = 0;
    { int aa = g_opts.sf_aa_level; if (!(aa==0||aa==2||aa==4||aa==6||aa==8||aa==12||aa==16)) g_opts.sf_aa_level = 0; }
    { int af = g_opts.sf_af_level; if (!(af==1||af==2||af==4||af==8||af==16)) g_opts.sf_af_level = 1; }
    if (g_opts.gui_scale < 0 || g_opts.gui_scale > 3) g_opts.gui_scale = 0;
    if (g_opts.render_distance < 2) g_opts.render_distance = 8;
    if (g_opts.render_distance > 32) g_opts.render_distance = 32;
    if (g_opts.max_fps < 0) g_opts.max_fps = 0;
    if (g_opts.max_fps > MAX_FPS_CAP) g_opts.max_fps = MAX_FPS_CAP;
    if (g_opts.fov <= 1.0f) g_opts.fov = 70.0f + g_opts.fov * 40.0f;
    if (g_opts.fov < 70.0f) g_opts.fov = 70.0f;
    if (g_opts.fov > 110.0f) g_opts.fov = 110.0f;
    if (g_opts.render_scale_percent < 10) g_opts.render_scale_percent = 10;
    if (g_opts.render_scale_percent > 100) g_opts.render_scale_percent = 100;
    g_opts.render_scale_percent = 10 + ((g_opts.render_scale_percent - 10 + 5) / 10) * 10;
    if (g_opts.render_scale_percent > 100) g_opts.render_scale_percent = 100;
    if (g_opts.renderer_backend < 0 || g_opts.renderer_backend >= RENDERER_COUNT) g_opts.renderer_backend = RENDERER_OPENGL;
    g_opts.classic_audio_mask &= CLASSIC_AUDIO_ALL;
    /* Old options.txt files may request startup audio downloads.  That path moved
       to the Assets screen, so do not show legacy.json prompts at startup. */
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
#ifdef PEX_PLATFORM_PSP
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_opts.ignore_classic_resources_warning = 1;
    g_opts.download_classic_textures = 0;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
#endif
    g_selected_renderer_backend = g_opts.renderer_backend;
    if (g_opts.max_fps <= 0) g_opts.anaglyph = 0;
    if (!g_opts.username[0]) snprintf(g_opts.username, sizeof(g_opts.username), "Player");
    if (!g_opts.language[0]) snprintf(g_opts.language, sizeof(g_opts.language), "en_US");
    pex_set_language_code(g_opts.language);
}


static void save_options(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    /* Memory-only PSP mode: menu changes apply immediately but are not persisted. */
    g_opts.renderer_backend = RENDERER_OPENGL;
    g_selected_renderer_backend = RENDERER_OPENGL;
    g_opts.ignore_classic_resources_warning = 1;
    g_opts.download_classic_textures = 0;
    g_opts.download_classic_sounds = 0;
    g_opts.classic_audio_mask = 0;
    g_opts.ignore_classic_sounds_warning = 1;
    snprintf(g_opts.skin, sizeof(g_opts.skin), "%s", CLASSIC_PACK_NAME);
    return;
#endif
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "options.txt");
    FILE *f = fopen(path, "w");
    if (f) {
    fprintf(f, "music:%g\n", g_opts.music);
    fprintf(f, "sound:%g\n", g_opts.sound);
    fprintf(f, "invertYMouse:%s\n", g_opts.invert_mouse ? "true" : "false");
    fprintf(f, "mouseSensitivity:%g\n", g_opts.sensitivity);
    fprintf(f, "viewDistance:%d\n", g_opts.render_distance);
    fprintf(f, "guiScale:%d\n", g_opts.gui_scale);
    fprintf(f, "bobView:%s\n", g_opts.view_bobbing ? "true" : "false");
    fprintf(f, "anaglyph3d:%s\n", g_opts.anaglyph ? "true" : "false");
    fprintf(f, "maxFps:%d\n", g_opts.max_fps);
    {
        float fov_setting = (g_opts.fov - 70.0f) / 40.0f;
        if (fov_setting < 0.0f) fov_setting = 0.0f;
        if (fov_setting > 1.0f) fov_setting = 1.0f;
        fprintf(f, "fov:%g\n", fov_setting);
    }
    fprintf(f, "gamma:%g\n", g_opts.sf_gamma);
    fprintf(f, "sfClearWater:%s\n", g_opts.sf_clear_water ? "true" : "false");
    fprintf(f, "sfBetterSnow:%s\n", g_opts.sf_better_snow ? "true" : "false");
    fprintf(f, "sfSwampColors:%s\n", g_opts.sf_swamp_colors ? "true" : "false");
    fprintf(f, "sfSmoothBiomes:%s\n", g_opts.sf_smooth_biomes ? "true" : "false");
    fprintf(f, "sfRandomMobs:%s\n", g_opts.sf_random_mobs ? "true" : "false");
    fprintf(f, "sfMipmapLevel:%d\n", g_opts.sf_mipmap_level);
    fprintf(f, "sfMipmapLinear:%s\n", g_opts.sf_mipmap_linear ? "true" : "false");
    fprintf(f, "sfConnectedTextures:%d\n", g_opts.sf_connected_textures);
    fprintf(f, "sfNaturalTextures:%s\n", g_opts.sf_natural_textures ? "true" : "false");
    fprintf(f, "sfAaLevel:%d\n", g_opts.sf_aa_level);
    fprintf(f, "sfAfLevel:%d\n", g_opts.sf_af_level);
    fprintf(f, "difficulty:%d\n", g_opts.difficulty);
    fprintf(f, "fancyGraphics:%s\n", g_opts.fancy_graphics ? "true" : "false");
    fprintf(f, "fullscreen:%s\n", g_opts.fullscreen ? "true" : "false");
    fprintf(f, "showFps:%s\n", g_opts.show_fps ? "true" : "false");
    fprintf(f, "renderScalePercent:%d\n", g_opts.render_scale_percent);
    fprintf(f, "renderer:%s\n", renderer_backend_keys[(g_opts.renderer_backend >= 0 && g_opts.renderer_backend < RENDERER_COUNT) ? g_opts.renderer_backend : RENDERER_OPENGL]);
    fprintf(f, "ignoreClassicResourcesWarning:%s\n", g_opts.ignore_classic_resources_warning ? "true" : "false");
    fprintf(f, "downloadClassicTextures:%s\n", g_opts.download_classic_textures ? "true" : "false");
    fprintf(f, "downloadClassicSounds:%s\n", g_opts.download_classic_sounds ? "true" : "false");
    fprintf(f, "classicAudioMask:%d\n", g_opts.classic_audio_mask & CLASSIC_AUDIO_ALL);
    fprintf(f, "ignoreClassicSoundsWarning:%s\n", g_opts.ignore_classic_sounds_warning ? "true" : "false");
    fprintf(f, "skin:%s\n", g_opts.skin);
    fprintf(f, "skinPath:%s\n", g_opts.skin_path);
    fprintf(f, "lastServer:%s\n", g_opts.last_server);
    fprintf(f, "username:%s\n", g_opts.username);
    fprintf(f, "isnameset:%s\n", g_opts.name_set ? "true" : "false");
    fprintf(f, "tvRemoteMapped:%s\n", g_opts.tv_remote_mapped ? "true" : "false");
    fprintf(f, "resourcePackCacheRevision:%d\n", g_resource_pack_cache_revision);
    for (int i = 0; i < PEX_TV_REMOTE_ACTION_COUNT; ++i) fprintf(f, "tvRemote.%s:%d\n", pex_tv_remote_action_key(i), g_opts.tv_remote_map[i]);
    fprintf(f, "lang:%s\n", g_opts.language[0] ? g_opts.language : pex_current_language_code());
    for (int i = 0; i < PEX_KEY_BIND_COUNT; i++) fprintf(f, "key_%s:%d\n", key_option_names[i], vk_to_lwjgl(g_opts.keys[i]));
    fclose(f);
    }
    save_stivufine_options();
}

static float get_option_float(OptionId opt) {
    if (opt == OPT_MUSIC) return g_opts.music;
    if (opt == OPT_SOUND) return g_opts.sound;
    if (opt == OPT_SENSITIVITY) return g_opts.sensitivity;
    if (opt == OPT_RENDER_DISTANCE) {
        int c = g_opts.render_distance;
        if (c < 2) c = 2;
        if (c > 32) c = 32;
        return (float)(c - 2) / 30.0f;
    }
    if (opt == OPT_RENDER_RESOLUTION) {
        int pct = g_opts.render_scale_percent;
        if (pct < 10) pct = 10;
        if (pct > 100) pct = 100;
        return (float)(pct - 10) / 90.0f;
    }
    if (opt == OPT_FOV) {
        float fov = g_opts.fov;
        if (fov < 70.0f) fov = 70.0f;
        if (fov > 110.0f) fov = 110.0f;
        return (fov - 70.0f) / 40.0f;
    }
    if (opt == OPT_LIMIT_FRAMERATE) {
        if (g_opts.max_fps <= 0) return 1.0f;
        int fps = g_opts.max_fps;
        if (fps < 1) fps = 1;
        if (fps > MAX_FPS_CAP) fps = MAX_FPS_CAP;
        /* Keep the hard-right slider stop for Unlimited. 200 FPS sits just
           before it, so the setting reads naturally as 1..200..Unlimited. */
        float v = (float)(fps - 1) / (float)MAX_FPS_CAP;
        if (v < 0.0f) v = 0.0f;
        if (v > 0.995f) v = 0.995f;
        return v;
    }
    return 0.0f;
}

static void set_option_float(OptionId opt, float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (opt == OPT_MUSIC) g_opts.music = v;
    else if (opt == OPT_SOUND) g_opts.sound = v;
    else if (opt == OPT_SENSITIVITY) g_opts.sensitivity = v;
    else if (opt == OPT_RENDER_DISTANCE) g_opts.render_distance = 2 + (int)(v * 30.0f + 0.5f);
    else if (opt == OPT_FOV) g_opts.fov = 70.0f + v * 40.0f;
    else if (opt == OPT_RENDER_RESOLUTION) {
        int step = (int)(v * 9.0f + 0.5f);
        if (step < 0) step = 0;
        if (step > 9) step = 9;
        g_opts.render_scale_percent = 10 + step * 10;
    }
    else if (opt == OPT_LIMIT_FRAMERATE) {
        if (v >= FPS_UNLIMITED_SLIDER_VALUE) {
            g_opts.max_fps = 0; /* hard-right slider stop = truly uncapped */
            g_opts.anaglyph = 0; /* Unlimited FPS must not leave swap-interval/vsync on. */
        } else {
            g_opts.max_fps = 1 + (int)(v * (float)MAX_FPS_CAP);
            if (g_opts.max_fps < 1) g_opts.max_fps = 1;
            if (g_opts.max_fps > MAX_FPS_CAP) g_opts.max_fps = MAX_FPS_CAP;
        }
        apply_vsync_setting();
    }
    save_options();
}

static void get_option_label(OptionId opt, char *out, size_t cap) {
    const char *name = tr(opt_names[opt]);
    if (opt_is_slider[opt]) {
        float v = get_option_float(opt);
        if (opt == OPT_RENDER_DISTANCE) {
            int c = g_opts.render_distance;
            if (c < 2) c = 2;
            if (c > 32) c = 32;
            snprintf(out, cap, "%s: %d chunks", name, c);
        } else if (opt == OPT_SENSITIVITY) {
            if (v <= 0.0f) snprintf(out, cap, "%s: *yawn*", name);
            else if (v >= 1.0f) snprintf(out, cap, "%s: HYPERSPEED!!!", name);
            else snprintf(out, cap, "%s: %d%%", name, (int)(v * 200.0f));
        } else if (opt == OPT_LIMIT_FRAMERATE) {
            if (g_opts.max_fps <= 0) snprintf(out, cap, "%s: %s", name, tr("Unlimited"));
            else snprintf(out, cap, "%s: %d", name, g_opts.max_fps);
        } else if (opt == OPT_RENDER_RESOLUTION) {
            int pct = g_opts.render_scale_percent;
            if (pct < 10) pct = 10;
            if (pct > 100) pct = 100;
            int rw = (g_render_w * pct + 50) / 100;
            int rh = (g_render_h * pct + 50) / 100;
            if (rw < 16) rw = 16;
            if (rh < 16) rh = 16;
            if (pct >= 100) snprintf(out, cap, "%s: Native (%dx%d)", name, g_render_w, g_render_h);
            else if (pct <= 10) snprintf(out, cap, "%s: VHS (%dx%d)", name, rw, rh);
            else snprintf(out, cap, "%s: %d%% (%dx%d)", name, pct, rw, rh);
        } else if (opt == OPT_FOV) {
            int fov = (int)(g_opts.fov + 0.5f);
            if (fov <= 70) snprintf(out, cap, "%s: %s", name, tr_key_default("options.fov.min", "Minimum"));
            else if (fov >= 110) snprintf(out, cap, "%s: %s", name, tr_key_default("options.fov.max", "Quake Pro"));
            else snprintf(out, cap, "%s: %d", name, fov);
        } else {
            if (v <= 0.0f) snprintf(out, cap, "%s: %s", name, tr("OFF"));
            else snprintf(out, cap, "%s: %d%%", name, (int)(v * 100.0f));
        }
    } else if (opt_is_boolean[opt]) {
        int val = 0;
        if (opt == OPT_INVERT_MOUSE) val = g_opts.invert_mouse;
        else if (opt == OPT_VIEW_BOBBING) val = g_opts.view_bobbing;
        else if (opt == OPT_ANAGLYPH) val = g_opts.anaglyph;
        /* Max FPS is a slider, not boolean. */
        else if (opt == OPT_FULLSCREEN) val = g_opts.fullscreen;
        else if (opt == OPT_SHOW_FPS) val = g_opts.show_fps;
        snprintf(out, cap, "%s: %s", name, val ? tr("ON") : tr("OFF"));
    } else if (opt == OPT_RENDER_DISTANCE) {
        snprintf(out, cap, "%s: %d chunks", name, g_opts.render_distance);
    } else if (opt == OPT_DIFFICULTY) {
        snprintf(out, cap, "%s: %s", name, tr(difficulty_names[g_opts.difficulty & 3]));
    } else if (opt == OPT_GRAPHICS) {
        snprintf(out, cap, "%s: %s", name, g_opts.fancy_graphics ? tr("Fancy") : tr("Fast"));
    } else if (opt == OPT_RENDERER) {
        snprintf(out, cap, "%s: %s", name, renderer_backend_label(g_selected_renderer_backend));
    } else snprintf(out, cap, "%s: ", name);
}

static void bump_option(OptionId opt, int delta) {
    if (opt == OPT_INVERT_MOUSE) g_opts.invert_mouse = !g_opts.invert_mouse;
    else if (opt == OPT_RENDER_DISTANCE) { g_opts.render_distance += delta; if (g_opts.render_distance < 2) g_opts.render_distance = 2; if (g_opts.render_distance > 32) g_opts.render_distance = 32; }
    else if (opt == OPT_VIEW_BOBBING) g_opts.view_bobbing = !g_opts.view_bobbing;
    else if (opt == OPT_ANAGLYPH) { g_opts.anaglyph = !g_opts.anaglyph; apply_vsync_setting(); }
    else if (opt == OPT_DIFFICULTY) g_opts.difficulty = (g_opts.difficulty + delta) & 3;
    else if (opt == OPT_GRAPHICS) { g_opts.fancy_graphics = !g_opts.fancy_graphics; flat_mark_all_chunks_dirty(); }
    else if (opt == OPT_FULLSCREEN) { set_fullscreen_enabled(!g_opts.fullscreen); return; }
    else if (opt == OPT_SHOW_FPS) g_opts.show_fps = !g_opts.show_fps;
    else if (opt == OPT_RENDERER) {
#if defined(PEX_PLATFORM_SDL2) || defined(PEX_PLATFORM_PSP)
        (void)delta;
        g_selected_renderer_backend = RENDERER_OPENGL;
#else
        g_selected_renderer_backend += delta;
        if (g_selected_renderer_backend < 0) g_selected_renderer_backend = RENDERER_COUNT - 1;
        if (g_selected_renderer_backend >= RENDERER_COUNT) g_selected_renderer_backend = 0;
#endif
        return; /* restart-required setting is saved only from the confirmation screen */
    }
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
        case 29: return VK_CONTROL;
        case 16: return 'Q';
        case 18: return 'E';
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
        case VK_CONTROL: return 29;
        case 'Q': return 16;
        case 'E': return 18;
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
