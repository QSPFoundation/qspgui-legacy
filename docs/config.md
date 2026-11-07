# qspgui.cfg

The player keeps its settings in `qspgui.cfg`, an INI file read and written with `wxFileConfig`.

## Location

The player uses the first match:

1. `qspgui.cfg` next to the executable, if the file exists or the folder is writable (portable mode).
2. `qspgui.cfg` in the user configuration folder (`wxStandardPaths::GetUserDir(Dir_Config)`):
   - Windows: `%APPDATA%`
   - macOS: `~/Library/Preferences`
   - Linux: the home folder

When a game is opened and its folder contains `qspgui.cfg`, the player saves its current settings and
switches to that file until another game is opened.

Upstream `qspgui` uses the same file name and the same locations, so outside portable mode both players
share one file. See [Compatibility with upstream qspgui](#compatibility-with-upstream-qspgui).

## Format

Values are stored as text. Booleans are `0` or `1`. Colors are decimal integers of the form `0xBBGGRR`
(blue in the high byte), the value a game assigns to `BCOLOR`, `FCOLOR` and `LCOLOR`.
Missing or unreadable entries fall back to the defaults below.

### [Colors]

| Key | Type | Default | Description |
|---|---|---|---|
| `BackColor` | color | `14737632` (`0xE0E0E0`) | Background of all panes, unless the game sets `BCOLOR` |
| `FontColor` | color | `0` (`0x000000`) | Text color, unless the game sets `FCOLOR` |
| `LinkColor` | color | `16711680` (`0xFF0000`, blue) | Link color, unless the game sets `LCOLOR` |

### [Font]

| Key | Type | Default | Description |
|---|---|---|---|
| `FontSize` | integer, points | system font size, at least 12 | Font size, unless the game sets `FSIZE` |
| `FontName` | string | system font face | Font face, unless the game sets `$FNAME`; an unavailable face falls back to the system font |
| `UseFontSize` | boolean | `0` | Use `FontSize` even when the game sets `FSIZE` |

### [General]

| Key | Type | Default | Description |
|---|---|---|---|
| `Volume` | integer, 0–100 | `100` | Overall volume |
| `ShowHotkeys` | boolean | `0` | Show `[1]`–`[9]` before the first nine actions |
| `Panels` | string | built-in layout | Pane layout, a `wxAuiManager` perspective. Pane names: `imgview`, `desc`, `objs`, `acts`, `vars`, `input` |
| `CheckUpdates` | boolean | `1` | Check for updates on startup |
| `Language` | string | empty | Interface language as a canonical locale name, e.g. `ru_RU`; empty means the system language |

### [Pos]

| Key | Type | Default | Description |
|---|---|---|---|
| `Left`, `Top` | integer, pixels | `10` (scaled to the display) | Window position |
| `Width`, `Height` | integer, pixels | `850`, `650` (scaled to the display) | Client area size |
| `Maximize` | boolean | `0` | Window is maximized |

### [Render]

Selects the HTML renderer of each pane, also from **Settings → HTML renderer…**. Games can't choose the renderer from their code, but like
the other settings these entries are read from the active `qspgui.cfg`: a game author can ship a
`qspgui.cfg` with `[Render]` in the game folder. The renderer the user picks while such a game is
open is saved into that file too.

| Key | Values | Default | Description |
|---|---|---|---|
| `MainDesc` | `wxhtml`, `litehtml` | `wxhtml` | Main description |
| `SmoothBackImage` | `0`, `1` | `1` | `1` smooths an enlarged `BACKIMAGE` as browsers do, `0` keeps its pixels sharp as in wxHTML. Only for litehtml: wxHTML always keeps them sharp |

Unknown values of `MainDesc` mean `wxhtml`, so an older player reads a file written by a newer one, and so does a
player built without litehtml (`-DQSP_LITEHTML=OFF`).
More panes (`Actions`, `VarsDesc`, `Objects`, dialogs) will be added as new keys.

## Example

```ini
[Colors]
BackColor=14737632
FontColor=0
LinkColor=16711680
[Font]
FontSize=12
FontName=Segoe UI
UseFontSize=0
[General]
Volume=100
ShowHotkeys=0
Panels=layout2|name=imgview;...
CheckUpdates=1
Language=
[Pos]
Left=10
Top=10
Width=850
Height=650
Maximize=0
[Render]
MainDesc=litehtml
SmoothBackImage=1
```

## Compatibility with upstream qspgui

Both players read and write the same keys with the same meaning, and `wxFileConfig` keeps entries it
doesn't know when it rewrites the file. Upstream `qspgui` ignores `[Render]` and keeps it intact.

Rules for new settings:

- Never change the meaning, type or unit of an existing key; add a new key instead.
- Keep the pane names in `General/Panels`: upstream restores the layout by these names.
- Put settings of this player only into new sections, like `[Render]`.

If both players run at the same time, the one that exits last overwrites the settings of the other.
