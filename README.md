# ZX Spectrum Emulator

A work-in-progress C++ emulator for the 48K ZX Spectrum.

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

## Running

Run the emulator from the project root. By default, it loads the standard 48K ROM located in `roms/48k.bin`.

```bash
./build/ZXEmulator
```

### Command Line Arguments

| Argument | Description | Example |
| :--- | :--- | :--- |
| `-r <file>` | Load a custom ROM file. | `./build/ZXEmulator -r roms/brendanalford.bin` |
| `-s <file>` | Load a Snapshot file (`.sna` or `.z80`). | `./build/ZXEmulator -s roms/pacman.z80` |
| `-t <file>` | Load a Tape file (`.tzx` or `.tap`). | `./build/ZXEmulator -t roms/game.tzx` |
| `-d` | Start in Debug mode (paused). | `./build/ZXEmulator -d` |

## Controls

The emulator maps standard ZX Spectrum keys to the PC keyboard. 
- **0-9**: Standard number keys (and Numpad 0-9).
- **Letters**: Standard letter keys.
- **Symbol Shift**: Right Shift or Ctrl.
- **Caps Shift**: Left Shift.
- **Enter**: Return/Enter.
- **Space**: Space.


## Compatibility


## Compatibility

| Software | Format | Status | Notes |
| :--- | :--- | :--- | :--- |
| **Jet Set Willy** | Tape(.tsz) | **Partial** |  Loads correctly, Keeps ending game when you clisk enter to start |
| **Horace Goes Skiing** | Tape (.tzx) | **Partial** | Works Perfectly|
| **Manic Miner** | Tape (.tzx) | **Partial** | Loads correctly, audio/video synced.  Keeps ending game when you clisk enter to start |
| **Pacman** | Snapshot (.z80) | **Working** | Loads and runs. |
| **ZX Diagnostics** | ROM | **Partial** | Passes initial diagnostic tests. Still some issues. |

## Project Structure

- `src/`: Source code.
- `src/spectrum/`: Core emulation logic (Processor, Memory, Z80 Opcodes).
- `src/utils/`: Utility classes (File loading, Logging).
- `roms/`: Default ROMs and test files.

## License

MIT License. See `LICENSE` file for details.
