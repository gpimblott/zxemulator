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
