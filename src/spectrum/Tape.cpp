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

#include "Tape.h"
#include "../utils/Logger.h"

using namespace utils;
using namespace emulator_types;

// TZX Pulse Timing Constants (T-states)
// Standard Speed Data Block
const int PILOT_PULSE = 2168;
const int SYNC1_PULSE = 667;
const int SYNC2_PULSE = 735;
const int BIT0_PULSE = 855;
const int BIT1_PULSE = 1710;
const int PILOT_HEADER_COUNT = 8063; // ~5 seconds? No, pulses.
const int PILOT_DATA_COUNT = 3223;

// Internal helpers

Tape::Tape() {}

void Tape::play() {
  if (!blocks.empty()) {
    playing = true;
    currentBlockIndex = 0;
    // Start Pilot
    currentState = PILOT;
    tStateCounter = 0;
    nextEdgeTState = PILOT_PULSE;
    pulseCount = 0;
    earBit = false;
    Logger::write("Tape playing...");
  }
}

void Tape::stop() {
  playing = false;
  earBit = false;
  Logger::write("Tape stopped.");
}

bool Tape::getEarBit() { return earBit; }

void Tape::update(int tStates) {
  if (!playing)
    return;

  tStateCounter += tStates;

  if (tStateCounter >= nextEdgeTState) {
    tStateCounter = 0; // Relative timing
    if (currentState != PAUSE) {
      earBit = !earBit; // Toggle pulse
    }

    switch (currentState) {
    case PILOT:
      // Pulse count for pilot
      pulseCount++;
      // Check if pilot finished
      // How many pulses? Depends on flag byte.
      // We need to look ahead at data[0] to determine pilot length if not set?
      // Standard: 8063 if header (flag < 128), 3223 if data (flag >= 128)
      {
        if (currentBlockIndex >= blocks.size()) {
          stop();
          return;
        }

        int pilotLength = PILOT_HEADER_COUNT;
        if (blocks[currentBlockIndex].data.size() > 0) {
          byte flag = blocks[currentBlockIndex].data[0];
          if (flag >= 128)
            pilotLength = PILOT_DATA_COUNT;
        }

        if (pulseCount >= pilotLength) {
          currentState = SYNC1;
          nextEdgeTState = SYNC1_PULSE;
        } else {
          nextEdgeTState = PILOT_PULSE;
        }
      }
      break;

    case SYNC1:
      currentState = SYNC2;
      nextEdgeTState = SYNC2_PULSE;
      break;

    case SYNC2:
      currentState = DATA;
      currentByteIndex = 0;
      currentBitIndex = 0;
      pulseCount = 0;
      // TZX Spec: "Each byte is sent MSB first"
      nextEdgeTState = 0; // Trigger data logic immediately on next loop?

      // actually we are at the edge of SYNC2 ending.
      // We need to set the duration for the FIRST pulse of the FIRST bit.
      {
        if (blocks[currentBlockIndex].data.size() > 0) {
          byte b = blocks[currentBlockIndex].data[0];
          bool bit = (b & (1 << 7)) != 0; // MSB (Bit 7) first
          nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
        } else {
          // Empty block?
          currentState = PAUSE;
          nextEdgeTState = 3500;
        }
      }
      break;

    case DATA:
      // We just finished a pulse of a bit.
      // Was it the first or second pulse?
      // We can track this with pulseCount (reset at start of data? or use bit
      // phase) Let's rely on even/odd pulse count? No, messy if pilot was odd.
      // Use a bool isSecondPulse?
      // Let's add isSecondPulse to class state? Or static?
      // Static is dangerous if multiple instances.
      // Hack: use pulseCount as "Bit Pulse Index" (0 or 1)

      // If we change state to DATA, set pulseCount = 0.
      if (pulseCount == 0) {
        // Finished first pulse. Queue second.
        pulseCount = 1;
        // Length is same as first.
        // We look at CURRENT bit again.
        byte b = blocks[currentBlockIndex].data[currentByteIndex];
        bool bit = (b & (1 << (7 - currentBitIndex))) != 0;
        nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
      } else {
        // Finished second pulse of bit. Move to next bit.
        pulseCount = 0;
        currentBitIndex++;
        if (currentBitIndex > 7) {
          currentBitIndex = 0;
          currentByteIndex++;
          if (currentByteIndex >= blocks[currentBlockIndex].data.size()) {
            // End of block
            currentState = PAUSE;
            // Pause is in ms.
            int pauseMs = blocks[currentBlockIndex].pauseAfter;
            long pauseT = pauseMs * 3500L;
            if (pauseT < 3500)
              pauseT = 3500; // Min 1ms
            nextEdgeTState = pauseT;
          } else {
            // Next Byte, First Pulse (Bit 7)
            byte b = blocks[currentBlockIndex].data[currentByteIndex];
            bool bit = (b & (1 << 7)) != 0;
            nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
          }
        } else {
          // Next Bit, First Pulse
          byte b = blocks[currentBlockIndex].data[currentByteIndex];
          bool bit = (b & (1 << (7 - currentBitIndex))) != 0;
          nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
        }
      }
      break;

    case PAUSE:
      // Pause finished. Move to next block.
      currentBlockIndex++;
      if (currentBlockIndex < blocks.size()) {
        currentState = PILOT;
        pulseCount = 0;
        nextEdgeTState = PILOT_PULSE;
      } else {
        stop();
      }
      break;

    default:
      break;
    }
  }
} // End update

