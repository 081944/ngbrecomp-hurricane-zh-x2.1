# Android ARM64 port

Status: **playable**, 2026-10-30. The x2.1 build runs on an ARM64 phone (OnePlus 8,
Adreno 650, Mesa Turnip v25.3.0-R11) with touch controls, a touch settings menu,
ISO/folder import and driver switching. The bring-up audit further down is kept as
history: it describes the checkout before any of this existed.

## What the port does now

- `libmain.so` is the whole game, statically recompiled from the x2.1 `default.xbe`
  for `arm64-v8a`, drawn through Vulkan. No JIT, no executable guest memory.
- `libngbsetup.so` (`src/install.c` + `src/android_setup.c`) is loaded by the setup
  Activity only, so the import screen never pulls the game library in.
- First run: the setup screen appears only when `game/default.xbe` is missing or is
  not the XBE this build was made from (`ngb_check_xbe`). It imports either a folder
  (SAF tree) or a disc image (SAF fd, `ngb_install_iso_fd`, extracted into
  `game.partial` and renamed into place). After a successful import the game starts
  by itself; a manual start button and a two-failed-starts fallback stay behind it.
- In game, the **☰** button opens `TouchMenu.java` on the presenter thread:
  display (render scale, MSAA, aspect, V-Sync, FPS), quality (texture filtering,
  shadow resolution, glow, detail under load, HUD), audio (positional, channels) and
  graphics driver (system driver, imported ZIPs, ZIP import). A driver change
  restarts the app through `AlarmManager` so the process reloads the new library.
- Driver ZIPs land in the app's own `files/drivers/<id>/`; nothing is written to
  shared storage except the game folder, which the player imported on purpose.

Not implemented on Android: the hand-over to the arcade programs (`NinjaTrilogy` /
`NinjaTomoe`, `CMakeLists.txt` returns early for Android), the F1 settings overlay,
and keyboard/mouse input. No sustained-performance numbers have been recorded yet;
the 8 Gen 2 acceptance gate below is still open.

## Building and packaging

```powershell
# one-off: SDL2, GLib, glslang and the Vulkan loader for arm64-v8a
./tools/android/build-deps.ps1
./tools/android/build-glib.ps1
# debug APK, or -Release for the signed one that goes on a release page
./tools/android/build-apk.ps1
./tools/android/build-apk.ps1 -Release
./tools/android/build-apk.ps1 -Install -Serial '<adb serial>'      # also installs
python ./tools/android/make-icon.py ../icon03.png                   # launcher icons
```

`android/keystore/ngb-android.jks` is committed on purpose: the published APK has to
upgrade over local debug builds and anyone checking out the branch has to be able to
rebuild it. It is a shared test key, not a secret — do not treat an APK as trusted
because of it.

## Implementation progress

The target is the user's Thor Android handheld; its exact OS and Vulkan driver
are not yet confirmed. Build the probes for API 30 (Android 11) so they also work
on Android 13. The user has authorized the connected OnePlus 8 for initial
tests while Thor is unavailable. Thor remains the 60 fps acceptance device.

- Added `tools/pipeline.py`, a native Windows/POSIX runner for the existing
  Python extraction and recompilation tools. Generated the game code from the
  local ISO and compiled all generated C translation units for Android ARM64.
  They are object files only, not a linked or playable game.
- Enabled POSIX ARM64 exception dispatch, connected the existing AArch64 MMIO
  decoder, and adapted the host fiber assembly and its initial stack to ELF.
  Adapted the AC97 trap and the legacy NV2A MMIO hook to the ARM64 decoder.
- Replaced the Android secure-zero interface. Android remote `TerminateThread`
  explicitly returns `ERROR_NOT_SUPPORTED`; no current source caller uses it.
  This is a remaining API limitation, not implemented thread cancellation.
- Added Android preemption guards for libc critical sections while allowing
  preemption in the guest module and vDSO. Repeated timer requests retry deferred
  preemption. The vDSO exception is needed to avoid starving guest threads that
  read the clock frequently.
- Added standalone NDK builds and a Vulkan capability probe in `tools/android/`.
  Six runtime tests passed on a separate headless x86-64 Android API 35 emulator:
  exception/MMIO dispatch, vCPU scheduling, thread suspension, kernel memory,
  memory-layout aliases/teardown, and 10,000 fiber context round trips with
  independent FP rounding. All six also passed on the physical ARM64 OnePlus 8
  after correcting an alias-test ordering bug: disassembly showed the compiler
  hoisting the ordinary base load before the mirror store. Volatile accesses
  now test the mmap alias itself, as the game's MEM macros do.
- Corrected a stale kernel-memory test boundary from 64 MB to the implementation's
  existing 128 MB contiguous window. Runtime memory behavior was not changed.
- SDL2 2.32.8 and GLib 2.82.4 build and install for ARM64.
  The full renderer requires glslang 16.1.0's C entry-point API and its HLSL
  front end; the dependency driver builds it with its pinned SPIR-V optimizer.
  Build drivers pin source commits; the GLib driver also pins its Git-based
  dependencies and applies the Windows cross-install path fix.
