// File: (Screen.h)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

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

  static Screen *Factory();
};

#endif // ZXEMULATOR_SCREEN_H
