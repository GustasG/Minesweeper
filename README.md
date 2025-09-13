# Minesweeper (Win32)

A classic Minesweeper implementation for Windows using the raw Win32 API, bitmap assets, and DPI-aware rendering.

## Features

- Beginner/Intermediate/Expert presets
- Custom game with width, height, and mine count
- Pixel-accurate XP-style bitmaps (cells, borders, counters, faces)
- DPI-aware layout (resizes controls and assets by window DPI)
- Keyboard: `F2` starts a new game

## Controls

- Left-click: Reveal a cell
- Right-click: Flag/Unflag a cell
- Click the face button to start a new game of the current difficulty
- `F2`: New game

## Custom Game

- Use `Game → Custom…` to set:
  - Width (columns)
  - Height (rows)
  - Mines
- The dialog pre-fills with the current board values. When playing a custom board, `Game → New` or clicking the face restarts with the same width/height/mines.

## Build

- Requirements
  - Windows 10/11 with the Windows SDK
  - Visual Studio 2022 (v143 toolset) or MSBuild
- Open in Visual Studio
  - Open `Minesweeper.sln`
  - Choose `x64` and `Debug` or `Release`
  - Build and Run (`F5`)
- Build with MSBuild (Developer Command Prompt)
  - `MSBuild Minesweeper.sln /p:Configuration=Release /p:Platform=x64`
- Notes
  - Links against `dwmapi.lib`
  - Assets are embedded via `Minesweeper.rc`

## License

MIT — see `LICENSE`.

## About

- [GitHub](https://github.com/GustasG/Minesweeper)
- Built with the Win32 API and GDI, no external dependencies.
- MIT — see `LICENSE`.
