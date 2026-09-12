# Keyboard Shortcuts (QML Client)

Overview of the QML client's keyboard shortcuts. The shortcuts are defined in the QML
files – the corresponding references are given in parentheses. The shortcuts of
the Qt Widgets client are listed in
[widget_client_keyboard_shortcuts.md](widget_client_keyboard_shortcuts.md); the
differences between the two are at the end of this page.

## Global (everywhere)

| Key | Action |
|-----|--------|
| `Esc` | Back (close the active top-bar section); otherwise close the side menu |
| `Alt+S` | Toggle settings |
| `F11` | Toggle fullscreen (also active for spectators) |
| `Back` (Android/browser back) | Navigate back |

Source: [pokerth.qml](../src/gui/qt6-qml/pokerth.qml#L823-L857)

## At the game table

The following shortcuts apply while the game page is visible
(context `Qt.ApplicationShortcut`). Source: [pages/GamePage.qml](../src/gui/qt6-qml/pages/GamePage.qml#L260-L364)

### Toggle panels

| Key | Action |
|-----|--------|
| `Alt+L` | Toggle history tab |
| `Alt+I` | Toggle info panel (odds) |
| `Alt+C` | Toggle chat |

> Panel shortcuts are disabled for spectators. Fullscreen (`F11`) deliberately
> sits on the ApplicationWindow instead, see above.

### Game actions – function keys

Default layout:

| Key | Action |
|-----|--------|
| `F1` | Fold |
| `F2` | Call / Check |
| `F3` | Bet / Raise |
| `F4` | All-In |
| `F5` | Show your own cards |

With the setting **"Reverse function keys"** (`AlternateFKeysUserActionMode`) enabled,
the order of F1–F4 is reversed:

| Key | Action (reversed) |
|-----|-------------------|
| `F1` | All-In |
| `F2` | Bet / Raise |
| `F3` | Call / Check |
| `F4` | Fold |

Source: [pages/GamePage.qml](../src/gui/qt6-qml/pages/GamePage.qml#L216-L229)

### Switch playing mode

There are two equivalent key sets for the automatic playing mode:

| Mode | Letter | Function key |
|------|--------|--------------|
| Manual | `Alt+M` | `F6` |
| Auto Check/Call | `Alt+K` | `F8` |
| Auto Check/Fold | `Alt+F` | `F7` |

## Chat input field

Only active while the chat input field has focus.
Source: [components/ChatBox.qml](../src/gui/qt6-qml/components/ChatBox.qml#L611-L669)

| Key | Action |
|-----|--------|
| `Enter` / `Return` | Send message |
| `Tab` | Nickname completion |
| `↑` / `↓` | Browse the input history |
| `Ctrl+C` | Copy; with nothing selected in the input field, it copies the selection in the chat history |

### Emote suggestion list (`:` + at least 2 letters)

While the emote popup is open, these keys apply:

| Key | Action |
|-----|--------|
| `↑` / `↓` | Select suggestion |
| `Tab` / `Enter` / `Return` | Accept suggestion |
| `Esc` | Close the suggestion list |

## Differences to the Qt Widgets client

Identical in both clients: `F1`–`F5` including the reversal through
`AlternateFKeysUserActionMode`, `F6`/`F7`/`F8` for the playing mode, and the
chat keys `Enter`, `Tab` and `↑`/`↓`.

| Function | QML | Qt Widgets |
|----------|-----|------------|
| Fullscreen | `F11` | `Ctrl+F` |
| Chat panel | `Alt+C` | `Ctrl+T` |
| Log / history | `Alt+L` | `Ctrl+L` |
| Odds / chance | `Alt+I` | `Ctrl+C` |
| Playing mode by letter | `Alt+M` / `Alt+K` / `Alt+F` | – |
| Settings | `Alt+S` | – |
| Back / close the current section | `Esc` | – |
| Close the emote list | `Esc` | – |
| Copy the chat history selection | `Ctrl+C` | – |
| Hands window | – | `Ctrl+H` |
| Away window | – | `Ctrl+A` |
| Menu entries (new/create/join/internet game, close, quit) | – | `Ctrl+N`/`O`/`J`/`I`/`X`/`Q` |
| Pause the local game | – | `Shift` |
