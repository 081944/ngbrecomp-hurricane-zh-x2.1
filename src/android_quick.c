/* The second display owns Views, never guest memory. JNI is called only by the guest thread. */
#include "quick_panel.h"
#include "hurricane_zh.h"
#include <SDL.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>

static NgbQuickPanel s_panel;
static jmethodID s_connected, s_state, s_event, s_icon;
static char s_previous[131072];
static char s_packet[131072];
static char s_sent_icons[128][24];
static int s_icon_count;
static uint64_t s_last_snapshot;

static int exception(JNIEnv *env)
{
    if (!(*env)->ExceptionCheck(env)) return 0;
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return 1;
}

static void send_icon(JNIEnv *env, jobject activity, const char *name)
{
    if (!strcmp(name, "panel_armlet.spr")) return; /* Canvas-native equipment symbol. */
    for (int i = 0; i < s_icon_count; i++)
        if (!strcmp(name, s_sent_icons[i])) return;
    if (s_icon_count == 128) return;
    int w, h, bounds[4];
    const uint32_t *pixels = ngb_quick_icon(name, &w, &h, bounds);
    if (!pixels || w < 1 || h < 1 || w > 1024 || h > 1024) return;
    jintArray array = (*env)->NewIntArray(env, w * h);
    jstring key = (*env)->NewStringUTF(env, name);
    if (array && key) {
        (*env)->SetIntArrayRegion(env, array, 0, w * h, (const jint *)pixels);
        (*env)->CallVoidMethod(env, activity, s_icon, key, array, w, h);
        if (!exception(env)) snprintf(s_sent_icons[s_icon_count++], 24, "%s", name);
    }
    if (array) (*env)->DeleteLocalRef(env, array);
    if (key) (*env)->DeleteLocalRef(env, key);
    exception(env);
}

int ngb_android_quick_update(const NgbQuickMenu *menu, int enabled, uint16_t *item, int *category)
{
    uint64_t now = SDL_GetTicks64();
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    jobject activity = env ? SDL_AndroidGetActivity() : NULL;
    if (!activity) { ngb_panel_update(&s_panel, 0, now); return 0; }
    if (!s_connected) {
        jclass cls = (*env)->GetObjectClass(env, activity);
        s_connected = (*env)->GetMethodID(env, cls, "quickPanelConnected", "()Z");
        s_state = (*env)->GetMethodID(env, cls, "setQuickPanelState", "(Ljava/lang/String;)V");
        s_event = (*env)->GetMethodID(env, cls, "pollQuickPanelEvent", "()J");
        s_icon = (*env)->GetMethodID(env, cls, "setQuickPanelIcon", "(Ljava/lang/String;[III)V");
        (*env)->DeleteLocalRef(env, cls);
    }
    int connected = 0;
    if (!exception(env) && s_connected && s_state && s_event && s_icon)
        connected = (*env)->CallBooleanMethod(env, activity, s_connected);
    if (exception(env)) connected = 0;
    ngb_panel_update(&s_panel, connected && enabled && menu, now);
    if (menu && !menu->ready) s_panel.held = 0;
    int command = 0;
    if (connected) {
        for (int i = 0; i < 32; i++) {
            uint64_t event = (uint64_t)(*env)->CallLongMethod(env, activity, s_event);
            if (exception(env) || !event) break;
            /* One resource transition per tick. Drain later taps rather than replaying them afterward. */
            if (!command) command = ngb_panel_event(&s_panel, event, menu, now, item, category);
        }
        if (!s_last_snapshot || now - s_last_snapshot >= 100 || !s_panel.active) {
            s_last_snapshot = now;
            char *state = s_packet;
            int mode = !enabled ? 0 : !menu ? 1 : menu->ready ? 2 : 3;
            static int map_asset = -1;
            static char map_name[24];
            int map_id = menu && enabled ? menu->map_id : -1;
            if (map_id >= 0) {
                if (map_asset != menu->map_asset || !*map_name) {
                    if (ngb_quick_asset_name((unsigned)menu->map_asset, map_name)) map_asset = menu->map_asset;
                }
                if (*map_name) send_icon(env, activity, map_name);
                else map_id = -1;
            }
            size_t used = (size_t)snprintf(state, sizeof s_packet, "%u\t%d\t%d\t%d\t%s\t%d\t%.5f\t%.5f\n", s_panel.epoch, mode,
                                         menu ? menu->views[0].freeze : 0, map_id, map_id >= 0 ? map_name : "-",
                                         menu && menu->map_position, menu && menu->map_position ? menu->map_x : 0,
                                         menu && menu->map_position ? menu->map_y : 0);
            if (menu && enabled) {
                for (int c = 0; c < NGB_PANEL_CATEGORIES; c++) {
                    const NgbQuickView *view = &menu->views[c];
                    for (int i = 0; i < view->count; i++) {
                        const NgbQuickEntry *e = &view->entries[i];
                        const char *zh = ngb_zh(e->name);
                        int n = snprintf(state + used, sizeof s_packet - used, "%d\t%u\t%u\t%u\t%u\t%u\t%d\t%s\t%s\n",
                                         c + 1, e->id, e->count, e->usable, e->quantity, e->level,
                                         e->equipped, e->art, zh ? zh : e->name);
                        if (n < 0 || (size_t)n >= sizeof s_packet - used) { state[used] = 0; goto full; }
                        used += (size_t)n;
                        send_icon(env, activity, e->art);
                    }
                }
            }
full:
            if (strcmp(state, s_previous)) {
                int entries = 0;
                if (menu) for (int c = 0; c < NGB_PANEL_CATEGORIES; c++) entries += menu->views[c].count;
                fprintf(stderr, "[PANEL] snapshot mode=%d entries=%d epoch=%u\n", mode, entries, s_panel.epoch);
                jstring value = (*env)->NewStringUTF(env, state);
                if (value) {
                    (*env)->CallVoidMethod(env, activity, s_state, value);
                    if (!exception(env)) snprintf(s_previous, sizeof s_previous, "%s", state);
                    (*env)->DeleteLocalRef(env, value);
                }
            }
        }
    } else {
        s_previous[0] = 0;
        s_last_snapshot = 0;
        s_icon_count = 0;
    }
    int result = (command ? 2 : 0) | (ngb_panel_freeze(&s_panel, menu) ? 1 : 0);
    (*env)->DeleteLocalRef(env, activity);
    return result;
}
