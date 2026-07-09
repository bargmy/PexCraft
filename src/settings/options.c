/* Split from original monolithic main.c. Included by src/main.c unity build. */

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static void hptibine_set_defaults(void);
static float get_option_float(OptionId opt);
static void set_option_float(OptionId opt, float v);

static void set_default_options(void) {
    g_opts.music = 1.0f;
    g_opts.sound = 1.0f;
    g_opts.sensitivity = 0.5f;
    g_opts.invert_mouse = 0;
    g_opts.render_distance = 8;
    g_opts.view_bobbing = 1;
    g_opts.anaglyph = 0; /* V-Sync */
    g_opts.max_fps = 0;  /* unlimited */
    g_opts.fov = 70.0f;
    g_opts.difficulty = 2;
    g_opts.fancy_graphics = 1;
    g_opts.fullscreen = 0;
    g_opts.show_fps = 0;
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
    for (int i = 0; i < 10; i++) g_opts.keys[i] = default_keys[i];
    hptibine_set_defaults();
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


typedef enum HptiBineCategory {
    HPTI_CAT_MAIN = 0,
    HPTI_CAT_DETAILS,
    HPTI_CAT_QUALITY,
    HPTI_CAT_ANIMATIONS,
    HPTI_CAT_PERFORMANCE,
    HPTI_CAT_OTHER
} HptiBineCategory;

typedef enum HptiBineOptionKind {
    HPTI_KIND_BUTTON = 0,
    HPTI_KIND_SLIDER = 1
} HptiBineOptionKind;

typedef struct HptiBineOptionDef {
    HptiBineOptionId id;
    const char *label;
    HptiBineCategory category;
    HptiBineOptionKind kind;
    int implemented;
} HptiBineOptionDef;

static const HptiBineOptionDef hptibine_defs[] = {
    { HPTI_GRAPHICS, "Graphics", HPTI_CAT_MAIN, HPTI_KIND_BUTTON, 1 },
    { HPTI_RENDER_DISTANCE_FINE, "Render Distance", HPTI_CAT_MAIN, HPTI_KIND_SLIDER, 1 },
    { HPTI_AO_LEVEL, "Smooth Lighting", HPTI_CAT_MAIN, HPTI_KIND_SLIDER, 1 },
    { HPTI_FRAMERATE_LIMIT, "Max FPS", HPTI_CAT_MAIN, HPTI_KIND_SLIDER, 1 },
    { HPTI_ANAGLYPH, "3D Anaglyph", HPTI_CAT_MAIN, HPTI_KIND_BUTTON, 0 },
    { HPTI_VIEW_BOBBING, "View Bobbing", HPTI_CAT_MAIN, HPTI_KIND_BUTTON, 1 },
    { HPTI_GUI_SCALE, "GUI Scale", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 },
    { HPTI_ADVANCED_OPENGL, "Advanced OpenGL", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 0 },
    { HPTI_GAMMA, "Brightness", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_RENDER_CLOUDS, "Render Clouds", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_FOG_FANCY, "Fog", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },
    { HPTI_FOG_START, "Fog Start", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },

    { HPTI_CLOUDS, "Clouds", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_CLOUD_HEIGHT, "Cloud Height", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },
    { HPTI_TREES, "Trees", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_GRASS, "Grass", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_WATER, "Water", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },
    { HPTI_RAIN, "Rain & Snow", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },
    { HPTI_SKY, "Sky", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_STARS, "Stars", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_SUN_MOON, "Sun & Moon", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 1 },
    { HPTI_SHOW_CAPES, "Show Capes", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },
    { HPTI_DEPTH_FOG, "Depth Fog", HPTI_CAT_DETAILS, HPTI_KIND_BUTTON, 0 },

    { HPTI_MIPMAP_LEVEL, "Mipmap Level", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_MIPMAP_TYPE, "Mipmap Type", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_CLEAR_WATER, "Clear Water", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_RANDOM_MOBS, "Random Mobs", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_BETTER_GRASS, "Better Grass", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_BETTER_SNOW, "Better Snow", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_CUSTOM_FONTS, "Custom Fonts", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_CUSTOM_COLORS, "Custom Colors", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_SWAMP_COLORS, "Swamp Colors", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_SMOOTH_BIOMES, "Smooth Biomes", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_CONNECTED_TEXTURES, "Connected Textures", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_NATURAL_TEXTURES, "Natural Textures", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_AA_LEVEL, "Antialiasing", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },
    { HPTI_AF_LEVEL, "Anisotropic Filtering", HPTI_CAT_QUALITY, HPTI_KIND_BUTTON, 0 },

    { HPTI_ANIMATED_WATER, "Water Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_ANIMATED_LAVA, "Lava Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_ANIMATED_FIRE, "Fire Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_ANIMATED_PORTAL, "Portal Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_ANIMATED_REDSTONE, "Redstone Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_ANIMATED_EXPLOSION, "Explosion Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_ANIMATED_FLAME, "Flame Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_ANIMATED_SMOKE, "Smoke Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_VOID_PARTICLES, "Void Particles", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_WATER_PARTICLES, "Water Particles", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_RAIN_SPLASH, "Rain Splash", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_PORTAL_PARTICLES, "Portal Particles", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_PARTICLES, "Particles", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_DRIPPING_WATER_LAVA, "Dripping Water/Lava", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_ANIMATED_TERRAIN, "Terrain Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },
    { HPTI_ANIMATED_ITEMS, "Items Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 0 },
    { HPTI_ANIMATED_TEXTURES, "Textures Animated", HPTI_CAT_ANIMATIONS, HPTI_KIND_BUTTON, 1 },

    { HPTI_SMOOTH_FPS, "Smooth FPS", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 0 },
    { HPTI_SMOOTH_INPUT, "Smooth Input", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 0 },
    { HPTI_LOAD_FAR, "Load Far", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 0 },
    { HPTI_PRELOADED_CHUNKS, "Preloaded Chunks", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 0 },
    { HPTI_CHUNK_UPDATES, "Chunk Updates", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 1 },
    { HPTI_CHUNK_UPDATES_DYNAMIC, "Dynamic Updates", HPTI_CAT_PERFORMANCE, HPTI_KIND_BUTTON, 1 },

    { HPTI_FAST_DEBUG_INFO, "Fast Debug Info", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 },
    { HPTI_PROFILER, "Debug Profiler", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 },
    { HPTI_WEATHER, "Weather", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 },
    { HPTI_TIME, "Time", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 },
    { HPTI_FULLSCREEN_MODE, "Fullscreen", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 },
    { HPTI_AUTOSAVE_TICKS, "Autosave", HPTI_CAT_OTHER, HPTI_KIND_BUTTON, 0 }
};

static const HptiBineOptionDef *hptibine_find_def(HptiBineOptionId id) {
    for (int i = 0; i < (int)ARRAY_COUNT(hptibine_defs); ++i) if (hptibine_defs[i].id == id) return &hptibine_defs[i];
    return NULL;
}

static int hptibine_screen_category(ScreenId screen) {
    if (screen == SCREEN_HPTIBINE) return HPTI_CAT_MAIN;
    if (screen == SCREEN_HPTIBINE_DETAILS) return HPTI_CAT_DETAILS;
    if (screen == SCREEN_HPTIBINE_QUALITY) return HPTI_CAT_QUALITY;
    if (screen == SCREEN_HPTIBINE_ANIMATIONS) return HPTI_CAT_ANIMATIONS;
    if (screen == SCREEN_HPTIBINE_PERFORMANCE) return HPTI_CAT_PERFORMANCE;
    if (screen == SCREEN_HPTIBINE_OTHER) return HPTI_CAT_OTHER;
    return -1;
}

static const char *hptibine_screen_title(ScreenId screen) {
    if (screen == SCREEN_HPTIBINE_DETAILS) return "HptiBine Detail Settings";
    if (screen == SCREEN_HPTIBINE_QUALITY) return "HptiBine Quality Settings";
    if (screen == SCREEN_HPTIBINE_ANIMATIONS) return "HptiBine Animation Settings";
    if (screen == SCREEN_HPTIBINE_PERFORMANCE) return "HptiBine Performance Settings";
    if (screen == SCREEN_HPTIBINE_OTHER) return "HptiBine Other Settings";
    return "HptiBine";
}

static int hptibine_option_count_for_screen(ScreenId screen) {
    int cat = hptibine_screen_category(screen);
    int count = 0;
    if (cat < 0) return 0;
    for (int i = 0; i < (int)ARRAY_COUNT(hptibine_defs); ++i) if ((int)hptibine_defs[i].category == cat) count++;
    return count;
}

static HptiBineOptionId hptibine_option_at(ScreenId screen, int index) {
    int cat = hptibine_screen_category(screen);
    int count = 0;
    if (cat < 0) return HPTI_GRAPHICS;
    for (int i = 0; i < (int)ARRAY_COUNT(hptibine_defs); ++i) {
        if ((int)hptibine_defs[i].category != cat) continue;
        if (count == index) return hptibine_defs[i].id;
        count++;
    }
    return HPTI_GRAPHICS;
}

static int hptibine_option_enabled(HptiBineOptionId id) {
    const HptiBineOptionDef *d = hptibine_find_def(id);
    return d ? d->implemented : 0;
}

static int hptibine_option_is_slider(HptiBineOptionId id) {
    const HptiBineOptionDef *d = hptibine_find_def(id);
    return d && d->implemented && d->kind == HPTI_KIND_SLIDER;
}

static const char *hptibine_mode_default_fast_fancy_off(int v) {
    if (v == HPTI_FAST) return "Fast";
    if (v == HPTI_FANCY) return "Fancy";
    if (v == HPTI_OFF) return "OFF";
    return "Default";
}

static const char *hptibine_mode_default_fast_fancy(int v) {
    if (v == HPTI_FAST) return "Fast";
    if (v == HPTI_FANCY) return "Fancy";
    return "Default";
}

static const char *hptibine_on_off(int v) { return v ? "ON" : "OFF"; }

static const char *hptibine_anim_mode_label(int v) {
    if (v == HPTI_ANIM_DYNAMIC) return "Dynamic";
    if (v == HPTI_ANIM_OFF) return "OFF";
    return "ON";
}

static int hptibine_parse_bool(const char *v) {
    return v && (!strcmp(v, "true") || !strcmp(v, "1") || !strcmp(v, "yes") || !strcmp(v, "on") || !strcmp(v, "ON"));
}

static void hptibine_set_defaults(void) {
    g_opts.hpti_fog_type = 1;
    g_opts.hpti_fog_start = 0.8f;
    g_opts.hpti_ao_level = 1.0f;
    g_opts.hpti_clouds = HPTI_DEFAULT;
    g_opts.hpti_cloud_height = 0.0f;
    g_opts.hpti_trees = HPTI_DEFAULT;
    g_opts.hpti_grass = HPTI_DEFAULT;
    g_opts.hpti_water = HPTI_DEFAULT;
    g_opts.hpti_rain = HPTI_DEFAULT;
    g_opts.hpti_sky = 1;
    g_opts.hpti_stars = 1;
    g_opts.hpti_sun_moon = 1;
    g_opts.hpti_depth_fog = 1;
    g_opts.hpti_animated_water = HPTI_ANIM_ON;
    g_opts.hpti_animated_lava = HPTI_ANIM_ON;
    g_opts.hpti_animated_fire = 1;
    g_opts.hpti_animated_portal = 1;
    g_opts.hpti_animated_redstone = 1;
    g_opts.hpti_animated_explosion = 1;
    g_opts.hpti_animated_flame = 1;
    g_opts.hpti_animated_smoke = 1;
    g_opts.hpti_void_particles = 1;
    g_opts.hpti_water_particles = 1;
    g_opts.hpti_rain_splash = 1;
    g_opts.hpti_portal_particles = 1;
    g_opts.hpti_dripping_water_lava = 1;
    g_opts.hpti_animated_terrain = 1;
    g_opts.hpti_animated_items = 1;
    g_opts.hpti_animated_textures = 1;
    g_opts.hpti_particles = 0;
    g_opts.hpti_chunk_updates = 1;
    g_opts.hpti_chunk_updates_dynamic = 0;
    g_opts.hpti_fast_debug_info = 0;
    g_opts.hpti_profiler = 0;
    g_opts.hpti_weather = 1;
}

static void hptibine_clamp_options(void) {
    if (g_opts.hpti_fog_type < 1 || g_opts.hpti_fog_type > 3) g_opts.hpti_fog_type = 1;
    if (g_opts.hpti_fog_start < 0.2f) g_opts.hpti_fog_start = 0.2f;
    if (g_opts.hpti_fog_start > 0.8f) g_opts.hpti_fog_start = 0.8f;
    if (g_opts.hpti_ao_level < 0.0f) g_opts.hpti_ao_level = 0.0f;
    if (g_opts.hpti_ao_level > 1.0f) g_opts.hpti_ao_level = 1.0f;
    if (g_opts.hpti_clouds < 0 || g_opts.hpti_clouds > 3) g_opts.hpti_clouds = 0;
    if (g_opts.hpti_cloud_height < 0.0f) g_opts.hpti_cloud_height = 0.0f;
    if (g_opts.hpti_cloud_height > 1.0f) g_opts.hpti_cloud_height = 1.0f;
    if (g_opts.hpti_trees < 0 || g_opts.hpti_trees > 2) g_opts.hpti_trees = 0;
    if (g_opts.hpti_grass < 0 || g_opts.hpti_grass > 2) g_opts.hpti_grass = 0;
    if (g_opts.hpti_water < 0 || g_opts.hpti_water > 2) g_opts.hpti_water = 0;
    if (g_opts.hpti_rain < 0 || g_opts.hpti_rain > 3) g_opts.hpti_rain = 0;
    if (g_opts.hpti_animated_water < 0 || g_opts.hpti_animated_water > 2) g_opts.hpti_animated_water = 0;
    if (g_opts.hpti_animated_lava < 0 || g_opts.hpti_animated_lava > 2) g_opts.hpti_animated_lava = 0;
    if (g_opts.hpti_particles < 0 || g_opts.hpti_particles > 2) g_opts.hpti_particles = 0;
    if (g_opts.hpti_chunk_updates < 1) g_opts.hpti_chunk_updates = 1;
    if (g_opts.hpti_chunk_updates > 5) g_opts.hpti_chunk_updates = 5;
}

static void hptibine_load_file_named(const char *name) {
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
        if (!strcmp(k, "ofFogType")) g_opts.hpti_fog_type = atoi(v);
        else if (!strcmp(k, "ofFogStart")) g_opts.hpti_fog_start = parse_float_java(v);
        else if (!strcmp(k, "ofAoLevel")) g_opts.hpti_ao_level = parse_float_java(v);
        else if (!strcmp(k, "ofClouds")) g_opts.hpti_clouds = atoi(v);
        else if (!strcmp(k, "ofCloudsHeight")) g_opts.hpti_cloud_height = parse_float_java(v);
        else if (!strcmp(k, "ofTrees")) g_opts.hpti_trees = atoi(v);
        else if (!strcmp(k, "ofGrass")) g_opts.hpti_grass = atoi(v);
        else if (!strcmp(k, "ofWater")) g_opts.hpti_water = atoi(v);
        else if (!strcmp(k, "ofRain")) g_opts.hpti_rain = atoi(v);
        else if (!strcmp(k, "ofSky")) g_opts.hpti_sky = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofStars")) g_opts.hpti_stars = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofSunMoon")) g_opts.hpti_sun_moon = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofDepthFog")) g_opts.hpti_depth_fog = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedWater")) g_opts.hpti_animated_water = atoi(v);
        else if (!strcmp(k, "ofAnimatedLava")) g_opts.hpti_animated_lava = atoi(v);
        else if (!strcmp(k, "ofAnimatedFire")) g_opts.hpti_animated_fire = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedPortal")) g_opts.hpti_animated_portal = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedRedstone")) g_opts.hpti_animated_redstone = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedExplosion")) g_opts.hpti_animated_explosion = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedFlame")) g_opts.hpti_animated_flame = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedSmoke")) g_opts.hpti_animated_smoke = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofVoidParticles")) g_opts.hpti_void_particles = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofWaterParticles")) g_opts.hpti_water_particles = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofRainSplash")) g_opts.hpti_rain_splash = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofPortalParticles")) g_opts.hpti_portal_particles = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofDrippingWaterLava")) g_opts.hpti_dripping_water_lava = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedTerrain")) g_opts.hpti_animated_terrain = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedItems")) g_opts.hpti_animated_items = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofAnimatedTextures")) g_opts.hpti_animated_textures = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofChunkUpdates")) g_opts.hpti_chunk_updates = atoi(v);
        else if (!strcmp(k, "ofChunkUpdatesDynamic")) g_opts.hpti_chunk_updates_dynamic = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofFastDebugInfo")) g_opts.hpti_fast_debug_info = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofProfiler")) g_opts.hpti_profiler = hptibine_parse_bool(v);
        else if (!strcmp(k, "ofWeather")) g_opts.hpti_weather = hptibine_parse_bool(v);
        else if (!strcmp(k, "particles") || !strcmp(k, "particleSetting")) g_opts.hpti_particles = atoi(v);
    }
    fclose(f);
    hptibine_clamp_options();
    }
