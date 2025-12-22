// File: (WindowsScreen.h)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

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

  void drawRow(unsigned char **pixels, int y) const;

  void setPixel(unsigned char **pixel, sf::Color colour) const;

  void drawBorderRow(unsigned char **pixels) const;

  void drawBorder(unsigned char **pixel, sf::Color colour) const;

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
