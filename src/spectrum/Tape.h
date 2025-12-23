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

#ifndef ZXEMULATOR_TAPE_H
#define ZXEMULATOR_TAPE_H

#include "../utils/BaseTypes.h"
#include "../utils/TZXLoader.h"
#include <string>
#include <vector>

class Tape {
private:
  std::string filename;
  bool playing = false;
  std::vector<utils::TapeBlock> blocks;

  // Playback state
  enum TapeState { STOPPED, PILOT, SYNC1, SYNC2, DATA, PAUSE };
  TapeState currentState = STOPPED;
  size_t currentBlockIndex = 0;
  size_t currentByteIndex = 0;
  int currentBitIndex = 0;
  int pulseCount = 0;
  long tStateCounter = 0;
  long nextEdgeTState = 0;
  bool earBit = false;

public:
  Tape();

  void setFilename(const std::string &fn) { filename = fn; }
  void setBlocks(const std::vector<utils::TapeBlock> &blks) { blocks = blks; }
  void play();
  void stop();

  // Returns true if the EAR bit should be high (1) or low (0)
  // For now, this will be a stub to verify connection
  bool getEarBit();

  void update(int tStates);

  bool isPlaying() const { return playing; }

  // Fast Load Support
  // Returns true if block found and loaded, false otherwise
  bool fastLoadBlock(emulator_types::byte expectedFlag,
                     emulator_types::word length,
                     emulator_types::word startAddress, class Memory &memory);

  bool hasBlocks() const { return !blocks.empty(); }
};

#endif // ZXEMULATOR_TAPE_H
