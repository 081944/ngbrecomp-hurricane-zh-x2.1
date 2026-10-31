/*
 * Android: which Vulkan driver the game draws with. The phone's own, or Turnip -- Mesa's open driver for Qualcomm's
 * Adreno GPUs, built from Mesa's source and shipped in the APK (packaging/android/build.sh) -- loaded through
 * libadrenotools, which has the system's Vulkan loader load it for this app alone. Many Adreno 6xx phones' own
 * drivers stop at Vulkan 1.1 without the dynamic rendering, synchronization2 and push descriptors the renderer
 * needs (#87); Turnip has them.
 *
 * The setting (Graphics > GPU Driver, settings.ini gpu_driver; NGB_GPU_DRIVER=system|turnip|package over it):
 * System; Turnip; Package (one of the driver zips in the app's files folder); or Auto, Turnip only on
 * an Adreno whose own driver lacks those -- found by a look at the system's driver first, an
 * instance made and let go. Turnip finding no GPU it knows (an Adreno too new or too old for it) falls back to the
 * system's. The GPU emulation asks here before it starts Vulkan (xboxrecomp's instance.c, nv2a_vk_custom_loader).
 */
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <adrenotools/driver.h>

int ngb_pc_setting(int which);   /* src/ingame_menu.c: 35 GPU Driver */

#define TURNIP_LIB "libvulkan_freedreno.so"

/* The folder this library was loaded from: the app's native libraries (libadrenotools' hooks and Turnip) */
static int native_dir(char *out, size_t n)
{
    Dl_info info;
    if (!dladdr((void *)native_dir, &info) || !info.dli_fname)
        return 0;
    snprintf(out, n, "%s", info.dli_fname);
    char *slash = strrchr(out, '/');
    if (!slash)
        return 0;
    slash[1] = 0;
    return 1;
}

/* A device the renderer can use through gipa: 1 one it can; 0 none (or no instance or device at all: *made 0);
 * and *qualcomm when the first is an Adreno. The extensions listed and their features on (as instance.c turns
 * them on): a driver may list them and not have them. */
static int usable_device(PFN_vkGetInstanceProcAddr gipa, int *qualcomm, int *made)
{
    if (made)
        *made = 0;
    PFN_vkCreateInstance create = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    if (!create)
        return 0;
    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_1 };
    VkInstanceCreateInfo ci = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app };
    VkInstance inst;
    VkResult res = create(&ci, NULL, &inst);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "[GPU] its vkCreateInstance failed (%d)\n", res);
        return 0;
    }
    PFN_vkDestroyInstance destroy = (PFN_vkDestroyInstance)gipa(inst, "vkDestroyInstance");
    PFN_vkEnumeratePhysicalDevices devices =
        (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
    PFN_vkGetPhysicalDeviceProperties props =
        (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
    PFN_vkEnumerateDeviceExtensionProperties exts =
        (PFN_vkEnumerateDeviceExtensionProperties)gipa(inst, "vkEnumerateDeviceExtensionProperties");
    PFN_vkGetPhysicalDeviceFeatures2 features =
        (PFN_vkGetPhysicalDeviceFeatures2)gipa(inst, "vkGetPhysicalDeviceFeatures2");
    int ok = 0;
    uint32_t n = 1;
    VkPhysicalDevice dev;
    if (devices && props && exts && devices(inst, &n, &dev) >= 0 && n >= 1) {
        VkPhysicalDeviceProperties p;
        props(dev, &p);
        if (made)
            *made = 1;
        if (qualcomm)
            *qualcomm = p.vendorID == 0x5143;
        uint32_t k = 0;
        exts(dev, NULL, &k, NULL);
        VkExtensionProperties *e = calloc(k ? k : 1, sizeof *e);
        int dyn = p.apiVersion >= VK_API_VERSION_1_3, sync2 = dyn, push = 0;
        if (e && exts(dev, NULL, &k, e) >= 0)
            for (uint32_t i = 0; i < k; i++) {
                dyn |= !strcmp(e[i].extensionName, "VK_KHR_dynamic_rendering");
                sync2 |= !strcmp(e[i].extensionName, "VK_KHR_synchronization2");
                push |= !strcmp(e[i].extensionName, "VK_KHR_push_descriptor");
            }
        free(e);
        if (dyn && sync2 && features) {   /* (on, not only listed) */
            VkPhysicalDeviceDynamicRenderingFeaturesKHR d = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
            VkPhysicalDeviceSynchronization2FeaturesKHR s = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR, .pNext = &d };
            VkPhysicalDeviceFeatures2 f2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &s };
            features(dev, &f2);
            dyn = d.dynamicRendering;
            sync2 = s.synchronization2;
        }
        ok = dyn && sync2 && push;
        fprintf(stderr, "[GPU] %s: Vulkan %u.%u, %s\n", p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
                VK_API_VERSION_MINOR(p.apiVersion), ok ? "has what the renderer needs" : "lacks what the renderer needs");
    } else {
        fprintf(stderr, "[GPU] it lists no GPU here (%u found: built for another Adreno?)\n", n);
    }
    if (destroy)
        destroy(inst, NULL);
    return ok;
}

