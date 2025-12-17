// File: (WindowsScreen.cpp)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <SFML/Graphics/Color.hpp>
#include <cstdio>

#include "../../../utils/PeriodTimer.h"
#include "WindowsScreen.h"

using namespace std::chrono;

static const int numberOfRows = 192;
static const int bytesPerRow = 32;

WindowsScreen::WindowsScreen() : sprite(texture) {
  // Base colors (Bright 0)
  colors[0] = sf::Color(0, 0, 0);       // Black
  colors[1] = sf::Color(0, 0, 205);     // Blue
  colors[2] = sf::Color(205, 0, 0);     // Red
  colors[3] = sf::Color(205, 0, 205);   // Magenta
  colors[4] = sf::Color(0, 205, 0);     // Green
  colors[5] = sf::Color(0, 205, 205);   // Cyan
  colors[6] = sf::Color(205, 205, 0);   // Yellow
  colors[7] = sf::Color(205, 205, 205); // White

  // Bright colors (Bright 1)
  colors[8] = sf::Color(0, 0, 0);        // Black
  colors[9] = sf::Color(0, 0, 255);      // Bright Blue
  colors[10] = sf::Color(255, 0, 0);     // Bright Red
  colors[11] = sf::Color(255, 0, 255);   // Bright Magenta
  colors[12] = sf::Color(0, 255, 0);     // Bright Green
  colors[13] = sf::Color(0, 255, 255);   // Bright Cyan
  colors[14] = sf::Color(255, 255, 0);   // Bright Yellow
  colors[15] = sf::Color(255, 255, 255); // Bright White
}

void WindowsScreen::init(VideoBuffer *buffer) {
  printf("Init window()\n");
  this->videoBuffer = buffer;

  // Create the texture and assign it to a scaled sprite
  // Note: Sprite is already bound to texture in constructor
  if (!texture.resize(sf::Vector2u(FULL_WIDTH, FULL_HEIGHT))) {
    // handle error if needed, though resize returns bool in SFML 3?
    // Checking docs: Texture::resize is effectively create in SFML 2.
    // SFML 3 Texture::resize(size) exists.
  }
  // SFML 3: Texture::create is removed/renamed? explicit resize(size) is used.
  // Actually SFML 3 uses resize(Vector2u).
  // Let's assume resize(Vector2u) works or check if create exists.
  // SFML 3 docs: Texture::resize(Vector2u size).
  texture.resize(sf::Vector2u(FULL_WIDTH, FULL_HEIGHT));

  // sprite.setTexture(texture); // Done in constructor
  sprite.setScale(sf::Vector2f(WINDOW_SCALE, WINDOW_SCALE));
}

void WindowsScreen::show() {
  printf("Creating window()\n");
  theWindow.create(sf::VideoMode(sf::Vector2u(FULL_WIDTH * WINDOW_SCALE,
                                              FULL_HEIGHT * WINDOW_SCALE)),
                   "ZX Emulator");

  theWindow.setFramerateLimit(30);
  theWindow.clear(sf::Color::White);
}

void WindowsScreen::hide() { printf("Hide window()\n"); }

/**
 * Redraw the screen from the Video buffer
 * The way we do this read the video memory and draw to a byte buffer
 * (screenBuffer) The byte buffer is copied to a sprint texture which is then
 * copied to the window The window copy is scaled to increase its size
 */
void WindowsScreen::update() {
  // printf("Starting frame update\n");

  // Start a timer
  PeriodTimer timer;
  timer.start();

  // Get a pointer for the screen buffer
  byte *screenBufferPtr = this->videoBuffer->getBuffer();
  // byte *currentLinePtr = this->videoBuffer->getBuffer(); // unused

  //    for (int row = 0; row < numberOfRows; row++) {
  //
  //        // Move to the next row
  //        currentLinePtr += bytesPerRow;
  //    }

  // Redraw the double buffer
  std::uint8_t *currentPixelPtr = pixelBuffer;
  int bufferRow = 0;

  // Loop through drawing all the rows of the screen
  for (int y = 0; y < FULL_HEIGHT; y++) {
    if ((y < BORDER_WIDTH) || (y >= (FULL_HEIGHT - BORDER_WIDTH))) {
      // Draw the top and bottom borders
      drawBorderRow(&currentPixelPtr);
    } else {
      // Draw a row from the video buffer
      // y is absolute screen y. Need relative y for video buffer (0-191)
      int videoY = y - BORDER_WIDTH;
      drawRow(&currentPixelPtr, videoY);
    }

    screenBufferPtr += BYTES_PER_ROW;
  }

  texture.update(pixelBuffer);

  theWindow.draw(sprite);
  theWindow.display();

  // Get starting timepoint
  // printf("Finished frame update - execution %ld microseconds\n",
  // timer.stop());
}

bool WindowsScreen::processEvents() {
  while (const auto event = theWindow.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      theWindow.close();
      return false;
    }
  }
  return theWindow.isOpen();
}

/**
 * Draw a full border row for use at the top and bottom on the screen
 * May not be needed when I sort it all out
 * @param pixels
 * @return
 */
void WindowsScreen::drawBorderRow(std::uint8_t **pixels) const {
  // sf::Color borderColour = sf::Color::Cyan; // Incorrect hardcoded color
  sf::Color borderColour = colors[7]; // White border default

  for (int x = 0; x < FULL_WIDTH; x++) {
    setPixel(pixels, borderColour);
  }
}

/**
 * Draw a single row from the screen buffer including the pre and post borders
 * @param pixels
 * @param y
 * @return
 */
void WindowsScreen::drawRow(std::uint8_t **pixels, int y) const {
  // Draw the left border
  for (int i = 0; i < BORDER_WIDTH; i++) {
    setPixel(pixels, colors[7]); // White border default
  }

  for (int x = 0; x < VIEWPORT_WIDTH / 8; x++) {
    byte data = (*videoBuffer).getByte(x, y);
    byte attr = (*videoBuffer).getAttribute(x, y);

    // Decode Attribute: F B PPP III
    int brightIdx = (attr & 0x40) ? 8 : 0;
    int paperIdx = ((attr >> 3) & 0x07) + brightIdx;
    int inkIdx = (attr & 0x07) + brightIdx;

    sf::Color paperColor = colors[paperIdx];
    sf::Color inkColor = colors[inkIdx];

    for (int bits = 0; bits < 8; bits++) {
      // MSB is left pixel
      bool pixelSet = (data & (0x80 >> bits)) != 0;
      sf::Color colour = pixelSet ? inkColor : paperColor;
      setPixel(pixels, colour);
    }
  }

  // Draw the right border
  for (int i = 0; i < BORDER_WIDTH; i++) {
    setPixel(pixels, colors[7]);
  }
}

/**
 * Draw the border pixels into the buffer
 * @param pixel
 * @param colour
 */
void WindowsScreen::drawBorder(std::uint8_t **pixel, sf::Color colour) const {
  for (int x = 0; x < BORDER_WIDTH; x++) {
    setPixel(pixel, colour);
  }
}

/**
 * Set the colour of a specific pixel
 * @param pixel
 * @param colour
 */
void WindowsScreen::setPixel(std::uint8_t **pixel, sf::Color colour) const {
  std::uint8_t *&ptr = *pixel;
  // printf("Colour : %d\n" , colour.toInteger());
  *(ptr++) = colour.r;
  *(ptr++) = colour.g;
  *(ptr++) = colour.b;
  *(ptr++) = colour.a;
}

void WindowsScreen::waitForEvent() {
  // Deprecated
}
