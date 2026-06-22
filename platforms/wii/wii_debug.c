/* Wii/Dolphin diagnostics.
   SYS_Report goes to Dolphin's log when OSReport logging is enabled. printf goes
   to libogc's text console when the early video console is active. */

static int g_wii_debug_console_ready = 0;
static int g_wii_debug_lines = 0;
static GXRModeObj *g_wii_debug_rmode = NULL;
static void *g_wii_debug_xfb = NULL;
static char g_wii_debug_last_stage[96] = "boot";

static void wii_debug_wait_frames(int frames) {
    if (frames <= 0) return;
    for (int i = 0; i < frames; ++i) VIDEO_WaitVSync();
}

static void wii_debug_init_console(void) {
    if (g_wii_debug_console_ready) return;
    VIDEO_Init();
    g_wii_debug_rmode = VIDEO_GetPreferredMode(NULL);
    if (!g_wii_debug_rmode) {
        SYS_Report("[PexCraft/Wii] debug console: VIDEO_GetPreferredMode failed\n");
        return;
    }
    g_wii_debug_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_wii_debug_rmode));
    if (!g_wii_debug_xfb) {
        SYS_Report("[PexCraft/Wii] debug console: framebuffer allocation failed\n");
        return;
    }
    console_init(g_wii_debug_xfb, 20, 20,
                 g_wii_debug_rmode->fbWidth,
                 g_wii_debug_rmode->xfbHeight,
                 g_wii_debug_rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(g_wii_debug_rmode);
    VIDEO_SetNextFramebuffer(g_wii_debug_xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (g_wii_debug_rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    g_wii_debug_console_ready = 1;
    printf("\x1b[2;0H");
    printf("PexCraft Wii debug console\n");
    printf("Dolphin log: enable SYS_Report logging for full logs\n\n");
    SYS_Report("[PexCraft/Wii] debug console initialized fb=%dx%d xfb=%d\n",
             (int)g_wii_debug_rmode->fbWidth,
             (int)g_wii_debug_rmode->efbHeight,
             (int)g_wii_debug_rmode->xfbHeight);
}

static void wii_debug_logf(const char *fmt, ...) {
    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    SYS_Report("[PexCraft/Wii] %s\n", msg);
    if (g_wii_debug_console_ready) {
        if (g_wii_debug_lines > 22) {
            printf("\x1b[2;0H\x1b[J");
            printf("PexCraft Wii debug console\n\n");
            g_wii_debug_lines = 0;
        }
        printf("%s\n", msg);
        ++g_wii_debug_lines;
    } else {
        printf("[PexCraft/Wii] %s\n", msg);
    }
}

static void wii_debug_stagef(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_wii_debug_last_stage, sizeof(g_wii_debug_last_stage), fmt, ap);
    va_end(ap);
    wii_debug_logf("stage: %s", g_wii_debug_last_stage);
}

static void wii_debug_memoryf(const char *where) {
#if defined(HW_RVL)
    void *a1lo = SYS_GetArena1Lo();
    void *a1hi = SYS_GetArena1Hi();
    void *a2lo = SYS_GetArena2Lo();
    void *a2hi = SYS_GetArena2Hi();
    unsigned int a1 = (a1hi >= a1lo) ? (unsigned int)((uintptr_t)a1hi - (uintptr_t)a1lo) : 0;
    unsigned int a2 = (a2hi >= a2lo) ? (unsigned int)((uintptr_t)a2hi - (uintptr_t)a2lo) : 0;
    wii_debug_logf("mem %s: arena1=%p..%p free=%u arena2=%p..%p free=%u",
                   where ? where : "?", a1lo, a1hi, a1, a2lo, a2hi, a2);
#else
    wii_debug_logf("mem %s: HW_RVL not set", where ? where : "?");
#endif
}