- The complete ARM64 native game library links, and a debug APK builds with
  `libmain.so` and `libSDL2.so`. APK signature and 16 KB ZIP alignment checks
  passed. The packaged libraries are stripped copies; originals retain symbols.
- `ngb_shader_probe` ran on the OnePlus 8: both vertex and texture-sampling pixel
  HLSL compiled and passed SPIR-V validation. Its stock Adreno 650 driver reports
  Vulkan 1.1.128 and no BC1/2/3 optimal image support. The current renderer still
  requires Vulkan 1.3; this phone cannot establish target graphics compatibility.

Commands from PowerShell 7 (all output stays under ignored `build/` paths):

```powershell
python ./tools/pipeline.py 'E:/path/to/ngb.iso'
./tools/android/probes.ps1
./tools/android/build-deps.ps1
./tools/android/build-glib.ps1
./tools/android/build-apk.ps1
# Explicitly select the intended device; no phone is chosen automatically:
./tools/android/probes.ps1 -Run -Serial '<Thor adb serial>'
# Compile generated game C independently of unfinished graphics dependencies:
cmake --build ./build/android-probes/arm64-v8a --target game_code_compile -j 4
```

The initial audit below describes the original checkout; the progress above
supersedes items already addressed. A native-library target and SDL Android app
project now exist. The Java setup screen builds and launches on the test emulator
(at that stage without game native libraries); it is not evidence of game boot.
Next work is game startup, runtime and lifecycle validation on target hardware.
The prototype imports an extracted game directory using
Android's folder picker. Import status is stored outside the game directory so
the guest cannot enumerate an added marker as a game asset.

## Initial target

- Snapdragon 8 Gen 2 and newer Snapdragon flagship devices, ARM64 only.
- Proposed first bring-up system: Android 11 / API 30 or newer. The eventual
  minimum OS version should be decided after device and dependency testing.
- Stock Vulkan driver first. Query capabilities on each device; chipset name
  and Android version are not substitutes for feature and format checks.
- SDL2 controller input, landscape display, original 1x render resolution,
  MSAA off and original shadows. Target sustained 60 fps; no measured result yet.
- Main Ninja Gaiden Black game first. Touch controls, arcade executables,
  compressed-disc import and custom Vulkan drivers are later work.
- Import extracted game data into app-controlled storage for the first playable
  build. Keep game data separate from APK contents and generated source control.

## Local environment observed

The Windows host has CMake, Ninja, LLVM, Python, Git, Android SDK platforms
34/35/36, and NDK 27.2.12479018. Environment variables point to the installed
SDK, Android Studio JBR and NDK. There is no usable WSL distribution observed.
The existing pipeline is a Bash/Python workflow: choose a Windows-compatible
host runner or adapt the runner before generating the game code.

An ISO exists in the parent workspace. `src/recomp/gen/` and fetched third-party
sources are absent. Do not track either generated game code or extracted assets.
ADB currently sees a OnePlus 8 (IN2010), reporting API 30 and 4096-byte pages.
It can help with some low-level tests if an API-30 probe is built, but is outside
the proposed initial target and cannot establish target-device performance.

A Windows checkout without the symlink privilege cannot materialise
`xboxrecomp/src/nv2a_vk/shim/hw/xbox/nv2a`, a symlink to `../../../xemu`: git
writes it as an ordinary file, the shim include path then resolves to nothing, and
`git status` reports the link as deleted. Put a directory junction in its place --
no privilege needed -- and hide the difference from git:

```sh
cmd //c "mklink /J xboxrecomp\\src\\nv2a_vk\\shim\\hw\\xbox\\nv2a $(pwd)/xboxrecomp/src/nv2a_vk/xemu"
git update-index --skip-worktree xboxrecomp/src/nv2a_vk/shim/hw/xbox/nv2a
```

## Confirmed source blockers

| Area | Location | Required work |
| --- | --- | --- |
| Fibers | `src/fibers.c` | Separate CPU architecture from OS checks. Adapt the existing AArch64 context switch to ELF and match its initial stack layout. Android currently selects x86-64 assembly. |
| Exception dispatch | `xboxrecomp/src/platform/xbox_winnt.h`, `win32_compat.c` | Android ARM64 currently has `RECOMP_HAS_VEH=0`. Implement native context conversion and fault direction handling before enabling it. Merely widening the macro exposes x86 register assumptions. |
| MMIO | `platform/mmio_decode.h`, `mmio_decode_a64.h`, kernel/device handlers | Wire the existing Linux AArch64 decoder into the dispatch path and replace x86 single-step assumptions. Test integer and SIMD accesses and unsupported instruction handling. |
| Threads | `platform/win32_compat.c`, `vcpu.c` | Replace unavailable `pthread_cancel` with a design that preserves guest termination and scheduler semantics. Audit call sites; do not report a running thread as terminated just to satisfy compilation. |
| Host interfaces | `platform/win32_compat.c`, `src/main.c` | Replace unavailable `explicit_bzero` for the selected API and adapt desktop backtrace/profiling interfaces, including `execinfo.h`. |
| Memory | `platform/win32_compat.c`, `kernel/xbox_memory_layout.c` | Validate guest-window reservation, shared backing storage, aliases, protection and alignment on Android. Distinguish guest 4 KB pages from host 4/16 KB pages. |
| Dependencies | root and `nv2a_vk` CMake files | Cross-build SDL2, GLib, glslang and required SPIR-V libraries. Keep host pipeline tools separate from target libraries. Remove desktop pkg-config/linker assumptions. |
| App integration | root CMake, `src/main.c`, `install.c`, `launch.c`, SDL presenter | Build a native shared library behind an Android entry point. Provide app paths, import, pause/resume, surface recreation, audio focus and controller reconnection. Replace desktop process handover for the arcade later. |

