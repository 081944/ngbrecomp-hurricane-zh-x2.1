/* A device-only probe: no window, audio, game data or desktop dependencies. */
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int extension(const VkExtensionProperties *items, uint32_t n, const char *name)
{
    for (uint32_t i = 0; i < n; i++)
        if (!strcmp(items[i].extensionName, name)) return 1;
    return 0;
}

int main(void)
{
    uint32_t version = VK_API_VERSION_1_0;
    PFN_vkEnumerateInstanceVersion enumerate = (PFN_vkEnumerateInstanceVersion)
        vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
    if (enumerate && enumerate(&version) != VK_SUCCESS) return 1;
    if (version < VK_API_VERSION_1_1) {
        fprintf(stderr, "Vulkan 1.1 or newer is needed for capability queries\n");
        return 1;
    }
    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "NGB Android capability probe",
        .apiVersion = version < VK_API_VERSION_1_3 ? version : VK_API_VERSION_1_3};
    VkInstanceCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app};
    VkInstance instance;
    VkResult result = vkCreateInstance(&ci, NULL, &instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", result);
        return 1;
    }
    uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(instance, &count, NULL) != VK_SUCCESS || !count) {
        fprintf(stderr, "No Vulkan physical devices\n");
        vkDestroyInstance(instance, NULL);
        return 1;
    }
    VkPhysicalDevice *devices = calloc(count, sizeof(*devices));
    if (!devices) { vkDestroyInstance(instance, NULL); return 1; }
    if (vkEnumeratePhysicalDevices(instance, &count, devices) != VK_SUCCESS) {
        free(devices); vkDestroyInstance(instance, NULL); return 1;
    }
    printf("host_page_bytes=%ld\n", sysconf(_SC_PAGESIZE));
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties p;
        VkPhysicalDeviceFeatures2 f = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan13Features f13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        vkGetPhysicalDeviceProperties(devices[i], &p);
        if (p.apiVersion >= VK_API_VERSION_1_3 && app.apiVersion >= VK_API_VERSION_1_3)
            f.pNext = &f13;
        vkGetPhysicalDeviceFeatures2(devices[i], &f);
        printf("device[%u]=%s\nvendor_id=0x%04x\ndevice_id=0x%04x\ndriver_version=%u\n",
               i, p.deviceName, p.vendorID, p.deviceID, p.driverVersion);
        printf("vulkan_api=%u.%u.%u\n", VK_VERSION_MAJOR(p.apiVersion),
               VK_VERSION_MINOR(p.apiVersion), VK_VERSION_PATCH(p.apiVersion));
        uint32_t n = 0;
        result = vkEnumerateDeviceExtensionProperties(devices[i], NULL, &n, NULL);
        if (result != VK_SUCCESS) { free(devices); vkDestroyInstance(instance, NULL); return 1; }
        VkExtensionProperties *exts = calloc(n ? n : 1, sizeof(*exts));
        if (!exts) { free(devices); vkDestroyInstance(instance, NULL); return 1; }
        result = vkEnumerateDeviceExtensionProperties(devices[i], NULL, &n, exts);
        if (result != VK_SUCCESS) { free(exts); free(devices); vkDestroyInstance(instance, NULL); return 1; }
        int push = extension(exts, n, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        printf("push_descriptor=%d\ndynamic_rendering=%u\nsynchronization2=%u\n",
               push, f13.dynamicRendering, f13.synchronization2);
#define FEATURE(name) printf(#name "=%u\n", f.features.name)
        FEATURE(textureCompressionBC); FEATURE(fillModeNonSolid);
        FEATURE(samplerAnisotropy); FEATURE(depthClamp); FEATURE(geometryShader);
        FEATURE(occlusionQueryPrecise); FEATURE(shaderClipDistance);
        FEATURE(shaderTessellationAndGeometryPointSize); FEATURE(wideLines); FEATURE(largePoints);
#undef FEATURE
        const VkFormat formats[] = {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
            VK_FORMAT_BC2_UNORM_BLOCK, VK_FORMAT_BC3_UNORM_BLOCK,
            VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT_S8_UINT};
        const char *names[] = {"BC1", "BC2", "BC3", "BGRA8", "D24S8", "D32S8"};
        for (unsigned k = 0; k < sizeof(formats) / sizeof(formats[0]); k++) {
            VkFormatProperties fp;
            vkGetPhysicalDeviceFormatProperties(devices[i], formats[k], &fp);
            printf("format_%s_optimal=0x%x\n", names[k], fp.optimalTilingFeatures);
        }
        printf("d3d_core_requirements=%s\n",
               f13.dynamicRendering && f13.synchronization2 && push ? "present" : "missing");
        puts("presentation_support=not_tested_no_surface");
        puts("gameplay_and_performance=not_tested");
        free(exts);
    }
    free(devices);
    vkDestroyInstance(instance, NULL);
    return 0;
}
