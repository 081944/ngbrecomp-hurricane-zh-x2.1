// One selected Vulkan loader for preflight, volk and SDL. Keep it alive until
// its owning process exits: driver callbacks may outlive a destroyed instance.
#include "android_vulkan.h"
#include <adrenotools/driver.h>
#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <cstdlib>
#include <string>

static PFN_vkGetInstanceProcAddr selected_proc;
static std::string selected_key;

extern "C" PFN_vkGetInstanceProcAddr ngb_android_vulkan_proc(void) { return selected_proc; }

extern "C" JNIEXPORT jstring JNICALL
Java_org_ngb_port_DriverStore_configureNative(JNIEnv *env, jclass, jstring hooks,
    jstring directory, jstring name, jstring temporary)
{
    const char *h = env->GetStringUTFChars(hooks, nullptr);
    if (!h) return nullptr; // A pending JNI allocation exception propagates.
    const char *d = env->GetStringUTFChars(directory, nullptr);
    if (!d) { env->ReleaseStringUTFChars(hooks, h); return nullptr; }
    const char *n = env->GetStringUTFChars(name, nullptr);
    if (!n) { env->ReleaseStringUTFChars(directory, d); env->ReleaseStringUTFChars(hooks, h); return nullptr; }
    const char *t = env->GetStringUTFChars(temporary, nullptr);
    if (!t) {
        env->ReleaseStringUTFChars(name, n); env->ReleaseStringUTFChars(directory, d);
        env->ReleaseStringUTFChars(hooks, h); return nullptr;
    }
    std::string key = std::string(d) + "/" + n;
    std::string error;
    if (key != selected_key || !selected_proc) {
        void *handle = *n ? adrenotools_open_libvulkan(RTLD_NOW | RTLD_LOCAL,
            ADRENOTOOLS_DRIVER_CUSTOM, t, h, d, n, nullptr, nullptr)
            : dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        auto proc = handle ? reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(handle, "vkGetInstanceProcAddr")) : nullptr;
        if (!proc) {
            error = "无法加载所选图形驱动，请检查驱动是否支持此设备和 Android 版本。";
            selected_proc = nullptr;
            selected_key.clear();
        } else {
            selected_proc = proc;
            selected_key = key;
            std::string sdl = std::string(h) + "/libngb_sdl_vulkan.so";
            setenv("SDL_VULKAN_LIBRARY", sdl.c_str(), 1);
            __android_log_print(ANDROID_LOG_INFO, "NGBDriver", "Selected driver: %s", *n ? n : "system");
        }
    }
    env->ReleaseStringUTFChars(temporary, t); env->ReleaseStringUTFChars(name, n);
    env->ReleaseStringUTFChars(directory, d); env->ReleaseStringUTFChars(hooks, h);
    return error.empty() ? nullptr : env->NewStringUTF(error.c_str());
}
