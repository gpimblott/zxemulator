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

void Tape::startNextBlock() {
  bool blockFound = false;

  while (!blockFound && playing) {
    if (currentBlockIndex >= blocks.size()) {
      stop();
      return;
    }

    const auto &block = blocks[currentBlockIndex];
    pulseCount = 0;
    currentByteIndex = 0;
    currentBitIndex = 0;

    switch (block.type) {
    case utils::STANDARD_SPEED_DATA:
    case utils::TURBO_SPEED_DATA:
      currentState = PILOT;
      {
        int pilotLen = PILOT_HEADER_COUNT;
        if (block.type == utils::STANDARD_SPEED_DATA) {
          // Standard heuristic
          if (block.data.size() > 0 && block.data[0] >= 128) {
            pilotLen = PILOT_DATA_COUNT;
          }
          nextEdgeTState = PILOT_PULSE;
        } else {
          // Turbo
          pilotLen = block.pilotToneLen;
          nextEdgeTState = block.pilotPulseLen;
        }
      }
      blockFound = true; // We can play this
      break;

    case utils::PURE_TONE:
      currentState = TONE;
      nextEdgeTState = block.pulseLength;
      blockFound = true;
      break;

    case utils::PULSE_SEQUENCE:
      currentState = PULSE_SEQUENCE;
      if (!block.pulseSequence.empty()) {
        nextEdgeTState = block.pulseSequence[0];
        currentByteIndex = 1;
        blockFound = true;
      } else {
        currentBlockIndex++;
        // Continue loop
      }
      break;

    case utils::PAUSE:
      currentState = PAUSE;
      if (block.pauseAfter == 0) {
        // override: Treat 0 as 2 second pause to auto-resume
        long pauseT = 2000 * 3500L;
        nextEdgeTState = pauseT;
        blockFound = true;
      } else {
        long pauseT = block.pauseAfter * 3500L;
        if (pauseT < 3500)
          pauseT = 3500;
        nextEdgeTState = pauseT;
        blockFound = true;
      }
      break;

    case utils::LOOP_START: {
      LoopState ls;
      ls.startBlockIndex = currentBlockIndex + 1;
      ls.iterationsRemaining = block.pulseCount;
      loopStack.push(ls);
      currentBlockIndex++;
      // Continue loop
    } break;

    case utils::LOOP_END:
      if (!loopStack.empty()) {
        LoopState &ls = loopStack.top();
        ls.iterationsRemaining--;
        if (ls.iterationsRemaining > 0) {
          currentBlockIndex = ls.startBlockIndex;
          // Continue loop
        } else {
          loopStack.pop();
          currentBlockIndex++;
          // Continue loop
        }
      } else {
        currentBlockIndex++;
      }
      break;

    case utils::GROUP_START:
    case utils::GROUP_END:
      currentBlockIndex++;
      break;

    default:
      currentBlockIndex++;
      break;
    }
  }
}

void Tape::play() {
  if (!blocks.empty()) {
    playing = true;
    tStateCounter = 0;
    earBit = true;
    // Start with a small lead-in silence (1000ms)
    currentState = LEAD_IN;
    nextEdgeTState = 1000 * 3500;
  }
}

void Tape::stop() {
  playing = false;
  earBit = true;
}

bool Tape::getEarBit() { return earBit; }

