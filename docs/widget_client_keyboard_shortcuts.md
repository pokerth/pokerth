# Keyboard Shortcuts (Qt Widgets Client)

Overview of the classic Qt Widgets client's keyboard shortcuts. The menu
shortcuts are defined in the `.ui` files, everything else in the dialog classes –
the corresponding references are given in parentheses. The shortcuts of the QML
client are listed in
[qml_client_keyboard_shortcuts.md](qml_client_keyboard_shortcuts.md); the
differences between the two are at the end of this page.

## Menu shortcuts

Available while the game table window has the focus.
Source: [gametable.ui](../src/gui/qt/gametable.ui#L4696-L4824)

| Key | Action |
|-----|--------|
| `Ctrl+N` | Start local game ... |
| `Ctrl+O` | Create network game ... |
| `Ctrl+J` | Join network game ... |
| `Ctrl+I` | Internet game ... |
| `Ctrl+F` | Fullscreen |
| `Ctrl+T` | Show/hide the chat window |
| `Ctrl+L` | Show/hide the log window |
| `Ctrl+C` | Show/hide the chance window |
| `Ctrl+H` | Show/hide the hands window |
| `Ctrl+A` | Show/hide the away window |
| `Ctrl+X` | Close the game table |
| `Ctrl+Q` | Quit |

The start window carries `Ctrl+N`, `Ctrl+I` and `Ctrl+Q` as well.
Source: [startwindow.ui](../src/gui/qt/startwindow.ui)

## At the game table

Source: [gametable/gametableimpl.cpp](../src/gui/qt/gametable/gametableimpl.cpp#L3478-L3560)

### Game actions – function keys

Default layout:

| Key | Action |
|-----|--------|
| `F1` | Fold |
| `F2` | Call / Check |
| `F3` | Bet / Raise |
| `F4` | All-In |
| `F5` | Show your own cards |

With the setting **"Reverse function keys"** (`AlternateFKeysUserActionMode`)
enabled, the order of F1–F4 is reversed:

| Key | Action (reversed) |
|-----|-------------------|
| `F1` | All-In |
| `F2` | Bet / Raise |
| `F3` | Call / Check |
| `F4` | Fold |

### Switch playing mode

| Mode | Key |
|------|-----|
| Manual | `F6` |
| Auto Check/Fold | `F7` |
| Auto Check/Call | `F8` |

### Other keys at the table

| Key | Action |
|-----|--------|
| `Enter` / `Return` | With the focus in the bet field: trigger Bet / Raise |
| `Shift` | Pause the game (local game only) |

> `F6`–`F8` and the chat history keys sit in a `#ifndef GUI_800x480` block, so
> they are missing in the 800x480 build.

## Chat input

Applies at the game table
([gametableimpl.cpp](../src/gui/qt/gametable/gametableimpl.cpp#L3607-L3701)), in
the lobby
([gamelobbydialogimpl.cpp](../src/gui/qt/gamelobbydialog/gamelobbydialogimpl.cpp#L1641-L1717))
and in the waiting room
([startnetworkgamedialogimpl.cpp](../src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.cpp#L230-L263)).

| Key | Action |
|-----|--------|
| `Enter` / `Return` | Send message |
| `Tab` | Nickname completion (not while the emote list is open) |
| `↑` / `↓` | Browse the input history |

### Emote suggestion list (`:` + at least 2 letters)

While the emote popup is open, these keys apply.
Source: [chattools/chattools.cpp](../src/gui/qt/chattools/chattools.cpp#L464-L501)

| Key | Action |
|-----|--------|
| `↑` / `↓` | Select suggestion |
| `Tab` / `Enter` / `Return` | Accept suggestion |

## Dialogs

| Key | Action |
|-----|--------|
| `Enter` / `Return` | "Create game" in the create-internet-game dialog, "Connect" in the join-network-game dialog |
| `Delete` | Delete the selected log file (log file dialog, with the focus in the list) |
| `Back` (Android) | Close the game table respectively the open dialog |

## Differences to the QML client

Identical in both clients: `F1`–`F5` including the reversal through
`AlternateFKeysUserActionMode`, `F6`/`F7`/`F8` for the playing mode, and the
chat keys `Enter`, `Tab` and `↑`/`↓`.

| Function | Qt Widgets | QML |
|----------|------------|-----|
| Fullscreen | `Ctrl+F` | `F11` |
| Chat panel | `Ctrl+T` | `Alt+C` |
| Log / history | `Ctrl+L` | `Alt+L` |
| Odds / chance | `Ctrl+C` | `Alt+I` |
| Hands window | `Ctrl+H` | – |
| Away window | `Ctrl+A` | – |
| Menu entries (new/create/join/internet game, close, quit) | `Ctrl+N`/`O`/`J`/`I`/`X`/`Q` | – |
| Pause the local game | `Shift` | – |
| Playing mode by letter | – | `Alt+M` / `Alt+K` / `Alt+F` |
| Settings | – | `Alt+S` |
| Back / close the current section | – | `Esc` |
| Copy the chat history selection | – | `Ctrl+C` |
| Close the emote list | – | `Esc` |
