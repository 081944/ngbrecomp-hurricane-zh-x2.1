/*
 * The first start on Android: the player's disc image found on the phone and installed (src/install.c,
 * ngb_find_game), as the desktop's file picker does. The app asks once for access to the shared storage
 * (NgbActivity.java), then looks in a NinjaGaidenBlack folder, in Download and at the top of the storage for an
 * Xbox disc image (.iso, .xiso) or a folder of its files (with default.xbe); with more than one, the player
 * chooses. An archive (.7z, .zip) has to be unpacked first: there is no unpacker on the phone. The install shows
 * its progress in the system's small notes (toasts).
 */
#include <dirent.h>
#include <errno.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_system.h>

#include "install.h"

#define MAX_FOUND 3

/* NgbActivity's method by name (no arguments): its boolean, or for a void one 1 when it could be called */
static int activity_call(const char *name, const char *sig)
{
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    jobject act = SDL_AndroidGetActivity();
    if (!env || !act)
        return 0;
    jclass cls = (*env)->GetObjectClass(env, act);
    jmethodID m = (*env)->GetMethodID(env, cls, name, sig);
    int r = 0;
    if (m) {
        if (sig[2] == 'Z')
            r = (*env)->CallBooleanMethod(env, act, m);
        else {
            (*env)->CallVoidMethod(env, act, m);
            r = 1;
        }
    }
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        r = 0;
    }
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, act);
    return r;
}

/* 0 continue, 1 the second button... (buttons in order, the first the default) */
static int ask(const char *title, const char *text, const char **buttons, int n)
{
    SDL_MessageBoxButtonData b[5];
    for (int i = 0; i < n && i < 5; i++) {
        b[i].flags = i == 0 ? SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT : 0;
        b[i].buttonid = i;
        b[i].text = buttons[i];
    }
    SDL_MessageBoxData d = { SDL_MESSAGEBOX_INFORMATION, NULL, title, text, n, b, NULL };
    int hit = -1;
    if (SDL_ShowMessageBox(&d, &hit) != 0)
        return -1;
    return hit;
}

static int storage_access(void)
{
    while (!activity_call("haveStorageAccess", "()Z")) {
        static const char *b[] = { "Allow", "Quit" };
        if (ask("Ninja Gaiden Black",
                "The game's files are not on this phone yet. To install them from your own disc image, the app "
                "needs to read it where you put it: allow \"All files access\" on the next screen, then come "
                "back here.", b, 2) != 0)
            return 0;
        activity_call("askStorageAccess", "()V");
        SDL_Delay(1500);   /* (the system's page opens over the game) */
        static const char *c[] = { "Continue", "Quit" };
        if (!activity_call("haveStorageAccess", "()Z") &&
            ask("Ninja Gaiden Black", "Press Continue once access is allowed.", c, 2) != 0)
            return 0;
    }
    return 1;
}

static int is_image(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".xiso"));
}

static int has_xbe(const char *dir)
{
    char p[1100];
    snprintf(p, sizeof p, "%s/default.xbe", dir);
    return access(p, R_OK) == 0;
}

/* Images and game folders in dir (and its folders one deep, when deep) into found */
static void look(const char *dir, int deep, char found[][1024], int *n)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d)
        return;
    if (has_xbe(dir) && *n < MAX_FOUND)
        snprintf(found[(*n)++], 1024, "%s", dir);
    while (*n < MAX_FOUND && (e = readdir(d))) {
        char p[1024];
        struct stat st;
        if (e->d_name[0] == '.')
            continue;
        snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
        if (stat(p, &st) != 0)
            continue;
        if (S_ISREG(st.st_mode) && is_image(e->d_name) && st.st_size > (off_t)1 << 30)
            snprintf(found[(*n)++], 1024, "%s", p);
        else if (S_ISDIR(st.st_mode) && deep && has_xbe(p))
            snprintf(found[(*n)++], 1024, "%s", p);
    }
    closedir(d);
}

/* The disc image or game folder to install from, into out: 1, or 0 when the player quit */
int ngb_android_pick_disc(char *out, size_t n)
{
    if (!storage_access())
        return 0;
    const char *root = getenv("EXTERNAL_STORAGE");
    if (!root || !*root)
        root = "/storage/emulated/0";
    for (;;) {
        char found[MAX_FOUND][1024], dir[1024];
        int k = 0;
        snprintf(dir, sizeof dir, "%s/NinjaGaidenBlack", root);
        look(dir, 1, found, &k);
        snprintf(dir, sizeof dir, "%s/Download", root);
        look(dir, 1, found, &k);
        look(root, 0, found, &k);
        if (k == 1) {
            snprintf(out, n, "%s", found[0]);
            return 1;
        }
        if (k > 1) {
            const char *b[MAX_FOUND + 1];
            for (int i = 0; i < k; i++) {
                const char *slash = strrchr(found[i], '/');
                b[i] = slash ? slash + 1 : found[i];
            }
            b[k] = "Quit";
            int hit = ask("Ninja Gaiden Black", "Install the game from which of these?", b, k + 1);
            if (hit < 0 || hit >= k)
                return 0;
            snprintf(out, n, "%s", found[hit]);
            return 1;
        }
        static const char *b[] = { "Look again", "Quit" };
        if (ask("Ninja Gaiden Black",
                "No disc image found. Copy your Ninja Gaiden Black disc image (.iso, from the retail USA disc) "
                "into Download or into a NinjaGaidenBlack folder on the phone, then press Look again. A .7z or "
                ".zip has to be unpacked first; a folder of the disc's files (with default.xbe) also works.",
                b, 2) != 0)
            return 0;
    }
}

static void toast_progress(uint64_t done, uint64_t total, void *ud)
{
    int *last = ud;
    if (!total)
        return;
    int pct = (int)(done * 100 / total);
    if (pct / 10 != *last / 10) {   /* (a note every tenth: each stays a couple of seconds) */
        char msg[96];
        snprintf(msg, sizeof msg, "Installing Ninja Gaiden Black: %d%%", pct);
        SDL_AndroidShowToast(msg, 0, -1, 0, 0);
        *last = pct;
    }
}

/* ngb_install with its progress in toasts, and its failure said: 0 when installed */
int ngb_android_install(const char *src, const char *installed, char *out, size_t n)
{
    int last = -10;
    SDL_AndroidShowToast("Installing Ninja Gaiden Black: a few minutes", 1, -1, 0, 0);
    ngb_install_set_progress(toast_progress, &last);
    int rc = ngb_install(src, installed, out, n);
    ngb_install_set_progress(NULL, NULL);
    if (rc != 0) {
        char msg[1400];
        snprintf(msg, sizeof msg, "The game could not be installed.\n\n%s", ngb_install_error());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Ninja Gaiden Black", msg, NULL);
    } else {
        SDL_AndroidShowToast("Installed. You can delete the disc image now if you like.", 1, -1, 0, 0);
    }
    return rc;
}
