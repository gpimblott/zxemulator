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

#ifndef ZXEMULATOR_ESP32SCREEN_H
#define ZXEMULATOR_ESP32SCREEN_H

#include "../../src/spectrum/video/Screen.h"
#include "../../src/spectrum/video/VideoBuffer.h"
#include <cstdint>
#include <vector>

// Forward declaration to avoid pulling in TFT_eSPI everywhere
class TFT_eSPI;

class ESP32Screen : public Screen {
private:
  TFT_eSPI *tft;
  // A full framebuffer in 16-bit RGB565 format (used by TFT_eSPI)
  uint16_t *frameBuffer;
  int flashCounter;

  // We need to map the 16 ZX Spectrum colors to RGB565 format
  uint16_t palette[16];
  void initPalette();

  // Helper to extract the bits, colors, and render a single 8-pixel block
  void renderCharacterBlock(int x, int y, uint16_t *destBuffer,
                            bool flashInvert);

public:
  ESP32Screen();
  virtual ~ESP32Screen();

  virtual void init(VideoBuffer *buffer) override;
  virtual void show() override;
  virtual void hide() override;
  virtual void update() override;

  // For ESP32, we don't have a window manager pumping events,
  // but we can use this hook for polling local GPIOs if needed.
  virtual bool processEvents() override;
};

#endif // ZXEMULATOR_ESP32SCREEN_H