void Tape::update(int tStates) {
  if (!playing)
    return;

  tStateCounter += tStates;

  if (tStateCounter >= nextEdgeTState) {
    tStateCounter -= nextEdgeTState; // Relative timing with drift correction
    if (currentState != PAUSE && currentState != LEAD_IN) {
      earBit = !earBit; // Toggle pulse
    }

    switch (currentState) {
    case PILOT:
      // Pulse count for pilot
      pulseCount++;
      // Check if pilot finished
      {
        int target = PILOT_HEADER_COUNT;

        // Use block-specific pilot tone length if available (Turbo/Standard)
        if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
          target = blocks[currentBlockIndex].pilotToneLen;
        } else {
          // Standard heuristic
          if (blocks[currentBlockIndex].data.size() > 0 &&
              blocks[currentBlockIndex].data[0] >= 128) {
            target = PILOT_DATA_COUNT;
          }
        }

        if (pulseCount >= target) {
          currentState = SYNC1;
          // Sync 1
          if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
            nextEdgeTState = blocks[currentBlockIndex].sync1Len;
          } else {
            nextEdgeTState = SYNC1_PULSE;
          }
        } else {
          // Pilot Pulse
          if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
            nextEdgeTState = blocks[currentBlockIndex].pilotPulseLen;
          } else {
            nextEdgeTState = PILOT_PULSE;
          }
        }
      }
      break;

    case SYNC1:
      currentState = SYNC2;
      // Sync 2
      if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
        nextEdgeTState = blocks[currentBlockIndex].sync2Len;
      } else {
        nextEdgeTState = SYNC2_PULSE;
      }
      break;

    case SYNC2:
      currentState = DATA;
      currentByteIndex = 0;
      currentBitIndex = 0;
      pulseCount = 0;
      nextEdgeTState = 0; // Trigger immediately

      // Setup first bit pulse
      if (blocks[currentBlockIndex].data.size() > 0) {
        byte b = blocks[currentBlockIndex].data[0];
        bool bit = (b & (1 << 7)) != 0;

        if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
          nextEdgeTState = bit ? blocks[currentBlockIndex].oneLen
                               : blocks[currentBlockIndex].zeroLen;
        } else {
          nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
        }
      } else {
        // Empty data block? Go to pause.
        if (blocks[currentBlockIndex].pauseAfter > 0) {
          currentState = PAUSE;
          nextEdgeTState = blocks[currentBlockIndex].pauseAfter * 3500L;
        } else {
          currentBlockIndex++;
          startNextBlock();
        }
      }
      break;

    case DATA:
      if (pulseCount == 0) {
        // First pulse done, queue second
        pulseCount = 1;
        // Same length as first
        byte b = blocks[currentBlockIndex].data[currentByteIndex];
        bool bit = (b & (1 << (7 - currentBitIndex))) != 0;

        if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
          nextEdgeTState = bit ? blocks[currentBlockIndex].oneLen
                               : blocks[currentBlockIndex].zeroLen;
        } else {
          nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
        }
      } else {
        // Second pulse done. Next bit.
        pulseCount = 0;
        currentBitIndex++;

        // Handle last byte used bits (Turbo-specific, usually 8)
        int bitsInByte = 8;
        if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA &&
            currentByteIndex == blocks[currentBlockIndex].data.size() - 1) {
          bitsInByte = blocks[currentBlockIndex].lastByteUsedBits;
        }

        if (currentBitIndex >= bitsInByte) {
          currentBitIndex = 0;
          currentByteIndex++;
          if (currentByteIndex >= blocks[currentBlockIndex].data.size()) {
            // End of data.
            if (blocks[currentBlockIndex].pauseAfter > 0) {
              currentState = PAUSE;
              nextEdgeTState = blocks[currentBlockIndex].pauseAfter * 3500L;
            } else {
              currentBlockIndex++;
              startNextBlock();
            }
          } else {
            // Next Byte
            byte b = blocks[currentBlockIndex].data[currentByteIndex];
            bool bit = (b & (1 << 7)) != 0;

            if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
              nextEdgeTState = bit ? blocks[currentBlockIndex].oneLen
                                   : blocks[currentBlockIndex].zeroLen;
            } else {
              nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
            }
          }
        } else {
          // Next Bit
          byte b = blocks[currentBlockIndex].data[currentByteIndex];
          bool bit = (b & (1 << (7 - currentBitIndex))) != 0;

          if (blocks[currentBlockIndex].type == utils::TURBO_SPEED_DATA) {
            nextEdgeTState = bit ? blocks[currentBlockIndex].oneLen
                                 : blocks[currentBlockIndex].zeroLen;
          } else {
            nextEdgeTState = bit ? BIT1_PULSE : BIT0_PULSE;
          }
        }
      }
      break;

    case PAUSE:
      // Pause finished
      currentBlockIndex++;
      startNextBlock();
      break;

    case TONE:
      pulseCount++;
      if (pulseCount >= blocks[currentBlockIndex].pulseCount) {
        currentBlockIndex++;
        startNextBlock();
      } else {
        nextEdgeTState = blocks[currentBlockIndex].pulseLength;
      }
      break;

    case PULSE_SEQUENCE:
      if (currentByteIndex >= blocks[currentBlockIndex].pulseSequence.size()) {
        currentBlockIndex++;
        startNextBlock();
      } else {
        nextEdgeTState =
            blocks[currentBlockIndex].pulseSequence[currentByteIndex];
        currentByteIndex++;
      }
      break;

    case LEAD_IN:
      // Lead-in finished, start actual block
      startNextBlock();
      break;

    default:
      stop();
      break;
    }
  }
}
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

        // Found match! Validate checksum/length roughly
        // If length is huge (65535), we just trust the block size.
        // We ensure we have at least 2 bytes (Flag + Checksum)
        if (blocks[scanIndex].data.size() < 2) {
          // Block too short (no flag/checksum) - skipping
          scanIndex++;
          continue;
        }

        // Load Data
        // We load MIN(requested_length, available_payload)
        // Payload = BlockSize - 1 (Flag) - 1 (Checksum)
        // Actually, some TZX might not have checksum?
        // Standard ROM expects checksum byte at end.
        // We will copy payload bytes to memory.

        const std::vector<byte> &data = blocks[scanIndex].data;
        size_t payloadSize = data.size() - 2; // Exclude Flag and Checksum
        size_t copyLen = length;

        // If request is larger than block, we only copy what we have
        if (copyLen > payloadSize)
          copyLen = payloadSize;

        for (size_t i = 0; i < copyLen; i++) {
          memory[(startAddress + i) & 0xFFFF] = data[i + 1]; // +1 to skip Flag
        }

        // Advance Tape to *next* block
        currentBlockIndex = scanIndex + 1;

        // Always stop after fast load
        stop();

        return true;
      } else {
        // Flag mismatch (e.g. found Data when looking for Header)
        // Skip this block and keep searching
      }
    }
    // Skip this block (non-data or mismatch)
    scanIndex++;
  }

  return false;
}