#endif
}

static void load_hptibine_options(void) {
    hptibine_load_file_named("optionsof.txt");
    hptibine_load_file_named("optionshptibine.txt");
}

static void save_hptibine_options(void) {
#if defined(PEX_PLATFORM_PSP) && defined(PEX_PSP_MEMORY_ONLY) && PEX_PSP_MEMORY_ONLY
    return;
#else
    hptibine_clamp_options();
    char path[MAX_PATHBUF];
    path_join(path, sizeof(path), g_mc_dir, "optionshptibine.txt");
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# HptiBine settings (OptiFine_1.2.5_HD_C6-compatible keys)\n");
    fprintf(f, "ofFogType:%d\n", g_opts.hpti_fog_type);
    fprintf(f, "ofFogStart:%g\n", g_opts.hpti_fog_start);
    fprintf(f, "ofAoLevel:%g\n", g_opts.hpti_ao_level);
    fprintf(f, "ofClouds:%d\n", g_opts.hpti_clouds);
    fprintf(f, "ofCloudsHeight:%g\n", g_opts.hpti_cloud_height);
    fprintf(f, "ofTrees:%d\n", g_opts.hpti_trees);
    fprintf(f, "ofGrass:%d\n", g_opts.hpti_grass);
    fprintf(f, "ofWater:%d\n", g_opts.hpti_water);
    fprintf(f, "ofRain:%d\n", g_opts.hpti_rain);
    fprintf(f, "ofSky:%s\n", g_opts.hpti_sky ? "true" : "false");
    fprintf(f, "ofStars:%s\n", g_opts.hpti_stars ? "true" : "false");
    fprintf(f, "ofSunMoon:%s\n", g_opts.hpti_sun_moon ? "true" : "false");
    fprintf(f, "ofDepthFog:%s\n", g_opts.hpti_depth_fog ? "true" : "false");
    fprintf(f, "ofAnimatedWater:%d\n", g_opts.hpti_animated_water);
    fprintf(f, "ofAnimatedLava:%d\n", g_opts.hpti_animated_lava);
    fprintf(f, "ofAnimatedFire:%s\n", g_opts.hpti_animated_fire ? "true" : "false");
    fprintf(f, "ofAnimatedPortal:%s\n", g_opts.hpti_animated_portal ? "true" : "false");
    fprintf(f, "ofAnimatedRedstone:%s\n", g_opts.hpti_animated_redstone ? "true" : "false");
    fprintf(f, "ofAnimatedExplosion:%s\n", g_opts.hpti_animated_explosion ? "true" : "false");
    fprintf(f, "ofAnimatedFlame:%s\n", g_opts.hpti_animated_flame ? "true" : "false");
    fprintf(f, "ofAnimatedSmoke:%s\n", g_opts.hpti_animated_smoke ? "true" : "false");
    fprintf(f, "ofVoidParticles:%s\n", g_opts.hpti_void_particles ? "true" : "false");
    fprintf(f, "ofWaterParticles:%s\n", g_opts.hpti_water_particles ? "true" : "false");
    fprintf(f, "ofRainSplash:%s\n", g_opts.hpti_rain_splash ? "true" : "false");
    fprintf(f, "ofPortalParticles:%s\n", g_opts.hpti_portal_particles ? "true" : "false");
    fprintf(f, "ofDrippingWaterLava:%s\n", g_opts.hpti_dripping_water_lava ? "true" : "false");
    fprintf(f, "ofAnimatedTerrain:%s\n", g_opts.hpti_animated_terrain ? "true" : "false");
    fprintf(f, "ofAnimatedItems:%s\n", g_opts.hpti_animated_items ? "true" : "false");
    fprintf(f, "ofAnimatedTextures:%s\n", g_opts.hpti_animated_textures ? "true" : "false");
    fprintf(f, "particles:%d\n", g_opts.hpti_particles);
    fprintf(f, "ofChunkUpdates:%d\n", g_opts.hpti_chunk_updates);
    fprintf(f, "ofChunkUpdatesDynamic:%s\n", g_opts.hpti_chunk_updates_dynamic ? "true" : "false");
    fprintf(f, "ofFastDebugInfo:%s\n", g_opts.hpti_fast_debug_info ? "true" : "false");
    fprintf(f, "ofProfiler:%s\n", g_opts.hpti_profiler ? "true" : "false");
    fprintf(f, "ofWeather:%s\n", g_opts.hpti_weather ? "true" : "false");
    fclose(f);
#endif
}

