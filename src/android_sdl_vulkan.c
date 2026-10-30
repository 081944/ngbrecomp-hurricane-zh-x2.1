// SDL loads this local shim by path. Keep Vulkan's function-name export out of
// the main loader bridge: volk uses a global variable with the same name.
#include "android_vulkan.h"
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    PFN_vkGetInstanceProcAddr proc = ngb_android_vulkan_proc();
    return proc ? proc(instance, name) : NULL;
}
