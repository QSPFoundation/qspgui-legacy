# Video support

Games can show videos in any HTML pane with the `<video>` tag:

```html
<video src="intro.webm" autoplay loop muted width="480">
```

| Attribute | Meaning |
|---|---|
| `src` | File inside the game folder, resolved like `<img src>` |
| `width`, `height` | Size in pixels (`width` also accepts `%`); with one of them the other keeps the aspect ratio, with none the video keeps its own size |
| `autoplay` | Start playing at once; without it only the first frame is shown, as in a browser (there are no controls) |
| `loop` | Play again from the start when the video ends |
| `muted` | No sound; otherwise the sound follows the player volume |
| `align` | Same values as for `<img>` |

Content between `<video>` and `</video>` is never shown. When the text of a pane is updated, a video
that stays on the new page keeps playing instead of starting over.

## Decoders

| Files | Decoder |
|---|---|
| WebM (VP8, VP9 with alpha channel; Opus, Vorbis) | Built in, the same on every platform |
| Everything else (MP4/H.264/AAC, AV1, ...) | Windows: Media Foundation (the codecs installed in the system) |
| | macOS: AVFoundation |
| | Linux: GStreamer 1.x, loaded at runtime when present (`gstreamer1.0-plugins-good`, `gstreamer1.0-libav` and so on) |

The system decoders only read local files: GStreamer is not allowed to use network sources or HLS/DASH playlists.

## Built-in decoder libraries

`video_libs.cmake` fetches the libraries and builds them as plain C (no assembly), so the same rules
work for MinGW cross builds, MSVC, universal macOS binaries and Linux:

| Library | Version | License |
|---|---|---|
| [libvpx](https://github.com/webmproject/libvpx) | `v1.17.0`, VP8/VP9 decoders only | BSD-3-Clause |
| [libopus](https://opus-codec.org) | `1.6.1` (release tarball, SHA-256 checked) | BSD-3-Clause |
| [nestegg](https://github.com/kinetiknz/nestegg) | `767aab2` | ISC |

Vorbis is decoded by the `stb_vorbis` copy of the sound engine.

libvpx has no CMake build, so `libvpx/` holds the files its `configure` generates. They were made with

```sh
configure --target=generic-gnu --disable-vp8-encoder --disable-vp9-encoder --disable-examples \
  --disable-tools --disable-docs --disable-unit-tests --disable-webm-io --disable-libyuv \
  --enable-multithread --disable-vp9-highbitdepth --size-limit=8192x8192
```

`vpx_config.h.in` is the generated `vpx_config.h` with the platform-specific values left to CMake.
To update libvpx, regenerate these files with the new version and refresh the source list in
`video_libs.cmake` (the objects that `make` builds after such a `configure`).

Set `-DQSP_VIDEO=OFF` to build the player without video support.
