# Code Review & Performance Analysis

## Executive Summary
The codebase is clean and readable, with a strong object-oriented approach. However, this OO structure introduces significant overhead for a system emulator (Z80), where every cycle counts. The "Threaded Interpreter" pattern using C++ virtual-like dispatch (via function pointers) and heavy memory access encapsulation are the primary performance bottlenecks.

## Performance Improvements

### 1. Opcode Dispatch (High Impact)
**Current state**: 
The emulator uses a `catalogue` to lookup `OpCode` objects, then calls `execute()`, which calls a function pointer.
```cpp
OpCode *opCode = catalogue.lookupOpcode(opcode); // Array lookup
opCode->execute(state); // Member function -> Function pointer call
```
**Problem**: 
- Indirect calls cause pipeline flushes and cache misses.
- `OpCode` objects scatter code across many files, preventing compiler optimizations (inlining).
- Fetching the object and then calling checking for `nullptr` adds latency per instruction.

**Suggestion**: 
Replace the `OpCode` classes and `Catalogue` with a monolithic `switch` statement within `Processor::executeFrame`.
```cpp
switch(opcode) {
    case 0x00: // NOP
        break; 
    case 0x76: // HALT
        state.setHalted(true);
        break;
    // ...
}
```
This allows the compiler to generate a jump table and inline significantly more logic.

### 2. Memory Access (High Impact)
**Current State**:
`Memory::operator[]` contains multiple branches and exception throwing checks.
```cpp
if (i >= m_totalMemory) throw MemoryException(i); // Exception overhead
if (i < ROM_SIZE) { ... } // Branching
```
**Problem**: 
This function is called millions of times per second. The bounds check is largely redundant for valid emulation code (PC fetch, Stack operations).

**Suggestion**: 
- Implement `fastReadByte(address)` and `fastWriteByte(address)` that assume valid 16-bit addresses (which is guaranteed by `word` type masking).
- Give `Processor` direct access to `byte* m_memory` pointers for RAM and ROM to bypass the function call overhead completely for instruction fetching.

### 3. Audio Updates (Medium Impact)
**Current State**:
`audio.update(...)` is called **every instruction**.
**Problem**: 
Audio sampling does not need to happen at 3.5MHz.
**Suggestion**: 
Update audio once per scanline or batch updates every N cycles.

### 4. Memory::getWord (Correctness/Performance)
**Current State**:
```cpp
word *ptr = reinterpret_cast<word *>(m_memory + address);
return *(ptr);
```
**Problem**: 
- **UB**: Accessing unrelated types via reinterpret_cast violates strict aliasing.
- **Alignment**: If `address` is odd, this is undefined behavior on many platforms (and crashes on ARM/some server archs).
- **Endianness**: Relying on host endianness works on x86/ARM (LE) but is not portable.
**Suggestion**: 
Implement explicitly: `return m_memory[address] | (m_memory[address+1] << 8);`

## Structural Improvements

### 1. Simplify Opcode Handling
**Observation**: 
The `OpCodeProvider`, `OpCodeCatalogue` infrastructure is over-engineered. The Z80 instruction set is fixed; it does not need dynamic discovery.
**Suggestion**: 
Remove `OpCodeCatalogue`, `OpCodeProvider`, and individual `OpCode` classes. Move logic into `Processor.cpp` (or `CpuCore.cpp`) as helper functions called by the main switch.

### 2. Separation of Concerns
**Observation**: 
`Processor.cpp` handles:
- CPU Emulation
- Tape Loading
- Snapshot Loading (SNA/Z80)
- Audio Sync
- Auto-typing
**Suggestion**: 
- Extract `SnapshotLoader` into a utility class.
- Extract `TapeLoader` logic (already partially in `Tape.cpp`) but ensure `Processor` simply accepts a `Tape` object.

### 3. Fix Missing Instructions
**Observation**: 
`IndexOpcodes.cpp` has a `debug("Unknown Index CB Rotate Opcode...")` for opcodes `0x00-0x3F`.
**Suggestion**: 
Implement the missing Index CB rotate/shift instructions.

### 4. Remove Dangerous Exits
**Observation**: 
`VideoBuffer::operator[]` calls `exit(0)` on bounds error.
**Suggestion**: 
Log and ignore, or throw an exception that can be caught by the UI/Main loop to pause emulation, rather than killing the process.

## Next Steps
If you plan to implement these changes, I recommend:
1.  **Refactor Memory**: Simplify access patterns.
2.  **Refactor CPU Loop**: Move to a switch-based dispatcher.
3.  **Extract Loaders**: Clean up `Processor.cpp`.