static int hptibine_cloud_mode(void) {
    if (g_opts.hpti_clouds == HPTI_FAST || g_opts.hpti_clouds == HPTI_FANCY || g_opts.hpti_clouds == HPTI_OFF) return g_opts.hpti_clouds;
    return g_opts.fancy_graphics ? HPTI_FANCY : HPTI_FAST;
}

static int hptibine_fancy_trees_enabled(void) {
    if (g_opts.hpti_trees == HPTI_FAST) return 0;
    if (g_opts.hpti_trees == HPTI_FANCY) return 1;
    return g_opts.fancy_graphics;
}

static int hptibine_fancy_grass_enabled(void) {
    if (g_opts.hpti_grass == HPTI_FAST) return 0;
    if (g_opts.hpti_grass == HPTI_FANCY) return 1;
    return g_opts.fancy_graphics;
}

static int hptibine_smooth_lighting_enabled(void) {
    return g_opts.hpti_ao_level > 0.0001f;
}

static int hptibine_textures_animated(void) {
    return g_opts.hpti_animated_textures && g_opts.hpti_animated_terrain;
}

static int hptibine_animate_water_texture(void) {
    return hptibine_textures_animated() && g_opts.hpti_animated_water != HPTI_ANIM_OFF;
}

static int hptibine_animate_lava_texture(void) {
    return hptibine_textures_animated() && g_opts.hpti_animated_lava != HPTI_ANIM_OFF;
}

