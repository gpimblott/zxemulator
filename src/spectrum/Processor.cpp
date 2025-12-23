/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Processor.h"
#include "../utils/BinaryFileLoader.h"
#include "../utils/Logger.h"
#include "../utils/debug.h"
#include <chrono>
#include <thread>

Processor::Processor() : state(), audio() {
  // Set up the default state of the registers
  reset();
  audio.start();
  m_memory = state.memory.getRawMemory();
}

/**
 * initialise the processor with a ROM file
 */
void Processor::init(const char *romFile) {
  // Load the ROM into memory
  Rom theROM = Rom(romFile);
  if (theROM.getSize() <= 0) {
    utils::Logger::write(
        ("Error: Failed to load ROM file: " + std::string(romFile)).c_str());
    throw std::runtime_error("Failed to load ROM file");
  }
  state.memory.loadIntoMemory(theROM);

  // set up the start point
  state.registers.PC = ROM_LOCATION;
}

void Processor::loadTape(const char *filename) {
  if (state.tape.load(filename)) {
    // Don't play yet. Wait for Basic to boot and type LOAD ""
    autoLoadTape = true;
    frameCounter = 0;
    autoLoadStep = 0;
  }
}

void Processor::loadSnapshot(const char *filename) {
  std::string fn = std::string(filename);
  std::string ext = "";
  if (fn.find_last_of(".") != std::string::npos) {
    ext = fn.substr(fn.find_last_of(".") + 1);
  }

  // Simple lowercase check
  for (auto &c : ext)
    c = tolower(c);

  if (ext == "z80") {
    loadZ80Snapshot(filename);
    return;
  }

  // Default to SNA
  std::string msg = "Loading SNA Snapshot: " + std::string(filename);
  utils::Logger::write(msg.c_str());

  // SNA Header is 27 bytes
  // RAM is 48K (49152 bytes)
  const int SNA_HEADER_SIZE = 27;
  const int SNAPSHOT_RAM_SIZE = 49152;
  const int TOTAL_SIZE = SNA_HEADER_SIZE + SNAPSHOT_RAM_SIZE;

  long fileSize = BinaryFileLoader::getFileSize(filename);
  if (fileSize != TOTAL_SIZE) {
    utils::Logger::write(
        "Error: Snapshot file size incorrect. Only 48K SNA supported.");
    return;
  }

  BinaryFileLoader loader(filename);

  // 1. Registers
  state.registers.I = loader[0];
  state.registers.HL_ = (loader[2] << 8) | loader[1];
  state.registers.DE_ = (loader[4] << 8) | loader[3];
  state.registers.BC_ = (loader[6] << 8) | loader[5];
  state.registers.AF_ = (loader[8] << 8) | loader[7];

  state.registers.L = loader[9];
  state.registers.H = loader[10];
  state.registers.E = loader[11];
  state.registers.D = loader[12];
  state.registers.C = loader[13];
  state.registers.B = loader[14];

  state.registers.IX = (loader[16] << 8) | loader[15];
  state.registers.IY = (loader[18] << 8) | loader[17];

  byte iff2 = (loader[19] & 0x04) >> 2;
  state.setInterrupts(iff2 != 0);

  state.registers.R = loader[20];
  state.registers.F = loader[21];
  state.registers.A = loader[22];

  state.registers.SP = (loader[24] << 8) | loader[23];

  state.setInterruptMode(loader[25]);

  if (state.memory.getVideoBuffer()) {
    state.memory.getVideoBuffer()->setBorderColor(loader[26]);
  }

  // 2. Memory (starts at 16384)
  for (int i = 0; i < SNAPSHOT_RAM_SIZE; ++i) {
    state.memory[16384 + i] = loader[SNA_HEADER_SIZE + i];
  }

  // 3. PC Retrieval (stored on stack)
  // Need to read word at SP, then increment SP
  byte low = state.memory[state.registers.SP];
  byte high = state.memory[state.registers.SP + 1];
  state.registers.PC = (high << 8) | low;
  state.registers.SP += 2;

  utils::Logger::write("SNA Snapshot loaded successfully.");
}

