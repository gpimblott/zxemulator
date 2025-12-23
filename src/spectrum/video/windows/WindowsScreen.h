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
