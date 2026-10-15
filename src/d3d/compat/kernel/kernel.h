// The guest memory accessors SCDA-XBERecomp's gpu/ code uses, on this runtime's guest memory
// (recomp_types.h: XBOX_PTR, g_xbox_mem_offset).
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

extern "C" ptrdiff_t g_xbox_mem_offset;

namespace kernel {
inline void* HostPtr(uint32_t va) { return reinterpret_cast<void*>(uintptr_t(va) + uintptr_t(g_xbox_mem_offset)); }
inline uint32_t R32(uint32_t va) { uint32_t v; std::memcpy(&v, HostPtr(va), 4); return v; }
inline uint16_t R16(uint32_t va) { uint16_t v; std::memcpy(&v, HostPtr(va), 2); return v; }
inline uint8_t R8(uint32_t va) { return *static_cast<uint8_t*>(HostPtr(va)); }
inline void W32(uint32_t va, uint32_t v) { std::memcpy(HostPtr(va), &v, 4); }
}  // namespace kernel

namespace mem {
// a resource's physical address to where the title sees it: contiguous memory is mapped at 0x80000000
inline uint32_t VirtualFromPhysical(uint32_t pa) { return pa | 0x80000000u; }
// a resource's data: the renderer's own vertex buffer headers (Common bit 31) hold a virtual address
inline uint32_t ResourceData(uint32_t res) {
    uint32_t d = kernel::R32(res + 4);
    return (kernel::R32(res) & 0x80000000u) ? d : VirtualFromPhysical(d);
}
}  // namespace mem
