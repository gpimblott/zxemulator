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

#ifndef ZXEMULATOR_SCREEN_H
#define ZXEMULATOR_SCREEN_H

#include "../Memory.h"

#define SPECTRUM_SCREEN_WIDTH 256
#define SPECTRUM_SCREEN_WIDTH_BYTES (SPECTRUM_SCREEN_WIDTH / 8)
#define SPECTRUM_SCREEN_HEIGHT 192

class Processor;

class Screen {
protected:
  VideoBuffer *videoBuffer;

public:
  virtual void init(VideoBuffer *buffer) = 0;
  virtual void show() = 0;
  virtual void hide() = 0;
  virtual void update() = 0;
  virtual bool processEvents() { return true; };

  virtual void setProcessor(Processor *p) {}
  virtual void setDebugMode(bool debug) {}

  static Screen *Factory();
};

#endif // ZXEMULATOR_SCREEN_H
