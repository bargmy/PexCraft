/* Android Minecraft Classic texture-pack installer.
   Java/Android performs the HTTPS download with HttpURLConnection, then the
   native game extracts client.jar using the existing internal ZIP extractor. */

#ifndef CLASSIC_INSTALL_IDLE
#define CLASSIC_INSTALL_IDLE 0
#define CLASSIC_INSTALL_DOWNLOADING 1
#define CLASSIC_INSTALL_EXTRACTING 2
#define CLASSIC_INSTALL_DONE 3
#define CLASSIC_INSTALL_ERROR 4
#endif
#ifndef CLASSIC_SIZE_UNKNOWN
#define CLASSIC_SIZE_UNKNOWN 0
#define CLASSIC_SIZE_FETCHING 1
#define CLASSIC_SIZE_READY 2
#define CLASSIC_SIZE_ERROR 3
#endif

static volatile LONG g_classic_install_state = CLASSIC_INSTALL_IDLE;
static volatile LONG g_classic_install_progress = 0;
static HANDLE g_classic_install_thread = NULL;
static char g_classic_install_status[MAX_LABEL] = "";
static char g_classic_install_error[MAX_LABEL] = "";
static char g_classic_downloaded_jar_path[MAX_PATHBUF] = "";
static volatile LONG g_classic_download_size_state = CLASSIC_SIZE_UNKNOWN;
static volatile LONG g_classic_download_size_bytes = 0;

static void pack_install_set_state(LONG state, LONG progress, const char *status) {
    if (status) lstrcpynA(g_classic_install_status, status, sizeof(g_classic_install_status));
    InterlockedExchange(&g_classic_install_progress, progress);
    InterlockedExchange(&g_classic_install_state, state);
}

static void pack_install_fail(const char *msg) {
    lstrcpynA(g_classic_install_error, msg ? msg : "Unknown error", sizeof(g_classic_install_error));
    pack_install_set_state(CLASSIC_INSTALL_ERROR, 0, "Failed");
    log_msg("Minecraft Classic texture pack install failed: %s", g_classic_install_error);
}

static void android_clear_jni_exception(JNIEnv *env) {
    if (env && (*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
}

static int android_call_bool_string_string(const char *method, const char *a, const char *b) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return 0;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, method, "(Ljava/lang/String;Ljava/lang/String;)Z") : NULL;
    if (!mid) {
        android_clear_jni_exception(env);
        if (cls) (*env)->DeleteLocalRef(env, cls);
        (*env)->DeleteLocalRef(env, activity);
        return 0;
    }
    jstring ja = (*env)->NewStringUTF(env, a ? a : "");
    jstring jb = (*env)->NewStringUTF(env, b ? b : "");
    jboolean ok = (*env)->CallBooleanMethod(env, activity, mid, ja, jb);
    android_clear_jni_exception(env);
    if (jb) (*env)->DeleteLocalRef(env, jb);
    if (ja) (*env)->DeleteLocalRef(env, ja);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return ok ? 1 : 0;
}

static void android_call_void_string(const char *method, const char *a) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, method, "(Ljava/lang/String;)V") : NULL;
    if (mid) {
        jstring ja = (*env)->NewStringUTF(env, a ? a : "");
        (*env)->CallVoidMethod(env, activity, mid, ja);
        android_clear_jni_exception(env);
        if (ja) (*env)->DeleteLocalRef(env, ja);
    } else {
        android_clear_jni_exception(env);
    }
    if (cls) (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
}

static void android_call_void_noargs(const char *method) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, method, "()V") : NULL;
    if (mid) {
        (*env)->CallVoidMethod(env, activity, mid);
        android_clear_jni_exception(env);
    } else {
        android_clear_jni_exception(env);
    }
    if (cls) (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
}

static int android_call_int(const char *method) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return 0;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, method, "()I") : NULL;
    jint v = 0;
    if (mid) v = (*env)->CallIntMethod(env, activity, mid);
    android_clear_jni_exception(env);
    if (cls) (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
    return (int)v;
}

static void android_call_string_copy(const char *method, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = 0;
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = SDL_AndroidGetActivity();
    if (!env || !activity) return;
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID mid = cls ? (*env)->GetMethodID(env, cls, method, "()Ljava/lang/String;") : NULL;
    if (mid) {
        jstring js = (jstring)(*env)->CallObjectMethod(env, activity, mid);
        if (js) {
            const char *utf = (*env)->GetStringUTFChars(env, js, NULL);
            if (utf) {
                snprintf(out, cap, "%s", utf);
                (*env)->ReleaseStringUTFChars(env, js, utf);
            }
            (*env)->DeleteLocalRef(env, js);
        }
    }
    android_clear_jni_exception(env);
    if (cls) (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, activity);
}

static void pack_install_start_size_fetch(void) {
    LONG old = InterlockedCompareExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING, CLASSIC_SIZE_UNKNOWN);
    if (old != CLASSIC_SIZE_UNKNOWN) return;
    android_call_void_string("startClassicPackSizeFetch", CLASSIC_PACK_URL);
}

static void format_download_size(char *out, size_t cap) {
    int state = android_call_int("getClassicPackSizeState");
    int bytes = android_call_int("getClassicPackSizeBytes");
    if (state == CLASSIC_SIZE_READY && bytes > 0) {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_READY);
        InterlockedExchange(&g_classic_download_size_bytes, (LONG)bytes);
        snprintf(out, cap, "Download size: %.2f MB (%d bytes)", (double)bytes / (1024.0 * 1024.0), bytes);
    } else if (state == CLASSIC_SIZE_FETCHING) {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_FETCHING);
        snprintf(out, cap, "Download size: checking server...");
    } else {
        InterlockedExchange(&g_classic_download_size_state, CLASSIC_SIZE_ERROR);
        snprintf(out, cap, "Download size: unavailable");
    }
}