static int hptibine_animate_portal_texture(void) {
    return hptibine_textures_animated() && g_opts.hpti_animated_portal;
}

static int hptibine_particle_allowed(void) {
    if (g_opts.hpti_particles <= 0) return 1;
    if (g_opts.hpti_particles == 1) return (rand() & 1) == 0;
    return (rand() & 3) == 0;
}

static int hptibine_water_particles_enabled(void) { return g_opts.hpti_water_particles && hptibine_particle_allowed(); }
static int hptibine_portal_particles_enabled(void) { return g_opts.hpti_portal_particles && hptibine_particle_allowed(); }
static int hptibine_chunk_updates(void) { hptibine_clamp_options(); return g_opts.hpti_chunk_updates; }
static int hptibine_dynamic_chunk_updates_enabled(void) { return g_opts.hpti_chunk_updates_dynamic; }

static float get_hptibine_option_float(HptiBineOptionId opt) {
    if (opt == HPTI_RENDER_DISTANCE_FINE) {
        int c = g_opts.render_distance;
        if (c < 2) c = 2;
        if (c > 32) c = 32;
        return (float)(c - 2) / 30.0f;
    }
    if (opt == HPTI_AO_LEVEL) return g_opts.hpti_ao_level;
    if (opt == HPTI_FRAMERATE_LIMIT) return get_option_float(OPT_LIMIT_FRAMERATE);
    return 0.0f;
}

