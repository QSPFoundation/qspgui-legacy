# Audio library patches

The audio libraries in `qspgui-legacy/sound/thirdparty` are kept exactly as released upstream:

| File | Upstream version |
|---|---|
| `miniaudio.h` | miniaudio 0.11.25 |
| `stb_vorbis.c` | stb_vorbis 1.22 (nothings/stb master) |
| `tsf.h`, `tml.h` | TinySoundFont `ec8bce7` |

Local fixes live in the `*.diff` files of this folder. CMake copies the libraries to `<build>/audio-patched`,
applies every patch there and compiles the sound engine against the copy. The copy is recreated whenever
a library or a patch changes, so nothing has to be done by hand.

| Patch | Fixes |
|---|---|
| `miniaudio.diff` | Use-after-free when a sound file fails to load (fixed the same way in the miniaudio dev branch); out-of-bounds reads in the dr_mp3 Xing/Info tag parser (ported from dr_libs master) |
| `stb_vorbis.diff` | CVE-2023-45675..45682: allocation sizes checked in 64 bits, comment list kept consistent on errors, wild reads while decoding, truncated setup headers rejected (based on the SDL_mixer fixes) |
| `tinysoundfont.diff` | Voice count increased before the allocation succeeded; unchecked allocations and division by zero in the MIDI loader |

To update a library, replace its file in `thirdparty` and make sure the patches still apply
(`git apply --check` from a copy of the folder); drop the parts that upstream has fixed.
