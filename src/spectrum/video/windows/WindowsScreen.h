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

#ifndef ZXEMULATOR_WINDOWSSCREEN_H
#define ZXEMULATOR_WINDOWSSCREEN_H

#include "../Screen.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window.hpp>
#include <cstdint>

#define WINDOW_SCALE 2
#define BORDER_WIDTH 48

#define VIEWPORT_WIDTH (SPECTRUM_SCREEN_WIDTH)
#define VIEWPORT_HEIGHT (SPECTRUM_SCREEN_HEIGHT)
#define FULL_WIDTH (VIEWPORT_WIDTH + (BORDER_WIDTH * 2))
#define FULL_HEIGHT (VIEWPORT_HEIGHT + (BORDER_WIDTH * 2))

class Processor;

/**
 *
 */
class WindowsScreen : public Screen {
private:
  sf::RenderWindow theWindow;
  sf::Texture texture;
  sf::Sprite sprite;
  std::uint8_t *pixelBuffer =
      new std::uint8_t[FULL_WIDTH * FULL_HEIGHT *
                       4]; // * 4 because pixelBuffer have 4 components (RGBA)

  sf::Color colors[16];
  int flashFrameCounter = 0;

  void drawRow(unsigned char **pixels, int y, int absY) const;

  void setPixel(unsigned char **pixel, sf::Color colour) const;

  void drawBorderRow(unsigned char **pixels, int y) const;

  void drawBorder(unsigned char **pixel, sf::Color colour) const;

  void handleKey(sf::Keyboard::Key key, bool pressed);
  void mapSymbol(bool pressed, int unshiftedLine, int unshiftedBit,
                 int shiftedLine, int shiftedBit);

public:
  sf::RenderWindow debugWindow;
  sf::Font debugFont;
  bool showDebug = false;
  Processor *processor = nullptr;

  void drawDebugWindow();
  void initDebug();

public:
  WindowsScreen();
  void init(VideoBuffer *buffer) override;

  void update() override;

  void show() override;

  void hide() override;

  bool processEvents() override;
  void waitForEvent();

  void setProcessor(Processor *p) override { processor = p; };
  void setDebugMode(bool debug) override;
};

#endif // ZXEMULATOR_WINDOWSSCREEN_H
