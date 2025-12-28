/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SnapshotLoader.h"
#include "../utils/BinaryFileLoader.h"
#include "../utils/Logger.h"
#include <string>

void SnapshotLoader::load(const char *filename, ProcessorState &state) {
  std::string fn = std::string(filename);
  std::string ext = "";
  if (fn.find_last_of(".") != std::string::npos) {
    ext = fn.substr(fn.find_last_of(".") + 1);
  }

  // Simple lowercase check
  for (auto &c : ext)
    c = tolower(c);

  if (ext == "z80") {
    loadZ80(filename, state);
  } else {
    // Default to SNA
    loadSNA(filename, state);
  }
}

void SnapshotLoader::loadSNA(const char *filename, ProcessorState &state) {
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

void SnapshotLoader::loadZ80(const char *filename, ProcessorState &state) {
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