void Processor::loadZ80Snapshot(const char *filename) {
  std::string msg = "Loading Z80 Snapshot: " + std::string(filename);
  utils::Logger::write(msg.c_str());

  long fileSize = BinaryFileLoader::getFileSize(filename);
  BinaryFileLoader loader(filename);

  const int HEADER_SIZE = 30;
  if (fileSize < HEADER_SIZE) {
    utils::Logger::write("Error: Z80 file too small for header.");
    return;
  }

  // Decode Header (bytes 0-29)
  state.registers.A = loader[0];
  state.registers.F = loader[1];
  state.registers.C = loader[2];
  state.registers.B = loader[3];
  state.registers.L = loader[4];
  state.registers.H = loader[5];

  // PC is bytes 6&7. If 0, it means V2/V3
  word pc = (loader[7] << 8) | loader[6];
  bool isVersion2 = (pc == 0);

  state.registers.SP = (loader[9] << 8) | loader[8];
  state.registers.I = loader[10];
  state.registers.R = loader[11];

  byte flags1 = loader[12];
  // Bits 1-3 = border color
  byte border = (flags1 >> 1) & 0x07;
  if (state.memory.getVideoBuffer()) {
    state.memory.getVideoBuffer()->setBorderColor(border);
  }
  // Bit 0 = R register bit 7
  if (flags1 & 1)
    state.registers.R |= 0x80;
  // Bit 5 = Data compressed
  bool compressed = (flags1 & 0x20) != 0;

  state.registers.E = loader[13];
  state.registers.D = loader[14];

  state.registers.BC_ = (loader[16] << 8) | loader[15];
  state.registers.DE_ = (loader[18] << 8) | loader[17];
  state.registers.HL_ = (loader[20] << 8) | loader[19];
  state.registers.AF_ = (loader[21] << 8) | loader[22];

  state.registers.IY = (loader[24] << 8) | loader[23];
  state.registers.IX = (loader[26] << 8) | loader[25];

  state.registers.IFF1 = loader[27];
  state.registers.IFF2 = loader[28];
  state.setInterrupts(state.registers.IFF1 != 0);

  byte flags2 = loader[29];
  // Interrupt mode is bits 0 and 1
  state.setInterruptMode(flags2 & 0x03);

  int dataStart = 30;

  if (isVersion2) {
    // V2 or V3
    // Next 2 bytes are length of extra header
    word extraHeaderLen = (loader[31] << 8) | loader[30];
    dataStart = 32 + extraHeaderLen;

    // PC is in extra header.
    // V2/V3 header offset 32.
    // PC is at offset 32.
    pc = (loader[33] << 8) | loader[32];

    // Check for hardware mode (we only support 48k for now)
    // Byte 34 is Hardware Mode.
    // ... simplistic handling for now ...
    utils::Logger::write(
        "Warning: Z80 V2/V3 file detected. Basic support only.");
  }

  state.registers.PC = pc;

  // Load Memory
  if (isVersion2) {
    // V2/V3: Page-based loading
    int fileIndex = dataStart;
    utils::Logger::write(
        ("Z80 V2/V3 Detected. Data Start: " + std::to_string(dataStart))
            .c_str());

    while (fileIndex < fileSize) {
      // Read Block Header
      if (fileIndex + 3 > fileSize)
        break; // Safety

      word blockLen = (loader[fileIndex + 1] << 8) | loader[fileIndex];
      byte pageId = loader[fileIndex + 2];
      fileIndex += 3;

      if (blockLen == 0)
        break; // End marker? usually just EOF.

      int targetAddress = -1;
      // 48K Page Mapping
      switch (pageId) {
      case 8:
        targetAddress = 16384;
        break; // 0x4000
      case 4:
        targetAddress = 32768;
        break; // 0x8000
      case 5:
        targetAddress = 49152;
        break; // 0xC000
      default:
        // Ignore other pages (128K banks) for 48K emulator
        utils::Logger::write(
            ("Skipping Page " + std::to_string(pageId)).c_str());
        // Skip this block
        fileIndex += (blockLen == 0xFFFF) ? 16384 : blockLen;
        continue;
      }

      bool isCompressed = (blockLen != 0xFFFF);
      long dataEnd = fileIndex + (isCompressed ? blockLen : 16384);

      // Decompress/Copy block
      int currentRam = targetAddress;
      while (fileIndex < dataEnd && currentRam < targetAddress + 16384) {
        byte b = loader[fileIndex];
        if (isCompressed && b == 0xED && fileIndex + 3 < dataEnd &&
            loader[fileIndex + 1] == 0xED) {
          byte count = loader[fileIndex + 2];
          byte val = loader[fileIndex + 3];
          fileIndex += 4;
          for (int k = 0; k < count; k++) {
            if (currentRam < 65536)
              state.memory[currentRam++] = val;
          }
        } else {
          state.memory[currentRam++] = b;
          fileIndex++;
        }
      }
      // Ensure we align to block end if we finished early (shouldn't happen if
      // logic correct)
      fileIndex = dataEnd;
    }
  } else {
    // V1: Linear loading
    utils::Logger::write("Z80 V1 Detected.");
    int ramAddress = 16384; // Start of 48K RAM
    int fileIndex = dataStart;

    bool isCompressed = (flags1 & 0x20) != 0;
    utils::Logger::write(
        ("Computed Compressed: " + std::to_string(isCompressed)).c_str());

    if (isCompressed) {
      while (fileIndex < fileSize && ramAddress < 65536) {
        byte b = loader[fileIndex];

        // Check for ED ED marker
        if (b == 0xED && fileIndex + 3 < fileSize &&
            loader[fileIndex + 1] == 0xED) {
          byte count = loader[fileIndex + 2];
          byte val = loader[fileIndex + 3];
          fileIndex += 4;

          for (int k = 0; k < count; k++) {
            if (ramAddress < 65536) {
              state.memory[ramAddress++] = val;
            }
          }
        } else {
          state.memory[ramAddress++] = b;
          fileIndex++;
        }

        // V1 ends with 00 ED ED 00.
        // However, we process stream until end.
      }
    } else {
      // Uncompressed 48K dump
      for (int i = 0; i < 49152 && (fileIndex + i) < fileSize; i++) {
        state.memory[ramAddress + i] = loader[fileIndex + i];
      }
    }
  }

  utils::Logger::write("Z80 Snapshot loaded successfully.");
}