## Graphics decisions to validate first

The Direct3D Vulkan backend requests Vulkan 1.3, dynamic rendering,
synchronization2 and `VK_KHR_push_descriptor`. Its standalone device creation
also unconditionally enables BC texture compression, anisotropy and non-solid
fill. The shared NV2A device creation has a separate capability policy.

1. Build a small native capability probe before a full game APK. Record device
   and driver identifiers, API version, queue presentation support, required
   extensions/features, depth formats, BC1/2/3 format support and memory types.
   Explicitly list missing capabilities instead of failing during game boot.
2. Make the main game's D3D rendering path explicit. The NV2A layer currently
   checks geometry shaders, depth clamp, precise occlusion and other drawing
   features. Its Apple-specific `require_draw_features` policy is a reference
   for separating D3D requirements from fallback-renderer requirements. Keep
   the NV2A services needed for command processing, fences and timing.
3. Query both device-creation paths. Only enable optional features when present;
   preserve requirements that have no implemented replacement. If push
   descriptors are absent, either provide a descriptor-set path or report the
   device unsupported by the prototype.
4. Implement a complete BC-to-uncompressed fallback if needed. `DecodeBC` already
   exists in `src/d3d/vk/backend.cpp`, but current use is for mip generation and
   does not establish a general fallback. Cover all mip levels, cubemap faces,
   alpha and cache invalidation, then measure extra memory and upload cost.
5. Test render targets, depth/stencil, blending, shader output and synchronization
   on the selected real driver. Vulkan version alone does not verify correctness.

## Acceptance gates

| Gate | Deliverable | Pass condition |
| --- | --- | --- |
| A: reproducible inputs | Host extraction/generation instructions, dependency versions, toolchain invocation | Game C is generated from the local disc; target objects use Android ARM64 libraries only. |
| B: runtime and device probes | Small native tests and a Vulkan capability report | Context switches, guest memory aliases, MMIO reads/writes and scheduler waits/preemption work on ARM64. Exercise 4 KB and 16 KB host-page behavior. |
| C: Android shell | Installable debug APK with SDL window and app data paths | Clean startup/exit, surface recreation and foreground/background transitions; useful crash logs. |
| D: first gameplay | Main game with controller input | Title, new game, a battle, loading, saving and save reload; correct drawing and audio. |
| E: baseline performance | Repeatable scenes and a 30-minute warm-device run on an 8 Gen 2 | Record frame-time distribution, slow frames, memory, thermal state and audio underruns. Evaluate against 16.67 ms/frame; cold shader compilation is measured separately. |
| F: broader support | At least one newer target device and a 16 KB environment | Repeat functional checks and sustained-performance runs; document driver-specific failures. |

For game tests, follow `AGENTS.md`: isolated game/save/config paths, silent audio
and controlled display output. Do not reuse personal saves in place. Reserve
audio-device testing for an intentional device test. A syntax check does not
substitute for scheduler, rendering or gameplay verification.

## Checks actually run

Using NDK 27.2.12479018's Windows `clang.exe` with
`--target=aarch64-linux-android33 -fsyntax-only -std=gnu11`:

- `platform/win32_compat.c`: failed on undeclared `pthread_cancel` and
  `explicit_bzero`. Exception code was excluded by `RECOMP_HAS_VEH=0`, so this
  does not audit the missing ARM64 exception implementation.
- `platform/mmio_decode_a64.h`, compiled as C: passed syntax checking.
- `templates/runtime/recomp_types.h`, compiled as C: passed syntax checking.

No target binary was linked or run. No game source was generated, no dependencies
were downloaded, and no APK was installed during this audit.

NDK r27 can be used for bring-up, with explicit 16 KB ELF alignment where needed.
All packaged native dependencies must also be compatible; linker alignment alone
does not fix page-sensitive runtime code.

## References

- Qualcomm Snapdragon 8 Gen 2 product brief:
  https://www.qualcomm.com/content/dam/qcomm-martech/dm-assets/documents/Snapdragon-8-Gen-2-Product-Brief.pdf
- Android native-code page-size guidance:
  https://developer.android.com/guide/practices/page-sizes
- Android Bionic interface status:
  https://android.googlesource.com/platform/bionic/+/master/docs/status.md