#include "../spectrum/Memory.h"

// ... existing code ...

bool Tape::fastLoadBlock(byte expectedFlag, word length, word startAddress,
                         Memory &memory) {
  if (!playing && !blocks.empty()) {
    // If not playing but we have blocks (e.g. started via -f), ensure we are
    // ready. Or just leverage currentBlockIndex. Let's assume user calls
    // play() or we manually manage index.
  }

  // Look for next data block
  size_t scanIndex = currentBlockIndex;

  while (scanIndex < blocks.size()) {
    // Accept both standard (0x10) and turbo (0x11) speed blocks
    if (blocks[scanIndex].id == 0x10 || blocks[scanIndex].id == 0x11) {
      // Check Flag
      if (blocks[scanIndex].data.size() > 0 &&
          blocks[scanIndex].data[0] == expectedFlag) {
        // Found match!
        // Verify size. TZX Block Data includes Flag + Data + Checksum.
        // We need to load 'length' bytes.
        // The block size should be at least length + 2.
        // Actually ROM expects 'length' bytes. plus Flag (1) and Checksum
        // (1). So total block size must be >= length + 2.

        if (blocks[scanIndex].data.size() < (size_t)(length + 2)) {
          // Error: Block too short?
          Logger::write("Block too short for fast load");
          return false;
        }

        // Debug Logging
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "FastLoad: Flag=%02X Exp=%02X Len=%d IX=%04X Data[0]=%02X "
                 "Data[1]=%02X",
                 blocks[scanIndex].data[0], expectedFlag, length, startAddress,
                 blocks[scanIndex].data[0], blocks[scanIndex].data[1]);
        Logger::write(msg);

        // Load Data
        // Skip Flag (index 0)
        // Copy length bytes to memory
        // rom routine loads to IX.
        const std::vector<byte> &data = blocks[scanIndex].data;
        for (size_t i = 0; i < length; i++) {
          if (i + 1 < data.size())
            memory[(startAddress + i) & 0xFFFF] = data[i + 1];
        }

        // Advance Tape
        currentBlockIndex = scanIndex + 1;

        // Always stop after fast load - don't restart playback
        // The tape pointer advances but audio doesn't play
        stop();

        return true;
      } else {
        // Flag mismatch. (e.g. found Header when looking for Data).
        // Should we skip it? No, if we expect Data but find Header, strict
        // trapping says Fail. The ROM would read the flag, see mismatch, and
        // fail (or retry). However, for Fast Load, if we are desynced, maybe
        // we search? Standard behavior: Return false. The ROM will then fail
        // (Carry Clear) and maybe retry? Or we just return false and let the
        // ROM execute normally (and likely fail if tape not playing). But if
        // we are -fast-loading, we want to force it? If we find a Header (00)
        // and we wanted Data (FF), it's a fail. BUT, if we have Block 0x30
        // (Text), we should skip it. My loop below skips ID != 0x10.

        // If we found 0x10 block and flag doesn't match...
        // Maybe we should just return false.
        return false;
      }
    }
    // Skip non-data blocks (Text, etc)
    scanIndex++;
  }

  return false;
}
