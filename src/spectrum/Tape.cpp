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

#include "Tape.h"
#include "../utils/Logger.h"
#include "../utils/TZXLoader.h"

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

enum TapeState { STOPPED, PILOT, SYNC1, SYNC2, DATA, PAUSE };

// State variables
TapeState currentState = STOPPED;
size_t currentBlockIndex = 0;
size_t currentByteIndex = 0;
int currentBitIndex = 0;
int pulseCount = 0;
long tStateCounter = 0;
long nextEdgeTState = 0;
bool earBit = false;

// Internal helpers
void nextBlock() {
  // Logic to move to next block
}

Tape::Tape() {}

bool Tape::load(const std::string &filename) {
  this->filename = filename;
  std::string msg = "Loading tape: " + filename;
  Logger::write(msg.c_str());

  TZXLoader loader(filename.c_str());
  if (loader.isValid()) {
    loader.parse();
    this->blocks = loader.getBlocks();
    return true;
  } else {
    Logger::write("Failed to load or invalid TZX file");
    return false;
  }
}

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
}
// Stubbing complex logic for now to allow build, full impl requires careful
// coding. For the immediate user request "it reads but runs basic rom", just
// getting the tape *ready* with valid blocks is step 1. The user needs
// automation to type LOAD "". I will implement a simpler "Play" that just logs
// "Playing" for now if we don't have full pulse logic ready. Actually, I
// promised signal generation.