/* ---------- driver packages (the emulators' zip kind: meta.json + the .so) ----------
 *
 * The zips the player copies into the app's own files folder's drivers/ (Android/data/io.github.gamboozino.ngbrecomp/files/drivers)
 * are unpacked at the start (below, through NgbActivity.java's syncDrivers) into
 * files/drivers/<zip's name>/, listed in Graphics > Driver Package, loaded when GPU Driver is Package. The
 * meta.json is the emulators' common one (name, vendor, libraryName...).
 */
#include <dirent.h>
#include <sys/stat.h>
#include <jni.h>
#include <SDL.h>
#include <SDL_system.h>

int ngb_driver_store_dir(char *out, size_t n)
{
    const char *base = SDL_AndroidGetInternalStoragePath();
    if (!base || !*base)
        return 0;
    snprintf(out, n, "%s/drivers", base);
    mkdir(out, 0755);
    return 1;
}

/* A field's string out of a package's meta.json ("name": "x"), 1 when it is there */
static int pkg_field(const char *dir, const char *field, char *out, size_t n)
{
    char path[1200];
    snprintf(path, sizeof path, "%s/meta.json", dir);
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    char text[4096];
    size_t got = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[got] = 0;
    char key[80];
    snprintf(key, sizeof key, "\"%s\"", field);
    const char *k = strstr(text, key);
    if (!k)
        return 0;
    const char *v = strchr(k + strlen(key), '"');
    if (!v)
        return 0;
    v++;
    size_t i = 0;
    while (v[i] && v[i] != '"' && i + 1 < n) {
        out[i] = v[i];
        i++;
    }
    out[i] = 0;
    return out[0] != 0;
}

/* The i-th package (alphabetical): its directory and shown name; 0 when there is no such one */
int ngb_driver_pkg(int i, char *dir, size_t dn, char *name, size_t nn)
{
    char store[1024];
    if (!ngb_driver_store_dir(store, sizeof store))
        return 0;
    DIR *d = opendir(store);
    if (!d)
        return 0;
    char names[32][128];
    int found = 0;
    for (struct dirent *e; (e = readdir(d)) && found < 32; ) {
        if (e->d_name[0] == '.')
            continue;
        char p[1200];
        struct stat st;
        snprintf(p, sizeof p, "%s/%s", store, e->d_name);
        if (stat(p, &st) != 0 || !S_ISDIR(st.st_mode))
            continue;
        snprintf(names[found], sizeof names[found], "%s", e->d_name);
        found++;
    }
    closedir(d);
    if (i < 0 || i >= found)
        return 0;
    for (int a = 1; a < found; a++)              /* (small lists: an insertion sort) */
        for (int b = a; b > 0 && strcmp(names[b - 1], names[b]) > 0; b--) {
            char t[128];
            snprintf(t, sizeof t, "%s", names[b - 1]);
            snprintf(names[b - 1], sizeof names[b - 1], "%s", names[b]);
            snprintf(names[b], sizeof names[b], "%s", t);
        }
    snprintf(dir, dn, "%s/%s", store, names[i]);
    if (name && nn)
        snprintf(name, nn, "%s", names[i]);
    return 1;
}

int ngb_driver_pkg_count(void)
{
    int n = 0;
    while (ngb_driver_pkg(n, NULL, 0, NULL, 0))
        n++;
    return n;
}

void ngb_driver_pkg_select(const char *name)
{
    char store[1024], path[1150];
    if (!ngb_driver_store_dir(store, sizeof store))
        return;
    snprintf(path, sizeof path, "%s/.selected", store);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n", name);
        fclose(f);
    }
}

