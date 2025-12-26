# ZX Spectrum Emulator

A work-in-progress ZX Spectrum Emulator written in C++.

## Features

- **Core Emulation**: 48K RAM, Z80 CPU implementation (including undocumented opcodes and R register emulation).
- **Z80 Support**: Fully implemented instructions, including Extended (ED), Index (DD/FD), and Bit (CB) prefixes.
- **Interrupts**: Support for Interrupt Modes 0, 1, and 2.
- **Graphics**: Real-time display using SFML.
- **Loading Formats**:
  - **SNA Snapshots**: Support for 48K SNA files.
  - **Z80 Snapshots**: Support for versions 1, 2, and 3 (compressed and uncompressed).
  - **TAP/TZX Tapes**: Basic support for tape loading.
- **Diagnostic Support**: Compatible with diagnostic ROMs (e.g., Brendan Alford's ZX Diagnostics).

## Prerequisites

- **C++ Compiler** (supporting C++17)
- **CMake** (3.10+)
- **SFML 3.0** (Graphics, Window, System, Audio modules)

## Building

1. Clone the repository.
2. Configure and build using CMake:

```bash
cmake -S src -B build
cmake --build build
```

## Packaging (Installers)

You can generate platform-specific installers (macOS DMG, Windows Installer, Linux Debian/TGZ) using CPack:

```bash
# Generate package
cd build
cpack
```

**Platform Specifics**:
- **macOS**: Generates a `.dmg` disk image.
- **Windows**: Generates an `.exe` installer (requires NSIS).
- **Linux**: Generates `.deb` and `.tar.gz` packages.

## Running

Run the emulator from the project root. By default, it loads the standard 48K ROM located in `roms/48k.bin`.

### macOS
**Running from Build Directory (Bundle):**
```bash
./build/ZXEmulator.app/Contents/MacOS/ZXEmulator
```
Or simply double-click `ZXEmulator` in the `build` folder.

**Running from Build Directory (Standalone Binary):**
A standalone copy of the executable is also created in the build folder, similar to Linux/Windows.
```bash
./build/ZXEmulator
```

**Running after Installation (in /Applications):**
```bash
/Applications/ZXEmulator.app/Contents/MacOS/ZXEmulator [args]
```
*Example:* `/Applications/ZXEmulator.app/Contents/MacOS/ZXEmulator -d`

### Linux / Windows
```bash
./build/ZXEmulator
```

### Command Line Arguments

| Argument | Description | Example (macOS) |
| :--- | :--- | :--- |
| `-r <file>` | Load a custom ROM file. | `./build/ZXEmulator.app/Contents/MacOS/ZXEmulator -r roms/brendanalford.bin` |
| `-s <file>` | Load a Snapshot file (`.sna` or `.z80`). | `./build/ZXEmulator.app/Contents/MacOS/ZXEmulator -s roms/pacman.z80` |
| `-t <file>` | Load a Tape file (`.tzx` or `.tap`). | `./build/ZXEmulator.app/Contents/MacOS/ZXEmulator -t roms/game.tzx` |
| `-f <file>` | Fast Load a Tape file (skips loading time). | `./build/ZXEmulator.app/Contents/MacOS/ZXEmulator -f roms/game.tzx` |
| `-d` | Start in Debug mode (paused). | `./build/ZXEmulator.app/Contents/MacOS/ZXEmulator -d` |

## Controls

The emulator maps standard ZX Spectrum keys to the PC keyboard. 
- **0-9**: Standard number keys (and Numpad 0-9).
- **Letters**: Standard letter keys.
- **Symbol Shift**: Right Shift or Ctrl.
- **Caps Shift**: Left Shift.
- **Enter**: Return/Enter.
- **Space**: Space.
- **Kempston Joystick**: Arrow Keys + Left Alt (Fire).

## Compatibility

* No Joystick support yet
* Known issues with fast loading of tape files

| Software | Publisher | Year | Format | Status | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Ant Attack** | Quicksilva | 1983 | Snapshot (.z80) | **Working** | OK |
| **Atic Atac** | Ultimate Play The Game | 1983 | Snapshot (.z80) | **Working** | OK |
| **Attack of the Killer Tomatoes** | Global Software | 1986 | Snapshot (.z80) | **Working** | OK | 
| **Back to Skool** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **Bomb Jack** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **Elite** | Firebird | 1985 | Snapshot (.z80) | **Not Working** | Hangs after showing start screen |
| **Chuckie Egg** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **Everyone's a Wally** | Mikro-Gen | 1985 | Snapshot (.z80) | **Working** | OK |
| **Gosh Wonderful ROM** | Geoff Wearmouth | 2003 | ROM (.rom) | **Working** | OK |
| **Hobbit** | Melbourne House | 1982 | Tape (.tzx) | **Working** | OK |
| **Horace and the spiders** | Sinclair Research | 1983 | ROM (.rom) | **Working** | OK |
| **Horace Goes Skiing** | Sinclair Research | 1982 | Tape (.tzx) | **Working** | OK |
| **Jetpac** | Ultimate Play The Game | 1983 | Snapshot (.z80) | **Working** | OK |
| **Jet Set Willy** | Software Projects | 1984 | Tape (.tzx) | **Working** | OK |
| **Knight Lore** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **Manic Miner** | Software Projects | 1983 | Tape (.tzx) | **Working** | OK |
| **Pacman** | Atarisoft | 1983 | Snapshot (.z80) | **Unknown** | Not sure about keys - seems to work? |
| **Sabre Wulf** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **Three weeks in paradise** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **Underwurlde** | Ultimate Play The Game | 1984 | Snapshot (.z80) | **Working** | OK |
| **ZX Diagnostics** | Dylan Smith / Brendan Alford | 2017 | ROM | **Working** | Passes all diagnostic tests. |

## Project Structure

- `src/`: Source code.
- `src/spectrum/`: Core emulation logic (Processor, Memory, Z80 Opcodes).
- `src/utils/`: Utility classes (File loading, Logging).
- `roms/`: Default ROMs and test files.

## License

MIT License. See `LICENSE` file for details.
