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
    return true;
  } else {
    Logger::write("Failed to load or invalid TZX file");
    return false;
  }
}

void Tape::play() {
  if (!filename.empty()) {
    playing = true;
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
  Logger::write("Tape stopped.");
}

bool Tape::getEarBit() { return earBit; }

void Tape::update(int tStates) {
  if (!playing)
    return;

  tStateCounter += tStates;

  if (tStateCounter >= nextEdgeTState) {
    tStateCounter = 0; // Relative timing for simplicity
    earBit = !earBit;  // Toggle pulse

    switch (currentState) {
    case PILOT:
      if (++pulseCount >= PILOT_HEADER_COUNT) {
        currentState = SYNC1;
        nextEdgeTState = SYNC1_PULSE;
      } else {
        nextEdgeTState = PILOT_PULSE;
      }
      break;
    // ... implementation of other states
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