void Processor::run() {
  running = true;
  while (running) {
    executeFrame();
  }
}

void Processor::executeFrame() {
  // 3.5MHz * 0.02s (50Hz) ~= 69888 T-states per frame
  int tStates = 0;
  const int frameCycles = 69888;

  state.setFrameTStates(0);
  if (state.memory.getVideoBuffer()) {
    state.memory.getVideoBuffer()->newFrame();
  }

  // Fire an interrupt
  if (!paused && state.areInterruptsEnabled()) {
    // If we were halted, we are no longer halted
    if (state.isHalted()) {
      state.setHalted(false);
      // PC already points to next instruction (instruction following HALT)
    }

    // Push PC
    state.registers.SP -= 2;
    word pc = state.registers.PC;
    state.memory[state.registers.SP] = (byte)(pc & 0xFF);
    state.memory[state.registers.SP + 1] = (byte)((pc >> 8) & 0xFF);

    // Interrupt Mode Logic
    int mode = state.getInterruptMode();
    if (mode == 2) {
      // IM 2: Vector form (I << 8) | bus_value. Bus usually 0xFF
      word vector = (state.registers.I << 8) | 0xFF;
      word dest = state.memory.getWord(vector);
      state.registers.PC = dest;
      // Cycles: 19
      tStates += 19;
      state.addFrameTStates(19);
    } else {
      // IM 0/1: RST 38 (0x0038)
      // Note: IM 0 executes instruction on bus. Spectrum bus usually 0xFF (RST
      // 38).
      state.registers.PC = 0x0038;
      // Cycles: 13
      tStates += 13;
      state.addFrameTStates(13);
    }

    // Disable interrupts (standard Z80 behavior on accept)
    state.setInterrupts(false);
  }

  while (tStates < frameCycles && running) {
    if (paused) {
      if (stepRequest) {
        stepRequest = false;
      } else {
        break;
      }
    }

    if (state.isHalted()) {
      // CPU executes NOPs (4 T-states) while halted
      tStates += 4;
      state.addFrameTStates(4);
      this->state.tape.update(4);
      // R register is incremented during NOPs too (M1 cycles)
      state.registers.R =
          (state.registers.R & 0x80) | ((state.registers.R + 1) & 0x7F);
      continue; // Skip fetch/execute
    }

    // Fast Load Trap
    if (state.isFastLoad() && state.registers.PC == 0x0556) {
      // 0x0556 is LD_BYTES.
      // inputs: IX=Dest, DE=Length, A=Flag(00=Header, FF=Data), Carry set=Load
      // outputs: Carry set=Success.

      // We delegate to Tape to see if it can satisfy this request from current
      // block Note: We ignore the 'Verify' case (Carry clear on entry usually
      // means Verify, but ROM routine handles both) We assume Load.
      bool success =
          state.tape.fastLoadBlock(state.registers.A, state.registers.DE,
                                   state.registers.IX, state.memory);

      if (success) {
        // Set Carry
        state.registers.F |= 1;
      } else {
        // Clear Carry
        state.registers.F &= ~1;
      }

      // Execute RET (Pop PC)
      byte low = state.memory[state.registers.SP];
      byte high = state.memory[state.registers.SP + 1];
      state.registers.PC = (high << 8) | low;
      state.registers.SP += 2;

      // Don't execute instruction at 0x0556
      continue;
    }

    // Fetch opcode
    byte opcode = m_memory[state.registers.PC];

    // Increment Refresh Register (Lower 7 bits) - happens on M1 cycle
    state.registers.R =
        (state.registers.R & 0x80) | ((state.registers.R + 1) & 0x7F);

    // Increment PC past opcode
    state.registers.PC++;

    int cycles = 0;
    bool handled = true;

    // Hybrid dispatch: switch for common opcodes, catalogue for others
    switch (opcode) {
    case 0x00: // NOP
      cycles = 4;
      break;

    case 0x76: // HALT
      state.setHalted(true);
      cycles = 4;
      break;

    case 0x3E: { // LD A, n
      byte value = m_memory[state.registers.PC++];
      state.registers.A = value;
      cycles = 7;
      break;
    }

    case 0x06: { // LD B, n
      byte value = m_memory[state.registers.PC++];
      state.registers.B = value;
      cycles = 7;
      break;
    }

    case 0x0E: { // LD C, n
      byte value = m_memory[state.registers.PC++];
      state.registers.C = value;
      cycles = 7;
      break;
    }

    case 0x16: { // LD D, n
      byte value = m_memory[state.registers.PC++];
      state.registers.D = value;
      cycles = 7;
      break;
    }

    case 0x1E: { // LD E, n
      byte value = m_memory[state.registers.PC++];
      state.registers.E = value;
      cycles = 7;
      break;
    }

    case 0x26: { // LD H, n
      byte value = m_memory[state.registers.PC++];
      state.registers.H = value;
      cycles = 7;
      break;
    }

    case 0x2E: { // LD L, n
      byte value = m_memory[state.registers.PC++];
      state.registers.L = value;
      cycles = 7;
      break;
    }

    case 0xC3: { // JP nn
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC = address;
      cycles = 10;
      break;
    }

    case 0xC9: { // RET
      byte low = m_memory[state.registers.SP];
      byte high = m_memory[state.registers.SP + 1];
      state.registers.PC = (high << 8) | low;
      state.registers.SP += 2;
      cycles = 10;
      break;
    }

    case 0xCD: { // CALL nn
      word address = m_memory[state.registers.PC] |
                     (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      // Push PC
      state.registers.SP -= 2;
      word pc = state.registers.PC;
      m_memory[state.registers.SP] = (byte)(pc & 0xFF);
      m_memory[state.registers.SP + 1] = (byte)((pc >> 8) & 0xFF);
      state.registers.PC = address;
      cycles = 17;
      break;
    }

    case 0xF3: // DI
      state.setInterrupts(false);
      cycles = 4;
      break;

    case 0xFB: // EI
      state.setInterrupts(true);
      cycles = 4;
      break;

    // LD r, r' instructions (0x40-0x7F range, very common)
    // LD B, r
    case 0x40:
      cycles = 4;
      break; // LD B, B (NOP-like)
    case 0x41:
      state.registers.B = state.registers.C;
      cycles = 4;
      break;
    case 0x42:
      state.registers.B = state.registers.D;
      cycles = 4;
      break;
    case 0x43:
      state.registers.B = state.registers.E;
      cycles = 4;
      break;
    case 0x44:
      state.registers.B = state.registers.H;
      cycles = 4;
      break;
    case 0x45:
      state.registers.B = state.registers.L;
      cycles = 4;
      break;
    case 0x46:
      state.registers.B = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD B, (HL)
    case 0x47:
      state.registers.B = state.registers.A;
      cycles = 4;
      break;

    // LD C, r
    case 0x48:
      state.registers.C = state.registers.B;
      cycles = 4;
      break;
    case 0x49:
      cycles = 4;
      break; // LD C, C
    case 0x4A:
      state.registers.C = state.registers.D;
      cycles = 4;
      break;
    case 0x4B:
      state.registers.C = state.registers.E;
      cycles = 4;
      break;
    case 0x4C:
      state.registers.C = state.registers.H;
      cycles = 4;
      break;
    case 0x4D:
      state.registers.C = state.registers.L;
      cycles = 4;
      break;
    case 0x4E:
      state.registers.C = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD C, (HL)
    case 0x4F:
      state.registers.C = state.registers.A;
      cycles = 4;
      break;

    // LD D, r
    case 0x50:
      state.registers.D = state.registers.B;
      cycles = 4;
      break;
    case 0x51:
      state.registers.D = state.registers.C;
      cycles = 4;
      break;
    case 0x52:
      cycles = 4;
      break; // LD D, D
    case 0x53:
      state.registers.D = state.registers.E;
      cycles = 4;
      break;
    case 0x54:
      state.registers.D = state.registers.H;
      cycles = 4;
      break;
    case 0x55:
      state.registers.D = state.registers.L;
      cycles = 4;
      break;
    case 0x56:
      state.registers.D = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD D, (HL)
    case 0x57:
      state.registers.D = state.registers.A;
      cycles = 4;
      break;

    // LD E, r
    case 0x58:
      state.registers.E = state.registers.B;
      cycles = 4;
      break;
    case 0x59:
      state.registers.E = state.registers.C;
      cycles = 4;
      break;
    case 0x5A:
      state.registers.E = state.registers.D;
      cycles = 4;
      break;
    case 0x5B:
      cycles = 4;
      break; // LD E, E
    case 0x5C:
      state.registers.E = state.registers.H;
      cycles = 4;
      break;
    case 0x5D:
      state.registers.E = state.registers.L;
      cycles = 4;
      break;
    case 0x5E:
      state.registers.E = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD E, (HL)
    case 0x5F:
      state.registers.E = state.registers.A;
      cycles = 4;
      break;

    // LD H, r
    case 0x60:
      state.registers.H = state.registers.B;
      cycles = 4;
      break;
    case 0x61:
      state.registers.H = state.registers.C;
      cycles = 4;
      break;
    case 0x62:
      state.registers.H = state.registers.D;
      cycles = 4;
      break;
    case 0x63:
      state.registers.H = state.registers.E;
      cycles = 4;
      break;
    case 0x64:
      cycles = 4;
      break; // LD H, H
    case 0x65:
      state.registers.H = state.registers.L;
      cycles = 4;
      break;
    case 0x66:
      state.registers.H = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD H, (HL)
    case 0x67:
      state.registers.H = state.registers.A;
      cycles = 4;
      break;

    // LD L, r
    case 0x68:
      state.registers.L = state.registers.B;
      cycles = 4;
      break;
    case 0x69:
      state.registers.L = state.registers.C;
      cycles = 4;
      break;
    case 0x6A:
      state.registers.L = state.registers.D;
      cycles = 4;
      break;
    case 0x6B:
      state.registers.L = state.registers.E;
      cycles = 4;
      break;
    case 0x6C:
      state.registers.L = state.registers.H;
      cycles = 4;
      break;
    case 0x6D:
      cycles = 4;
      break; // LD L, L
    case 0x6E:
      state.registers.L = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD L, (HL)
    case 0x6F:
      state.registers.L = state.registers.A;
      cycles = 4;
      break;

    // LD (HL), r
    case 0x70:
      m_memory[state.registers.HL] = state.registers.B;
      cycles = 7;
      break;
    case 0x71:
      m_memory[state.registers.HL] = state.registers.C;
      cycles = 7;
      break;
    case 0x72:
      m_memory[state.registers.HL] = state.registers.D;
      cycles = 7;
      break;
    case 0x73:
      m_memory[state.registers.HL] = state.registers.E;
      cycles = 7;
      break;
    case 0x74:
      m_memory[state.registers.HL] = state.registers.H;
      cycles = 7;
      break;
    case 0x75:
      m_memory[state.registers.HL] = state.registers.L;
      cycles = 7;
      break;
    // 0x76 is HALT (already handled above)
    case 0x77:
      m_memory[state.registers.HL] = state.registers.A;
      cycles = 7;
      break;

    // LD A, r
    case 0x78:
      state.registers.A = state.registers.B;
      cycles = 4;
      break;
    case 0x79:
      state.registers.A = state.registers.C;
      cycles = 4;
      break;
    case 0x7A:
      state.registers.A = state.registers.D;
      cycles = 4;
      break;
    case 0x7B:
      state.registers.A = state.registers.E;
      cycles = 4;
      break;
    case 0x7C:
      state.registers.A = state.registers.H;
      cycles = 4;
      break;
    case 0x7D:
      state.registers.A = state.registers.L;
      cycles = 4;
      break;
    case 0x7E:
      state.registers.A = m_memory[state.registers.HL];
      cycles = 7;
      break; // LD A, (HL)
    case 0x7F:
      cycles = 4;
      break; // LD A, A

    // LD (BC), A and LD A, (BC)
    case 0x02:
      m_memory[state.registers.BC] = state.registers.A;
      cycles = 7;
      break;
    case 0x0A:
      state.registers.A = m_memory[state.registers.BC];
      cycles = 7;
      break;

    // LD (DE), A and LD A, (DE)
    case 0x12:
      m_memory[state.registers.DE] = state.registers.A;
      cycles = 7;
      break;
    case 0x1A:
      state.registers.A = m_memory[state.registers.DE];
      cycles = 7;
      break;

    // 16-bit loads with immediates
    case 0x01: { // LD BC, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.BC = value;
      cycles = 10;
      break;
    }

    case 0x11: { // LD DE, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.DE = value;
      cycles = 10;
      break;
    }

    case 0x21: { // LD HL, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.HL = value;
      cycles = 10;
      break;
    }

    case 0x31: { // LD SP, nn
      word value = m_memory[state.registers.PC] |
                   (m_memory[state.registers.PC + 1] << 8);
      state.registers.PC += 2;
      state.registers.SP = value;
      cycles = 10;
      break;
    }

    case 0x36: { // LD (HL), n
      byte value = m_memory[state.registers.PC++];
      m_memory[state.registers.HL] = value;
      cycles = 10;
      break;
    }

    default:
      // Fall back to catalogue for opcodes not yet migrated
      handled = false;
      break;
    }

    if (handled) {
      tStates += cycles;
      state.addFrameTStates(cycles);
      this->state.tape.update(cycles);
      audio.update(cycles, state.getSpeakerBit(), state.tape.getEarBit());
    } else {
      // Fallback to old catalogue system
      state.registers.PC--; // Rewind PC for catalogue lookup
      OpCode *opCode = getNextInstruction();
      if (opCode != nullptr) {
        // increment past the opcode
        this->state.registers.PC++;

        int cycles = opCode->execute(this->state);
        tStates += cycles;
        state.addFrameTStates(cycles);
        this->state.tape.update(cycles);
        audio.update(cycles, state.getSpeakerBit(), state.tape.getEarBit());

      } else {
        byte unknownOpcode = state.memory[state.registers.PC];
        char errorMsg[100];
        snprintf(errorMsg, sizeof(errorMsg),
                 "Unknown opcode %02X at address %d", unknownOpcode,
                 this->state.registers.PC);
        lastError = std::string(errorMsg);

        debug("Unknown opcode %02X at address %d\n", unknownOpcode,
              this->state.registers.PC);
        running = false;
      }
    }
  }
  audio.flush();

  // Audio Sync: Throttle execution to match audio consumption rate
  // If buffer has > 3 frames of audio (approx 60ms), slow down.
  // This locks emulation speed to the audio card clock (44.1kHz).
  while (audio.getBufferSize() > 2646) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // Auto-Type Logic (Frame based)
  if (autoLoadTape && running && !paused) {
    frameCounter++;
    // Wait 120 frames (~2.4s) for boot
    if (frameCounter > 120) {
      // Steps:
      // 0: Start J
      // 1: Release J
      // 2: Start Sym+P
      // 3: Release P
      // 4: Start Sym+P
      // 5: Release P
      // 6: Release Sym
      // 7: Start Enter
      // 8: Release Enter
      // 9: Play Tape + End

      keyHoldFrames++;
      const int PRESS_DURATION = 5;
      const int GAP_DURATION = 5;

      switch (autoLoadStep) {
      case 0:                              // J
        state.keyboard.setKey(6, 3, true); // J
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 1: // Release J
        state.keyboard.setKey(6, 3, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 2:                              // Sym+P
        state.keyboard.setKey(7, 1, true); // Sym
        state.keyboard.setKey(5, 0, true); // P
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 3: // Release P (keep Sym)
        state.keyboard.setKey(5, 0, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 4:                              // Sym+P (again for second quote)
        state.keyboard.setKey(5, 0, true); // P
                                           // Sym already held
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 5: // Release P
        state.keyboard.setKey(5, 0, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 6: // Release Sym
        state.keyboard.setKey(7, 1, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 7: // Enter
        state.keyboard.setKey(6, 0, true);
        if (keyHoldFrames > PRESS_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 8: // Release Enter
        state.keyboard.setKey(6, 0, false);
        if (keyHoldFrames > GAP_DURATION) {
          autoLoadStep++;
          keyHoldFrames = 0;
        }
        break;
      case 9: // Play
        state.tape.play();
        autoLoadTape = false;
        break;
      }
    }
  }
}

VideoBuffer *Processor::getVideoBuffer() {
  return state.memory.getVideoBuffer();
}

void Processor::shutdown() {}

void Processor::reset() {
  state.registers.PC = 0x0;
  state.registers.AF = 0xFFFF;
  state.registers.SP = 0xFFFF;
  state.registers.BC = 0;
  state.registers.DE = 0;
  state.registers.HL = 0;
  state.registers.IX = 0;
  state.registers.IY = 0;
  state.registers.I = 0;
  state.registers.R = 0;
  state.setHalted(false);
  state.setInterrupts(false);
  state.setInterruptMode(0); // Reset to IM 0
  lastError = "";
  running = true;
  paused = false;
  audio.reset();
}

/**
 * Read the next instruction and process it
 * @return
 */
OpCode *Processor::getNextInstruction() {

  byte opcode = state.memory[state.registers.PC];
  return catalogue.lookupOpcode(opcode);
}

// ============================================================================
// Opcode Helper Methods (Placeholders)
// ============================================================================

void Processor::op_load(byte opcode) {
  // To be implemented
}

void Processor::op_arithmetic(byte opcode) {
  // To be implemented
}

void Processor::op_logic(byte opcode) {
  // To be implemented
}

void Processor::op_rotate_shift(byte opcode) {
  // To be implemented
}

void Processor::op_bit(byte opcode) {
  // To be implemented
}

void Processor::op_jump(byte opcode) {
  // To be implemented
}

void Processor::op_stack(byte opcode) {
  // To be implemented
}

void Processor::op_io(byte opcode) {
  // To be implemented
}

void Processor::op_misc(byte opcode) {
  // To be implemented
}

void Processor::exec_ed_opcode() {
  // To be implemented
}

void Processor::exec_cb_opcode() {
  // To be implemented
}

void Processor::exec_index_opcode(byte prefix) {
  // To be implemented
}