static void set_hptibine_option_float(HptiBineOptionId opt, float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    if (opt == HPTI_RENDER_DISTANCE_FINE) set_option_float(OPT_RENDER_DISTANCE, v);
    else if (opt == HPTI_AO_LEVEL) {
        g_opts.hpti_ao_level = v;
        flat_mark_all_chunks_dirty();
        save_options();
    } else if (opt == HPTI_FRAMERATE_LIMIT) set_option_float(OPT_LIMIT_FRAMERATE, v);
}

static void hptibine_render_distance_label(char *out, size_t cap) {
    int blocks = g_opts.render_distance * 16;
    const char *name = "Tiny";
    int base = 32;
    if (blocks >= 64) { name = "Short"; base = 64; }
    if (blocks >= 128) { name = "Normal"; base = 128; }
    if (blocks >= 256) { name = "Far"; base = 256; }
    if (blocks >= 512) { name = "Extreme"; base = 512; }
    int diff = blocks - base;
    if (diff == 0) snprintf(out, cap, "Render Distance: %s", name);
    else snprintf(out, cap, "Render Distance: %s +%d", name, diff);
}

static void get_hptibine_option_label(HptiBineOptionId opt, char *out, size_t cap) {
    const HptiBineOptionDef *d = hptibine_find_def(opt);
    const char *name = d ? d->label : "HptiBine";
    if (!d || !d->implemented) { snprintf(out, cap, "%s: N/A", name); return; }
    switch (opt) {
        case HPTI_GRAPHICS: snprintf(out, cap, "%s: %s", name, g_opts.fancy_graphics ? "Fancy" : "Fast"); break;
        case HPTI_RENDER_DISTANCE_FINE: hptibine_render_distance_label(out, cap); break;
        case HPTI_AO_LEVEL:
            if (g_opts.hpti_ao_level <= 0.0f) snprintf(out, cap, "%s: OFF", name);
            else snprintf(out, cap, "%s: %d%%", name, (int)(g_opts.hpti_ao_level * 100.0f + 0.5f));
            break;
        case HPTI_FRAMERATE_LIMIT:
            if (g_opts.max_fps <= 0) snprintf(out, cap, "%s: Unlimited", name);
            else snprintf(out, cap, "%s: %d", name, g_opts.max_fps);
            break;
        case HPTI_ANAGLYPH: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.anaglyph)); break;
        case HPTI_VIEW_BOBBING: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.view_bobbing)); break;
        case HPTI_RENDER_CLOUDS: snprintf(out, cap, "%s: %s", name, hptibine_cloud_mode() == HPTI_OFF ? "OFF" : "ON"); break;
        case HPTI_CLOUDS: snprintf(out, cap, "%s: %s", name, hptibine_mode_default_fast_fancy_off(g_opts.hpti_clouds)); break;
        case HPTI_TREES: snprintf(out, cap, "%s: %s", name, hptibine_mode_default_fast_fancy(g_opts.hpti_trees)); break;
        case HPTI_GRASS: snprintf(out, cap, "%s: %s", name, hptibine_mode_default_fast_fancy(g_opts.hpti_grass)); break;
        case HPTI_SKY: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_sky)); break;
        case HPTI_STARS: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_stars)); break;
        case HPTI_SUN_MOON: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_sun_moon)); break;
        case HPTI_ANIMATED_WATER: snprintf(out, cap, "%s: %s", name, hptibine_anim_mode_label(g_opts.hpti_animated_water)); break;
        case HPTI_ANIMATED_LAVA: snprintf(out, cap, "%s: %s", name, hptibine_anim_mode_label(g_opts.hpti_animated_lava)); break;
        case HPTI_ANIMATED_PORTAL: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_animated_portal)); break;
        case HPTI_WATER_PARTICLES: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_water_particles)); break;
        case HPTI_PORTAL_PARTICLES: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_portal_particles)); break;
        case HPTI_PARTICLES:
            snprintf(out, cap, "%s: %s", name, g_opts.hpti_particles == 2 ? "Minimal" : (g_opts.hpti_particles == 1 ? "Decreased" : "All"));
            break;
        case HPTI_ANIMATED_TERRAIN: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_animated_terrain)); break;
        case HPTI_ANIMATED_TEXTURES: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_animated_textures)); break;
        case HPTI_CHUNK_UPDATES: snprintf(out, cap, "%s: %d", name, g_opts.hpti_chunk_updates); break;
        case HPTI_CHUNK_UPDATES_DYNAMIC: snprintf(out, cap, "%s: %s", name, hptibine_on_off(g_opts.hpti_chunk_updates_dynamic)); break;
        default: snprintf(out, cap, "%s: N/A", name); break;
    }
}

