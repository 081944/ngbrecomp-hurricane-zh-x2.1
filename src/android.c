/* Called by SDL's Activity before its native game thread starts. */
#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include "android_vulkan.h"
extern void xbox_AndroidTouchSet(unsigned buttons, int lx, int ly, int rx, int ry);
extern void ngb_menu_open(void);   /* src/ingame_menu.c: the RECOMP SETTINGS overlay */

JNIEXPORT void JNICALL Java_org_ngb_port_VirtualPad_nativeState(JNIEnv *env, jclass cls,
    jint buttons, jint lx, jint ly, jint rx, jint ry)
{
    (void)env; (void)cls;
    xbox_AndroidTouchSet((unsigned)buttons, lx, ly, rx, ry);
}

/* The touch overlay's menu button: opens the settings overlay at the next
 * frame, from anywhere (no Back+Start, which is the game's soft reset). */
JNIEXPORT void JNICALL
Java_org_ngb_port_GameActivity_nativeOpenMenu(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    ngb_menu_open();
}

JNIEXPORT jstring JNICALL
Java_org_ngb_port_SetupActivity_checkGraphics(JNIEnv *env, jclass activity)
{
    (void)activity;
    char error[256] = "No suitable Vulkan 1.3 device was found.";
    PFN_vkGetInstanceProcAddr get = ngb_android_vulkan_proc();
    if (!get) return (*env)->NewStringUTF(env, "Vulkan driver has not been selected.");
    uint32_t version = VK_API_VERSION_1_0;
    PFN_vkEnumerateInstanceVersion enumerate_version = get ?
        (PFN_vkEnumerateInstanceVersion)get(VK_NULL_HANDLE, "vkEnumerateInstanceVersion") : NULL;
    if (enumerate_version) enumerate_version(&version);
    if (!get || version < VK_API_VERSION_1_1) {
        snprintf(error, sizeof error, "Vulkan loader: %u.%u.%u; required: 1.1 or newer.",
                 VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
        return (*env)->NewStringUTF(env, error);
    }
    PFN_vkCreateInstance create = (PFN_vkCreateInstance)get(VK_NULL_HANDLE, "vkCreateInstance");
    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "NGB compatibility check",
        .apiVersion = version < VK_API_VERSION_1_3 ? version : VK_API_VERSION_1_3};
    VkInstanceCreateInfo info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app};
    VkInstance instance = VK_NULL_HANDLE;
    if (!create || create(&info, NULL, &instance) != VK_SUCCESS) {
        return (*env)->NewStringUTF(env, "Unable to initialize Vulkan 1.3.");
    }
    PFN_vkDestroyInstance destroy = (PFN_vkDestroyInstance)get(instance, "vkDestroyInstance");
    PFN_vkEnumeratePhysicalDevices enumerate = (PFN_vkEnumeratePhysicalDevices)get(instance, "vkEnumeratePhysicalDevices");
    PFN_vkGetPhysicalDeviceProperties properties = (PFN_vkGetPhysicalDeviceProperties)get(instance, "vkGetPhysicalDeviceProperties");
    PFN_vkGetPhysicalDeviceFeatures2 features = (PFN_vkGetPhysicalDeviceFeatures2)get(instance, "vkGetPhysicalDeviceFeatures2");
    PFN_vkEnumerateDeviceExtensionProperties extensions = (PFN_vkEnumerateDeviceExtensionProperties)get(instance, "vkEnumerateDeviceExtensionProperties");
    uint32_t count = 0;
    int supported = 0;
    if (enumerate(instance, &count, NULL) == VK_SUCCESS && count) {
        VkPhysicalDevice *devices = calloc(count, sizeof(*devices));
        if (devices && enumerate(instance, &count, devices) == VK_SUCCESS) {
            for (uint32_t i = 0; i < count; i++) {
                VkPhysicalDeviceProperties p;
                properties(devices[i], &p);
                __android_log_print(ANDROID_LOG_INFO, "NGBDriver", "Device: %s, Vulkan %u.%u.%u",
                    p.deviceName, VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion), VK_VERSION_PATCH(p.apiVersion));
                snprintf(error, sizeof error, "%.160s: Vulkan %u.%u.%u; required: 1.3.", p.deviceName,
                    VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion), VK_VERSION_PATCH(p.apiVersion));
                if (p.apiVersion < VK_API_VERSION_1_3) continue;
                VkPhysicalDeviceVulkan13Features f13 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
                VkPhysicalDeviceFeatures2 f = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &f13};
                features(devices[i], &f);
                uint32_t n = 0;
                if (extensions(devices[i], NULL, &n, NULL) != VK_SUCCESS) continue;
                VkExtensionProperties *items = calloc(n ? n : 1, sizeof(*items));
                int push = 0;
                if (items && extensions(devices[i], NULL, &n, items) == VK_SUCCESS)
                    for (uint32_t k = 0; k < n; k++)
                        if (!strcmp(items[k].extensionName, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)) push = 1;
                free(items);
                if (f13.dynamicRendering && f13.synchronization2 && push) { supported = 1; break; }
                snprintf(error, sizeof error, "%.160s does not provide the required Vulkan rendering features.", p.deviceName);
            }
        }
        free(devices);
    }
    destroy(instance, NULL);
    return supported ? NULL : (*env)->NewStringUTF(env, error);
}

JNIEXPORT jboolean JNICALL
Java_org_ngb_port_GameActivity_configurePaths(JNIEnv *env, jclass activity,
    jstring game, jstring userdata, jstring config, jstring cache, jboolean silent)
{
    (void)activity;
    const char *g = (*env)->GetStringUTFChars(env, game, NULL);
    if (!g) return JNI_FALSE;
    const char *u = (*env)->GetStringUTFChars(env, userdata, NULL);
    if (!u) { (*env)->ReleaseStringUTFChars(env, game, g); return JNI_FALSE; }
    const char *c = (*env)->GetStringUTFChars(env, config, NULL);
    if (!c) {
        (*env)->ReleaseStringUTFChars(env, userdata, u);
        (*env)->ReleaseStringUTFChars(env, game, g);
        return JNI_FALSE;
    }
    const char *s = (*env)->GetStringUTFChars(env, cache, NULL);
    int ok = s && !setenv("NGB_GAME_DIR", g, 1)
        && !setenv("NGB_SAVE_DIR", u, 1) && !setenv("NGB_CONFIG_DIR", c, 1)
        && !setenv("NV2A_SHADER_CACHE", s, 1);
    if (ok) {
        setenv("NGB_FULLSCREEN", "1", 1);
        setenv("NV2A_SCALE", "1", 0);
        setenv("NV2A_FPS_LOG", "1", 1);
        setenv("NV2A_SHOW_FPS", "1", 1);
        xbox_AndroidTouchSet(0, 0, 0, 0, 0);
        if (silent) { setenv("NGB_AUDIO", "0", 1); setenv("SDL_AUDIODRIVER", "dummy", 1); }
    }
    if (s) (*env)->ReleaseStringUTFChars(env, cache, s);
    (*env)->ReleaseStringUTFChars(env, config, c);
    (*env)->ReleaseStringUTFChars(env, userdata, u);
    (*env)->ReleaseStringUTFChars(env, game, g);
    return ok ? JNI_TRUE : JNI_FALSE;
}
