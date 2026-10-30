// Check the port's real HLSL conversion/compiler without a window or game data.
#include "vk/spirv.h"
#include <spirv-tools/libspirv.h>
#include <cstdio>
#include <string>
#include <vector>

int main() {
    const char* shaders[] = {
        "float4 main(float4 v0 : ATTR0) : SV_Position { return v0; }",
        "Texture2D image : register(t0);\n"
        "SamplerState filtering : register(s0);\n"
        "float4 main(float2 uv : TEXCOORD0) : SV_Target { return image.Sample(filtering, uv); }"
    };
    for (int stage = 0; stage < 2; stage++) {
        std::vector<uint32_t> words;
        std::string log;
        if (!vkr::Compile(shaders[stage], stage == 0, words, &log) || words.empty()) {
            std::fprintf(stderr, "HLSL stage %d failed: %s\n", stage, log.c_str());
            return 1;
        }
        spv_context context = spvContextCreate(SPV_ENV_VULKAN_1_1);
        if (!context) return 1;
        spv_const_binary_t binary{words.data(), words.size()};
        spv_diagnostic diagnostic = nullptr;
        spv_result_t result = spvValidate(context, &binary, &diagnostic);
        if (result != SPV_SUCCESS)
            std::fprintf(stderr, "SPIR-V stage %d invalid: %s\n", stage,
                         diagnostic ? diagnostic->error : "no diagnostic");
        spvDiagnosticDestroy(diagnostic);
        spvContextDestroy(context);
        if (result != SPV_SUCCESS) return 1;
        std::printf("shader stage %d: %zu words, validation passed\n", stage, words.size());
    }
    return 0;
}