static void bump_hptibine_option(HptiBineOptionId opt, int delta) {
    (void)delta;
    if (!hptibine_option_enabled(opt)) return;
    switch (opt) {
        case HPTI_GRAPHICS:
            g_opts.fancy_graphics = !g_opts.fancy_graphics;
            flat_mark_all_chunks_dirty();
            break;
        case HPTI_ANAGLYPH:
            g_opts.anaglyph = !g_opts.anaglyph;
            apply_vsync_setting();
            break;
        case HPTI_VIEW_BOBBING:
            g_opts.view_bobbing = !g_opts.view_bobbing;
            break;
        case HPTI_RENDER_CLOUDS:
            g_opts.hpti_clouds = (hptibine_cloud_mode() == HPTI_OFF) ? HPTI_DEFAULT : HPTI_OFF;
            break;
        case HPTI_CLOUDS:
            g_opts.hpti_clouds = (g_opts.hpti_clouds + 1) & 3;
            break;
        case HPTI_TREES:
            g_opts.hpti_trees++;
            if (g_opts.hpti_trees > 2) g_opts.hpti_trees = 0;
            flat_mark_all_chunks_dirty();
            break;
        case HPTI_GRASS:
            g_opts.hpti_grass++;
            if (g_opts.hpti_grass > 2) g_opts.hpti_grass = 0;
            flat_mark_all_chunks_dirty();
            break;
        case HPTI_SKY: g_opts.hpti_sky = !g_opts.hpti_sky; break;
        case HPTI_STARS: g_opts.hpti_stars = !g_opts.hpti_stars; break;
        case HPTI_SUN_MOON: g_opts.hpti_sun_moon = !g_opts.hpti_sun_moon; break;
        case HPTI_ANIMATED_WATER:
            g_opts.hpti_animated_water++;
            if (g_opts.hpti_animated_water > 2) g_opts.hpti_animated_water = 0;
            break;
        case HPTI_ANIMATED_LAVA:
            g_opts.hpti_animated_lava++;
            if (g_opts.hpti_animated_lava > 2) g_opts.hpti_animated_lava = 0;
            break;
        case HPTI_ANIMATED_PORTAL: g_opts.hpti_animated_portal = !g_opts.hpti_animated_portal; break;
        case HPTI_WATER_PARTICLES: g_opts.hpti_water_particles = !g_opts.hpti_water_particles; break;
        case HPTI_PORTAL_PARTICLES: g_opts.hpti_portal_particles = !g_opts.hpti_portal_particles; break;
        case HPTI_PARTICLES:
            g_opts.hpti_particles = (g_opts.hpti_particles + 1) % 3;
            break;
        case HPTI_ANIMATED_TERRAIN: g_opts.hpti_animated_terrain = !g_opts.hpti_animated_terrain; break;
        case HPTI_ANIMATED_TEXTURES: g_opts.hpti_animated_textures = !g_opts.hpti_animated_textures; break;
        case HPTI_CHUNK_UPDATES:
            g_opts.hpti_chunk_updates++;
            if (g_opts.hpti_chunk_updates > 5) g_opts.hpti_chunk_updates = 1;
            break;
        case HPTI_CHUNK_UPDATES_DYNAMIC:
            g_opts.hpti_chunk_updates_dynamic = !g_opts.hpti_chunk_updates_dynamic;
            break;
        default: break;
    }
    hptibine_clamp_options();
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
        else if (!strcmp(k, "bobView")) g_opts.view_bobbing = !strcmp(v, "true");
        else if (!strcmp(k, "anaglyph3d")) g_opts.anaglyph = !strcmp(v, "true");
        else if (!strcmp(k, "limitFramerate")) { if (!strcmp(v, "true")) g_opts.max_fps = 60; }
        else if (!strcmp(k, "maxFps")) g_opts.max_fps = atoi(v);
        else if (!strcmp(k, "fov") || !strcmp(k, "fieldOfView")) g_opts.fov = parse_float_java(v);
        else if (!strcmp(k, "difficulty")) g_opts.difficulty = atoi(v) & 3;
        else if (!strcmp(k, "fancyGraphics")) g_opts.fancy_graphics = !strcmp(v, "true");
        else if (!strcmp(k, "fullscreen") || !strcmp(k, "enableFullscreen")) g_opts.fullscreen = !strcmp(v, "true");
        else if (!strcmp(k, "showFps") || !strcmp(k, "showFPSCounter")) g_opts.show_fps = !strcmp(v, "true");
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
            for (int i = 0; i < 10; i++) {
                const char *orig[10] = {"key.forward","key.left","key.back","key.right","key.jump","key.sneak","key.drop","key.inventory","key.chat","key.fog"};
                if (!strcmp(name, orig[i])) g_opts.keys[i] = lwjgl_to_vk(atoi(v));
            }
        }
    }
    fclose(f);
    }
    load_hptibine_options();
    if (g_opts.render_distance < 2) g_opts.render_distance = 8;
    if (g_opts.render_distance > 32) g_opts.render_distance = 32;
    if (g_opts.max_fps < 0) g_opts.max_fps = 0;
    if (g_opts.max_fps > MAX_FPS_CAP) g_opts.max_fps = MAX_FPS_CAP;
    if (g_opts.fov <= 1.0f) g_opts.fov = 70.0f + g_opts.fov * 40.0f;
    if (g_opts.fov < 70.0f) g_opts.fov = 70.0f;
    if (g_opts.fov > 110.0f) g_opts.fov = 110.0f;
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
    fprintf(f, "bobView:%s\n", g_opts.view_bobbing ? "true" : "false");
    fprintf(f, "anaglyph3d:%s\n", g_opts.anaglyph ? "true" : "false");
    fprintf(f, "maxFps:%d\n", g_opts.max_fps);
    {
        float fov_setting = (g_opts.fov - 70.0f) / 40.0f;
        if (fov_setting < 0.0f) fov_setting = 0.0f;
        if (fov_setting > 1.0f) fov_setting = 1.0f;
        fprintf(f, "fov:%g\n", fov_setting);
    }
    fprintf(f, "difficulty:%d\n", g_opts.difficulty);
    fprintf(f, "fancyGraphics:%s\n", g_opts.fancy_graphics ? "true" : "false");
    fprintf(f, "fullscreen:%s\n", g_opts.fullscreen ? "true" : "false");
    fprintf(f, "showFps:%s\n", g_opts.show_fps ? "true" : "false");
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
    const char *orig[10] = {"key.forward","key.left","key.back","key.right","key.jump","key.sneak","key.drop","key.inventory","key.chat","key.fog"};
    for (int i = 0; i < 10; i++) fprintf(f, "key_%s:%d\n", orig[i], vk_to_lwjgl(g_opts.keys[i]));
    fclose(f);
    }
    save_hptibine_options();
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
