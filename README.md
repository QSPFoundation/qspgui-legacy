# Classic QSP Legacy player

Please check [Releases](https://github.com/QSPFoundation/qspgui-legacy/releases) section.

## Screenshots

<img alt="Screenshot 1" src="screenshots/example1.png" height="300px" /> <img alt="Screenshot 2" src="screenshots/example2.png" height="300px" />
<img alt="Screenshot 3" src="screenshots/example3.png" height="300px" /> <img alt="Screenshot 4" src="screenshots/example4.png" height="300px" />

## Linux & MacOS build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Windows build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX=out
cmake --build build --target install --config Release
```

## Chat group

Discord: https://discord.gg/6gWVYUtUGZ | Telegram: https://t.me/QuestopiaChat
