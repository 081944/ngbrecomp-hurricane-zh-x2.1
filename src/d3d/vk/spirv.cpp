// The renderer's HLSL (gpu/shaders.cpp) to SPIR-V with glslang's HLSL front end. Resource
// registers and vertex inputs get explicit Vulkan bindings and locations first: glslang would
// otherwise number them in declaration order.
#include "vk/spirv.h"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#include <cstdio>
#include <mutex>
#include <regex>

namespace vkr {

std::string VulkanHlsl(const std::string& src) {
    std::string s = src;
    // MSVC's infinity literal: glslang does not know it
    s = std::regex_replace(s, std::regex(R"(1\.#INF)"), "asfloat(0x7F800000)");
    // cbuffer X : register(bN) -> binding N
    s = std::regex_replace(s, std::regex(R"(cbuffer\s+(\w+)\s*:\s*register\(b(\d)\))"), "[[vk::binding($2)]] cbuffer $1");
    // textures t0..t3 -> bindings 2..5, samplers s0..s3 -> 6..9
    std::string out;
    std::regex tex(R"(^(\s*)(\S[^\n:]*?)\s*:\s*register\(([ts])(\d)\);)", std::regex::multiline);
    std::sregex_iterator it(s.begin(), s.end(), tex), end;
    size_t last = 0;
    for (; it != end; ++it) {
        const auto& m = *it;
        out.append(s, last, m.position() - last);
        int n = std::stoi(m[4].str()) + (m[3].str() == "t" ? 2 : 6);
        out += m[1].str() + "[[vk::binding(" + std::to_string(n) + ")]] " + m[2].str() + ";";
        last = m.position() + m.length();
    }
    out.append(s, last, std::string::npos);
    // vertex inputs: ATTRn at location n
    out = std::regex_replace(out, std::regex(R"((\w+)\s+v(\d+)\s*:\s*ATTR(\d+);)"), "[[vk::location($3)]] $1 v$2 : ATTR$3;");
    return out;
}

bool Compile(const std::string& hlsl, bool vertex, std::vector<uint32_t>& spirv, std::string* log) {
    static std::once_flag once;
    std::call_once(once, [] { glslang_initialize_process(); });
    std::string src = VulkanHlsl(hlsl);
    glslang_input_t in = {};
    in.language = GLSLANG_SOURCE_HLSL;
    in.stage = vertex ? GLSLANG_STAGE_VERTEX : GLSLANG_STAGE_FRAGMENT;
    in.client = GLSLANG_CLIENT_VULKAN;
    in.client_version = GLSLANG_TARGET_VULKAN_1_1;
    in.target_language = GLSLANG_TARGET_SPV;
    in.target_language_version = GLSLANG_TARGET_SPV_1_3;
    in.code = src.c_str();
    in.default_version = 100;
    in.default_profile = GLSLANG_NO_PROFILE;
    in.messages = GLSLANG_MSG_DEFAULT_BIT;
    in.resource = glslang_default_resource();
    glslang_shader_t* sh = glslang_shader_create(&in);
    glslang_shader_set_entry_point(sh, "main");
    bool ok = glslang_shader_preprocess(sh, &in) && glslang_shader_parse(sh, &in);
    glslang_program_t* prog = nullptr;
    if (ok) {
        prog = glslang_program_create();
        glslang_program_add_shader(prog, sh);
        ok = glslang_program_link(prog, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
    }
    if (ok) {
        glslang_program_SPIRV_generate(prog, in.stage);
        size_t n = glslang_program_SPIRV_get_size(prog);
        spirv.resize(n);
        glslang_program_SPIRV_get(prog, spirv.data());
    } else if (log) {
        *log = std::string(glslang_shader_get_info_log(sh)) + (prog ? glslang_program_get_info_log(prog) : "");
    }
    if (prog) glslang_program_delete(prog);
    glslang_shader_delete(sh);
    return ok;
}

}  // namespace vkr
