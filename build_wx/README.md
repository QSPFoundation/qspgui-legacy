# wxWidgets patch

`wxPatch.diff` is applied to the pinned wxWidgets revision at build time. Everything that can be
done from the player itself lives in the player; only two changes are left:

| File | Change | Why it stays |
|---|---|---|
| `include/wx/htmllbox.h` | `m_htmlParser` and `m_filesystem` of `wxHtmlListBox` are protected instead of private | `QSPListBox` creates the parser itself to resolve the images of the lists inside the game folder; it needs an upstream change to `wxHtmlListBox` |
| `src/html/winpars.cpp` | `wxHtmlWinParser::OpenURL` passes URLs to `OnHTMLOpeningURL` as they are, without resolving and unescaping them first | The checks of game resources (`QSPTools::GetGameResourceURL`) recognize the file URLs they build only in the exact form they built them |

Moved to the player:

- Page and link colours: `<BODY BGCOLOR LINK>` in the pages of `QSPTextBox` and `QSPListBox`.
- Scrolling over the background image: `QSPTextBox::ScrollWindow`.
- Pane sizes of `wxAuiManager`: `QSPFrame::RememberPaneSizes`.
- `<A CLASS="PLAIN">` and `<IMG>`: our own tag handlers in `htmltags.cpp`.
- Animations of the image pane: `QSPAnimWin` with `QSPAnimation`.
