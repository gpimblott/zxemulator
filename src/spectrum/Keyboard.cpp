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

#include "Keyboard.h"

Keyboard::Keyboard() { reset(); }

void Keyboard::reset() {
  for (int i = 0; i < 8; ++i) {
    keyLines[i] = 0; // 0 = no keys pressed
  }
  kempstonState = 0;
}

void Keyboard::setKey(int line, int bit, bool pressed) {
  if (line < 0 || line > 7 || bit < 0 || bit > 4)
    return;

  if (pressed) {
    keyLines[line] |= (1 << bit);
  } else {
    keyLines[line] &= ~(1 << bit);
  }
}

emulator_types::byte Keyboard::readPort(emulator_types::byte highByte) {
  emulator_types::byte result = 0x1F; // Default: all keys released (11111)

  // Verify which line is being requested.
  // The ULA pulls the lines low. The high byte determines which address line is
  // low. If bit 0 of highByte is 0, then line 0 (FE) is selected. If bit 1 of
  // highByte is 0, then line 1 (FD) is selected.
  // ...
  // Multiple lines can be selected at once!

  for (int i = 0; i < 8; ++i) {
    // Check if bit i of highByte is LOW (0)
    if (!(highByte & (1 << i))) {
      // Line i is selected.
      // AND the result with the inverted key state for this line.
      // keyLines[i] has 1 for pressed. We need 0 for pressed.
      // So we want result bits to be 0 if key is pressed.
      result &= ~keyLines[i];
    }
  }

  // Mask to just lower 5 bits, upper bits are typically 1 (floating or other
  // usage) The ULA usually returns 1s for floating bits, often bits 5-7 depend
  // on other things (ear/mic/floating bus) We'll return 1s for upper bits here
  // to be safe or just mask. Real hardware: D0-D4 are keys. D6 is EAR. For now
  // we just focus on the keys. Upper bits should probably be FF or result | E0
  // FIX: Bit 6 is EAR. We should leave it 0 here so IOOpcodes can OR in the
  // actual EAR value. Bits 5 and 7 are usually 1.
  return result | 0xA0;
}

void Keyboard::setKempstonKey(int bit, bool pressed) {
  if (bit < 0 || bit > 4)
    return;

  // Kempston is Active High (1 = Pressed)
  if (pressed) {
    kempstonState |= (1 << bit);
  } else {
    kempstonState &= ~(1 << bit);
  }
}

emulator_types::byte Keyboard::readKempstonPort() { return kempstonState; }
