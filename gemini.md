# ZX Spectrum Emulator Project Status

This project is a C++ implementation of a ZX Spectrum emulator using SFML for the presentation layer. It is currently in a **very early skeletal stage**.

## Current Implementation State
- **Architecture**: A modular design separates the `Processor` (CPU), `Memory`, and `Screen`.
- **Build System**: Uses CMake and supports building with system-installed SFML 3 (macOS/Homebrew compatible).
- **Video**: A `WindowsScreen` class implements basic video output using SFML, scaling the 256x192 Spectrum resolution.
- **CPU**: The Z80 processor implementation is minimal. It has a run loop but lacks almost all instruction logic.

## Missing Components & Roadmap

### 1. Z80 Instruction Set (Critical)
The CPU is missing the vast majority of the Z80 instruction set. Only a handful of opcodes are currently implemented for testing purposes:
- **Implemented**: `LD DE,nn`, `LD B,A`, `LD A,n`, `XOR A`, and basic jumps/out stubs.
- **Missing**:
    - Arithmetic (ADD, SUB, ADC, SBC, INC, DEC)
    - Logic (AND, OR, CP)
    - Stack Operations (PUSH, POP, CALL, RET)
    - Bit Manipulation (SET, RES, BIT)
    - Rotates & Shifts (RLC, RRC, SLA, SRA, etc.)
    - Block Operations (LDIR, CPIR, etc.)
    - Exchange & Alternate Registers (EX, EXX)

### 2. Interrupt Handling
- There is no implementation for generating vertical blank interrupt requests (50Hz interrupt) which is essential for Spectrum timing and keyboard scanning.
- The `IM` (Interrupt Mode) instructions and handling logic are missing.

### 3. I/O & Keyboard
- **Input**: There is no mapping from host keyboard events to the Spectrum's I/O ports (0xFE). You cannot interact with the emulator.
- **Ports**: The `IN` and `OUT` instruction logic needs to route requests to appropriate devices (ULA (screen/border/ear), Keyboard, Kempston Joystick).

### 4. Timing & Synchronization
- The `Processor` run loop executes as fast as possible with no T-state accounting.
- **Needed**: A scheduler to limit execution to ~3.5MHz (approx 69,888 T-states per frame) and synchronize the screen update (50fps).

### 5. Memory Contention
- The generated code does not currently simulate memory contention for the lower 16KB of RAM, which affects timing on real hardware.

### 6. Media Support
- **Loading**: Can load a raw 16KB ROM file.
- **Missing**: Support for loading tape files (.TAP, .TZX) or snapshots (.Z80, .SNA).

### 7. Audio
- No audio implementation exists. Needs to emulate the beeper via the ULA output port.

## Conclusion
To complete the emulator, the immediate priority is fleshing out the **Z80 Instruction Set** to allow the ROM to boot properly. Once the ROM can enter its main loop and scan the keyboard, focus should shift to **Input Mapping** and **Accurate Timing**.
