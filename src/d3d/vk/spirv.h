#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vkr {
// the HLSL with Vulkan bindings: cbuffers b0 b1 -> 0 1, textures t0-3 -> 2-5, samplers s0-3 -> 6-9,
// vertex input ATTRn -> location n
std::string VulkanHlsl(const std::string& hlsl);
bool Compile(const std::string& hlsl, bool vertex, std::vector<uint32_t>& spirv, std::string* log = nullptr);
}  // namespace vkr
