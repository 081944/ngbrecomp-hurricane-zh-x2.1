#pragma once
// A resource header's words as of the call being drawn. The console's push buffer takes a texture's format and
// address, a surface's or a vertex buffer's address, when they are set; the title points the same header elsewhere
// before the GPU gets there. A render thread draws after the call returned, so its capture keeps a copy of each
// header a call uses (src/d3d/gpu/threaded.cpp) and shows them here while it draws; without one, the guest's
// memory is read, as when the renderer draws inside the call.
#include <cstdint>
#include <unordered_map>

#include "kernel/kernel.h"

namespace gpu {

struct HeaderCopy {
    uint32_t w[5];   // common, data, lock, format, size
};

inline thread_local const std::unordered_map<uint32_t, HeaderCopy>* g_header_view = nullptr;

// word i (0-4) of the header at guest address header
inline uint32_t HeaderWord(uint32_t header, int i) {
    if (g_header_view) {
        auto it = g_header_view->find(header);
        if (it != g_header_view->end()) return it->second.w[i];
    }
    return kernel::R32(header + 4 * uint32_t(i));
}

// mem::ResourceData through the copies: the data's virtual address (Common bit 31: already virtual)
inline uint32_t ResourceDataOf(uint32_t res) {
    uint32_t d = HeaderWord(res, 1);
    return (HeaderWord(res, 0) & 0x80000000u) ? d : mem::VirtualFromPhysical(d);
}

}  // namespace gpu
