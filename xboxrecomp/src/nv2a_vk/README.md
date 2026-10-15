# nv2a_vk — NV2A GPU on Vulkan (from xemu)

The NV2A model and its Vulkan renderer, taken from
[xemu](https://github.com/xemu-project/xemu) `hw/xbox/nv2a` at commit `478b4f4`
(2026-09-28). It replaces the plain-RAM register aperture for titles that drive
the GPU through their own statically linked D3D (D3D8LTCG): their pushbuffers
are executed by xemu's PFIFO/PGRAPH and rendered with Vulkan.

- `xemu/` holds xemu's sources. The first commit that adds them is verbatim;
  every later change is a diff against that, so what was altered for this host
  is reviewable. `nv2a.c.orig` is kept for reference only: its PCI/VGA/QOM
  device glue is replaced by `host/nv2a_host.c`.
- `shim/` provides the slice of QEMU the code expects (threads, events, queues,
  the LRU and hash helpers from xemu's own `include/qemu`, timers, a minimal
  VGA register file, trace stubs).
- `host/` connects it to the runtime: register traps, interrupts, guest RAM as
  VRAM, and presentation.

Licensing: the xemu files are LGPL-2.1-or-later (see each file's header and
`xemu/COPYING.LIB`); `shim/qemu/lru.h` is MIT. Third-party libraries come from
`third_party/fetch.sh`.