int ngb_driver_pkg_selected(char *out, size_t n)
{
    char store[1024], path[1150];
    if (!ngb_driver_store_dir(store, sizeof store))
        return 0;
    snprintf(path, sizeof path, "%s/.selected", store);
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    if (!fgets(out, (int)n, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    char *nl = strpbrk(out, "\r\n");
    if (nl)
        *nl = 0;
    return out[0] != 0;
}

/* The player's driver zips (the app's files folder) unpacked into the store, at the start (NgbActivity's
 * syncDrivers); nothing when it did not go */
void ngb_android_sync_drivers(void)
{
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    jobject act = SDL_AndroidGetActivity();
    if (!env || !act) {
        if (act)
            (*env)->DeleteLocalRef(env, act);
        return;
    }
    jclass cls = (*env)->GetObjectClass(env, act);
    jmethodID sync = (*env)->GetMethodID(env, cls, "syncDrivers", "()V");
    if (sync) {
        (*env)->CallVoidMethod(env, act, sync);
        if ((*env)->ExceptionCheck(env))
            (*env)->ExceptionClear(env);
    }
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, act);
}

/* One file copied over another (0 on failure) */
static int file_copy(const char *from, const char *to)
{
    FILE *in = fopen(from, "rb");
    if (!in)
        return 0;
    FILE *out = fopen(to, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    chmod(to, 0644);
    return 1;
}

/* A driver's dependencies the app's linker namespace cannot see (libcutils and its kind: system-private
 * libraries, off limits to apps since Android 7). The device's own copy, world-readable under /system,
 * /system_ext, /vendor or /odm, is put beside the driver -- each round of opening names the next one missing. */
static void vendor_missing_libs(const char *dir, const char *lib)
{
    char path[1300];
    snprintf(path, sizeof path, "%s/%s", dir, lib);
    char done[8][128];
    int done_n = 0;
    for (int round = 0; round < 8; round++) {
        void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (h)
            dlclose(h);
        const char *e = dlerror();
        if (!e)
            return;
        const char *a = strstr(e, "library \"");
        if (!a)
            return;
        a += 9;
        const char *b = strchr(a, '"');
        if (!b || b == a || (size_t)(b - a) >= 128)
            return;
        char name[128], from[256];
        memcpy(name, a, (size_t)(b - a));
        name[b - a] = 0;
        int seen = 0;
        for (int i = 0; i < done_n; i++)
            seen |= !strcmp(done[i], name);
        if (seen)
            return;   /* (the same name over: no more of the chain is learnable) */
        static const char *const where[] = {
            "/system/lib64/", "/system_ext/lib64/", "/vendor/lib64/", "/odm/lib64/",
        };
        int got = 0;
        for (unsigned i = 0; i < sizeof where / sizeof *where && !got; i++) {
            snprintf(from, sizeof from, "%s%s", where[i], name);
            char to[1300];
            snprintf(to, sizeof to, "%s/%s", dir, name);
            got = file_copy(from, to);
        }
        fprintf(stderr, "[GPU] %s: its %s is not for apps here: the device's own %s\n",
                lib, name, got ? "copied in" : "could not be found either");
        if (done_n < 8)
            snprintf(done[done_n++], sizeof done[0], "%s", name);
    }
}

/* The folder the driver-loading namespace searches: the hook libraries, the driver and everything
 * vendored in, gathered in one writable place (the APK's own lib folder cannot be written to). */
static void prepare_hook_dir(char *out, size_t n, const char *pkgdir)
{
    const char *base = SDL_AndroidGetInternalStoragePath();
    out[0] = 0;
    if (!base || !*base)
        return;
    snprintf(out, n, "%s/hooks", base);
    mkdir(out, 0755);
    char from[1300], to[1400], nd[1024];
    if (native_dir(nd, sizeof nd)) {
        static const char *const hooklibs[] = { "libhook_impl.so", "libmain_hook.so" };
        for (unsigned i = 0; i < sizeof hooklibs / sizeof *hooklibs; i++) {
            snprintf(from, sizeof from, "%s%s", nd, hooklibs[i]);
            snprintf(to, sizeof to, "%s/%s", out, hooklibs[i]);
            file_copy(from, to);
        }
    }
    DIR *d = opendir(pkgdir);
    if (!d)
        return;
    for (struct dirent *e; (e = readdir(d)); ) {
        size_t l = strlen(e->d_name);
        if (l < 3 || strcmp(e->d_name + l - 3, ".so") != 0)
            continue;
        snprintf(from, sizeof from, "%s/%s", pkgdir, e->d_name);
        snprintf(to, sizeof to, "%s/%s", out, e->d_name);
        file_copy(from, to);
    }
    closedir(d);
}

PFN_vkGetInstanceProcAddr nv2a_vk_custom_loader(void)
{
    int mode = ngb_pc_setting(35);   /* 0 Auto, 1 System, 2 Turnip, 3 Package */
    const char *env = getenv("NGB_GPU_DRIVER");
    if (env && !strcmp(env, "system"))
        mode = 1;
    else if (env && !strcmp(env, "turnip"))
        mode = 2;
    else if (env && !strcmp(env, "package"))
        mode = 3;
    if (mode == 1)
        return NULL;
    if (mode == 0) {   /* Auto: the system's driver first */
        void *sys = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        PFN_vkGetInstanceProcAddr gipa = sys ? (PFN_vkGetInstanceProcAddr)dlsym(sys, "vkGetInstanceProcAddr") : NULL;
        int qualcomm = 0, made = 0, ok = gipa ? usable_device(gipa, &qualcomm, &made) : 0;
        /* (no device from the system's at all: an Adreno's broken driver as likely as not -- Turnip tried, it
         * falls back by itself; an Adreno has its kernel driver's node) */
        if (!made)
            qualcomm = access("/dev/kgsl-3d0", F_OK) == 0;
        if (ok || !qualcomm) {
            fprintf(stderr, "[GPU] the system's driver%s\n", ok ? "" : " (not an Adreno: Turnip cannot help)");
            return NULL;
        }
    }
    char hooks[1024];
    if (!native_dir(hooks, sizeof hooks))
        return NULL;
    char pkgdir[1200] = "", lib[256] = TURNIP_LIB;
    if (mode == 3) {   /* Package: the imported one chosen (Graphics > Driver Package) */
        char sel[128];
        if (!ngb_driver_pkg_selected(sel, sizeof sel)) {
            fprintf(stderr, "[GPU] no driver package chosen: the system's driver\n");
            return NULL;
        }
        char store[1024];
        if (!ngb_driver_store_dir(store, sizeof store))
            return NULL;
        snprintf(pkgdir, sizeof pkgdir, "%s/%s", store, sel);
        if (!pkg_field(pkgdir, "libraryName", lib, sizeof lib)) {
            fprintf(stderr, "[GPU] the chosen package has no libraryName: the system's driver\n");
            return NULL;
        }
    }
    PFN_vkGetInstanceProcAddr gipa = NULL;
    void *vk = NULL;
    char hookdir[1024];
    if (*pkgdir) {   /* the player's own: its private dependencies are vendored in, everything gathered
                      * into a folder the namespace searches, then it is opened plainly -- a driver the
                      * ordinary linker can open needs no hooks */
        vendor_missing_libs(pkgdir, lib);
        prepare_hook_dir(hookdir, sizeof hookdir, pkgdir);
        char pkgpath[1300];
        snprintf(pkgpath, sizeof pkgpath, "%s/%s", pkgdir, lib);
        vk = dlopen(pkgpath, RTLD_NOW | RTLD_LOCAL);
        if (vk)
            gipa = (PFN_vkGetInstanceProcAddr)dlsym(vk, "vkGetInstanceProcAddr");
        if (gipa && usable_device(gipa, NULL, NULL)) {
            fprintf(stderr, "[GPU] package: %s\n", lib);
            return gipa;
        }
        if (gipa) {
            fprintf(stderr, "[GPU] %s would not serve this GPU plainly (the hooks' way is tried)\n", lib);
            dlclose(vk);
        } else {
            const char *e = dlerror();
            fprintf(stderr, "[GPU] %s would not open plainly: %s (the hooks' way is tried)\n",
                    lib, e ? e : "no detail");
        }
    } else {
        snprintf(hookdir, sizeof hookdir, "%s", hooks);
    }
    char drvdir[1200];   /* (adrenotools joins this and the name itself: the slash has to be there) */
    if (*pkgdir)
        snprintf(drvdir, sizeof drvdir, "%s/", pkgdir);
    else
        snprintf(drvdir, sizeof drvdir, "%s", hooks);
    vk = adrenotools_open_libvulkan(RTLD_NOW, ADRENOTOOLS_DRIVER_CUSTOM, NULL, hookdir,
                                    drvdir, lib, NULL, NULL);
    if (!vk) {
        const char *e = dlerror();
        fprintf(stderr, "[GPU] the chosen driver (%s) would not open: %s\n", lib, e ? e : "no detail");
        return NULL;
    }
    gipa = (PFN_vkGetInstanceProcAddr)dlsym(vk, "vkGetInstanceProcAddr");
    if (!gipa) {
        const char *e = dlerror();
        fprintf(stderr, "[GPU] %s: no vkGetInstanceProcAddr in it: %s\n", lib, e ? e : "no detail");
        return NULL;
    }
    if (!usable_device(gipa, NULL, NULL)) {
        fprintf(stderr, "[GPU] the chosen driver (%s) could not be used here: the system's driver\n", lib);
        return NULL;
    }
    fprintf(stderr, "[GPU] %s%s\n", *pkgdir ? "package: " : "Turnip (Mesa's driver for Adreno)",
            *pkgdir ? lib : "");
    return gipa;
}
