# Ninja Gaiden Black — static recompilation

A native Linux (and, later, Windows) build of *Ninja Gaiden Black* (Xbox, NTSC-U,
title ID `5443000D`), made with [xboxrecomp](https://github.com/sp00nznet/xboxrecomp).
The game's x86 code is lifted to C and compiled for the host. Nothing is emulated
at the CPU level. You need your own disc image; no game data is in this repo.

The toolkit changes this project needs live on the `linux-vulkan` branch of
`../xboxrecomp`.

## Status

| Stage | State |
|---|---|
| Disc extraction, XBE parse, disassembly (11.5k functions), lift to C (2.2M lines) | done, `tools/pipeline.sh` |
| Native Linux build (clang, ~10 s at `-j16`, <400 MB per chunk) | done |
| XAPI start-up: hard-disk partitions, utility drive, CRT, threads | boots |
| Direct3D init: GPU interrupt, vblank, pushbuffer fence | passes (fence is mirrored, not executed) |
| DirectSound init: APU registers, DSP effects download | passes (DSP mailbox acknowledged, not executed) |
| File table: opens every `.afs` on the disc | done |
| Main loop | **running** (~24 M indirect calls/s), submitting GPU work every frame |
| GPU (NV2A pushbuffer → Vulkan) | **not started**: frames are black |
| Audio output, controller input, saves | not started |
| Windows build | not started |

## Build and run (Linux)

```sh
# Once: extract your disc and generate the C (about 2 minutes)
mkdir -p disc && cp /path/to/ngb.xiso disc/ngb.xiso      # or pass the path
tools/pipeline.sh disc/ngb.xiso

cmake -S . -B build/linux -G Ninja -DCMAKE_C_COMPILER=clang
ninja -C build/linux
build/linux/NinjaGaidenBlack game_files
```

The game directory is the first argument, then `NGB_GAME_DIR`, then `game/` next to
the executable. Hard-disk images and saves go to `~/.local/share/NinjaGaidenBlack`
(sparse files; `NGB_SAVE_DIR` overrides).

A settings screen comes up first (keyboard, mouse or controller): aspect ratio (4:3,
16:9, 16:10 for the Steam Deck, or stretch), window size, windowed/fullscreen, render
resolution (1x-4x), V-Sync, skipping the intro movies (the boot movie and the New Game
prologue, which the game itself will not let you skip; story movies are untouched), and
whether to show the screen at all. They are saved to `~/.config/NinjaGaidenBlack/settings.ini`; `--settings` brings
the screen back if it is turned off. In game, F11 or Alt+Enter toggles fullscreen.

The environment still overrides the file, for testing: `NGB_WIDESCREEN=0|1`,
`NGB_FULLSCREEN=0|1`, `NGB_VSYNC=0|1`, `NV2A_SCALE=n`, `NGB_NO_SETTINGS=1` (skip the
screen), `NGB_AUDIO=0` (mute).

## Bring-up tools

- `kill -USR1 <pid>` prints the guest thread's host backtrace (every lifted function
  is a named `sub_XXXXXXXX`) and its guest registers. It needs no ptrace, so use it
  instead of gdb: gdb swallows the TF single-steps that the register traps depend on.
- `RECOMP_WATCHDOG_SECS=N` dumps guest state and exits after N seconds.
- `tools/feedback.sh [secs]` runs an instrumented build, records every
  indirect-branch target it reaches, reseeds function detection and regenerates.
  Run it after the game reaches new code.
- `XBOX_LOG_LEVEL=4` and `RECOMP_KERNEL_LOG_BUDGET=100000` give a full kernel and
  file trace.

The defaults in `bringup_defaults()` (`src/main.c`) stand in for hardware that isn't
modelled yet. Each one should disappear when the real thing exists.

## Roadmap

1. **NV2A renderer on Vulkan.** The game's statically linked D3D (D3D8LTCG) writes
   NV2A pushbuffers itself, so the renderer has to execute those, not D3D calls.
   The plan is to port xemu's PGRAPH Vulkan backend (`hw/xbox/nv2a/pgraph/vk`,
   LGPL, about 20k lines) behind a small QEMU-shim, fed by a PFIFO thread that
   follows `DMA_PUT`. That also makes the fence mirror honest. One backend serves
   Linux and Windows.
2. **Display and input:** SDL window/swapchain, gamepad → the emulated USB pad
   (OHCI is already modelled).
3. **Audio:** the APU voice processor exists; it needs a host output (SDL) and a GP
   DSP for effects. Then the DSP ack goes away.
4. **Steam Deck performance:** pipeline cache (no shader-compile stutter), render
   resolution scaling, PGO and LTO for the lifted code, frame pacing at 60/30 Hz.
5. **Windows build:** cross-compiled with clang + xwin, run under Proton for testing.
