# Contributors

People who made this port better, with code, testing, saves, traces or a report that found what was
wrong. If you've contributed and aren't listed, or a line is wrong, open an issue or a pull request
against this file.

The recompiler and Xbox runtime are [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by
sp00nznet; its own contributors are in [xboxrecomp/CONTRIBUTORS.md](xboxrecomp/CONTRIBUTORS.md). The
renderer builds on TGP482's [SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) and
[Xbox-XBERecomp](https://github.com/TGP482/Xbox-XBERecomp), and the GPU model and audio processor on
[xemu](https://xemu.app); see [THIRD_PARTY.md](THIRD_PARTY.md).

## Maintainer

- [gamboozino](https://github.com/gamboozino)

## Code

- [akirahayabusa](https://github.com/akirahayabusa): the freeze in scenes with the sun (#40), folders
  with non-ASCII names on Windows (#41), the Difficulty Selection highlight (#42)
- [ManemanFTW](https://github.com/ManemanFTW) (Manuel Flores): the Windows start-up hang (#9) and the
  audio crackling on Windows (#34)
- [HammerD72](https://github.com/HammerD72): the audio timing fix (#32); the two spots where the game
  picks the camera view at a load, for Start in Third Person (#23)
- [retro-meister](https://github.com/retro-meister): the macOS port (#49), and the community macOS builds

## Testing, saves and traces

- [Radarzo](https://github.com/Radarzo), [hj-0](https://github.com/hj-0),
  [hyperspeedgx](https://github.com/hyperspeedgx), [serguntchik](https://github.com/serguntchik),
  [PaulAndrewClift](https://github.com/PaulAndrewClift) and
  [phantuannghia95-rgb](https://github.com/phantuannghia95-rgb): seven test builds, screenshots and logs
  on Nvidia cards that tracked down the banded water (#16)
- [HammerD72](https://github.com/HammerD72): testing on Nvidia across many issues (shadows #53, water
  #16, menus and controls)
- [DAVID106-debug](https://github.com/DAVID106-debug): the missing particles and the Xbox comparison
  (#51), the high-resolution shadow flicker (#53)
- [hyperspeedgx](https://github.com/hyperspeedgx): saves across chapters 1 to 9, traces for the torch
  embers (#51) and the Chapter 5 roof (#29)
- [hj-0](https://github.com/hj-0): saves for chapters 11 to 13 and a guide to moving saves from xemu
  (#61)
- [yoyooo941-sys](https://github.com/yoyooo941-sys): the save that made the volcano strip reproducible
  (#55)
- [firepro1211](https://github.com/firepro1211) and
  [huokunlongCHAIN](https://github.com/huokunlongCHAIN): the Chapter 5 roof, including a comparison on
  the Xbox One (#29)
- [matitone71](https://github.com/matitone71): the Steam Deck Game Mode log that found the launch
  problem (#11)