static DWORD WINAPI classic_extract_worker(LPVOID unused) {
    (void)unused;
    char pack_dir[MAX_PATHBUF];
    char err[MAX_LABEL];
    err[0] = 0;

    ensure_dir(g_texpack_dir);
    pack_asset_path(pack_dir, sizeof(pack_dir));
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");

    if (!pxc_extract_zip_file(g_classic_downloaded_jar_path, pack_dir, err, sizeof(err))) {
        pack_install_fail(err[0] ? err : "Could not extract client.jar internally");
        return 0;
    }
    if (!pack_is_installed() || pack_missing_required_textures()) {
        pack_install_fail("Extracted pack is missing required textures");
        return 0;
    }

    DeleteFileA(g_classic_downloaded_jar_path);
    pack_install_set_state(CLASSIC_INSTALL_DONE, 100, "Done!");
    log_msg("Installed Minecraft Classic texture pack at %s", pack_dir);
    return 0;
}

static void classic_start_extract_thread(void) {
    if (g_classic_install_thread) return;
    pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Extracting textures...");
    g_classic_install_thread = CreateThread(NULL, 0, classic_extract_worker, NULL, 0, NULL);
    if (!g_classic_install_thread) pack_install_fail("Could not start extractor thread");
}

static void pack_install_request_cancel(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    android_call_void_noargs("cancelClassicPackDownload");
    if (g_classic_install_thread && state == CLASSIC_INSTALL_EXTRACTING) {
        /* ZIP extraction cannot be interrupted safely; keep the progress screen until it finishes. */
        pack_install_set_state(CLASSIC_INSTALL_EXTRACTING, 90, "Finishing extraction...");
        return;
    }
    if (g_classic_install_thread) {
        WaitForSingleObject(g_classic_install_thread, 0);
        CloseHandle(g_classic_install_thread);
        g_classic_install_thread = NULL;
    }
    DeleteFileA(g_classic_downloaded_jar_path);
    g_classic_install_error[0] = 0;
    pack_install_set_state(CLASSIC_INSTALL_IDLE, 0, "Canceled");
    set_screen(SCREEN_CLASSIC_PACK_DOWNLOAD_PROMPT);
}

static void pack_install_start(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, CLASSIC_INSTALL_DOWNLOADING, CLASSIC_INSTALL_IDLE);
    if (state == CLASSIC_INSTALL_DOWNLOADING || state == CLASSIC_INSTALL_EXTRACTING) {
        set_screen(SCREEN_TEXPACK_INSTALL);
        return;
    }

    if (g_classic_install_thread) {
        WaitForSingleObject(g_classic_install_thread, 0);
        CloseHandle(g_classic_install_thread);
        g_classic_install_thread = NULL;
    }

    g_classic_install_error[0] = 0;
    snprintf(g_classic_downloaded_jar_path, sizeof(g_classic_downloaded_jar_path), "%s/minecraft_classic_client.jar", g_mc_dir);
    DeleteFileA(g_classic_downloaded_jar_path);
    pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, 0, "Downloading client.jar...");
    set_screen(SCREEN_TEXPACK_INSTALL);

    if (!android_call_bool_string_string("startClassicPackDownload", CLASSIC_PACK_URL, g_classic_downloaded_jar_path)) {
        pack_install_fail("Could not start Android downloader");
    }
}

static void pack_install_tick(void) {
    LONG state = InterlockedCompareExchange(&g_classic_install_state, 0, 0);
    if (state == CLASSIC_INSTALL_DOWNLOADING) {
        int jstate = android_call_int("getClassicPackDownloadState");
        int prog = android_call_int("getClassicPackDownloadProgress");
        char status[MAX_LABEL];
        status[0] = 0;
        android_call_string_copy("getClassicPackDownloadStatus", status, sizeof(status));
        if (prog < 0) prog = 0;
        if (prog > 89) prog = 89;
        if (status[0]) pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, prog, status);
        else pack_install_set_state(CLASSIC_INSTALL_DOWNLOADING, prog, "Downloading client.jar...");

        if (jstate == CLASSIC_INSTALL_DONE) {
            classic_start_extract_thread();
        } else if (jstate == CLASSIC_INSTALL_ERROR) {
            char err[MAX_LABEL];
            err[0] = 0;
            android_call_string_copy("getClassicPackDownloadError", err, sizeof(err));
            pack_install_fail(err[0] ? err : "Android download failed");
        }
    } else if (state == CLASSIC_INSTALL_DONE) {
        if (g_classic_install_thread) {
            WaitForSingleObject(g_classic_install_thread, 0);
            CloseHandle(g_classic_install_thread);
            g_classic_install_thread = NULL;
        }
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        scan_texture_packs();
        for (int i = 0; i < g_texpack_count; i++) {
            if (g_texpacks[i].is_builtin_classic) {
                apply_texture_pack_index(i);
                break;
            }
        }
        set_screen(SCREEN_TEXPACK);
    } else if (state == CLASSIC_INSTALL_ERROR) {
        if (g_classic_install_thread) {
            WaitForSingleObject(g_classic_install_thread, 0);
            CloseHandle(g_classic_install_thread);
            g_classic_install_thread = NULL;
        }
        InterlockedExchange(&g_classic_install_state, CLASSIC_INSTALL_IDLE);
        open_notice("Texture Pack", "Could not install Minecraft Classic.", g_classic_install_error);
    }
}
