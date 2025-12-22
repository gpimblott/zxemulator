// File: (WindowsScreen.cpp)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <SFML/Graphics/Color.hpp>
#include <cstdio>

#include "../../../utils/PeriodTimer.h"
#include "../../Processor.h"
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
    // handle error if needed
    printf("Error: Failed to resize texture\n");
  }

  // Ensure sprite uses the full texture size now that it is resized
  sprite.setTexture(texture, true);
  sprite.setScale(sf::Vector2f(WINDOW_SCALE, WINDOW_SCALE));
}

void WindowsScreen::show() {
  printf("Creating window()\n");
  theWindow.create(sf::VideoMode(sf::Vector2u(FULL_WIDTH * WINDOW_SCALE,
                                              FULL_HEIGHT * WINDOW_SCALE)),
                   "ZX Emulator");

  theWindow.setFramerateLimit(50);
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
  // byte *screenBufferPtr = this->videoBuffer->getBuffer();
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

    // screenBufferPtr += BYTES_PER_ROW;
  }

  texture.update(pixelBuffer);
  // Debug attribute at 0,0 (Top Left)
  // printf("Attribute(0,0): %02X\n", videoBuffer->getAttribute(0, 0));

  theWindow.clear(sf::Color::Black);
  theWindow.draw(sprite);

  theWindow.draw(sprite);

  /* Debug button removed as per request */

  theWindow.display();

  // Get starting timepoint
  // printf("Finished frame update - execution %ld microseconds\n",
  // timer.stop());
}

// processEvents moved to bottom
// bool WindowsScreen::processEvents() { ... }

/**
 * Draw a full border row for use at the top and bottom on the screen
 * May not be needed when I sort it all out
 * @param pixels
 * @return
 */
void WindowsScreen::drawBorderRow(std::uint8_t **pixels) const {
  byte borderIdx = (*videoBuffer).getBorderColor();
  sf::Color borderColour = colors[borderIdx];

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
  byte borderIdx = (*videoBuffer).getBorderColor();
  sf::Color borderColour = colors[borderIdx];

  // Draw the left border
  for (int i = 0; i < BORDER_WIDTH; i++) {
    setPixel(pixels, borderColour);
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
    setPixel(pixels, borderColour);
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

void WindowsScreen::initDebug() {
  if (!debugFont.openFromFile("/System/Library/Fonts/Monaco.ttf")) {
    // Fallback if needed or log error
    printf("Failed to load debug font\n");
  } else {
    printf("Loaded debug font\n");
  }
}

void WindowsScreen::drawDebugWindow() {
  if (!showDebug || !debugWindow.isOpen())
    return;

  debugWindow.clear(sf::Color(50, 50, 50));

  if (!processor)
    return;

  ProcessorState &state = processor->getState();

  char buffer[256];
  sf::Text text(debugFont);
  text.setCharacterSize(14);
  text.setFillColor(sf::Color::White);
  text.setPosition({10, 10});

  // Registers
  snprintf(buffer, sizeof(buffer),
           "A: %02X  F: %02X\nBC: %04X\nDE: %04X\nHL: %04X\nSP: %04X\nPC: "
           "%04X\n\nFlags: %c%c%c%c%c%c%c%c",
           state.registers.A, state.registers.F, state.registers.BC,
           state.registers.DE, state.registers.HL, state.registers.SP,
           state.registers.PC, (state.registers.F & 0x80) ? 'S' : '-',
           (state.registers.F & 0x40) ? 'Z' : '-',
           (state.registers.F & 0x20) ? '5' : '-',
           (state.registers.F & 0x10) ? 'H' : '-',
           (state.registers.F & 0x08) ? '3' : '-',
           (state.registers.F & 0x04) ? 'P' : '-',
           (state.registers.F & 0x02) ? 'N' : '-',
           (state.registers.F & 0x01) ? 'C' : '-');
  text.setString(buffer);
  debugWindow.draw(text);

  // Status
  sf::Text statusText(debugFont);
  statusText.setCharacterSize(14);
  statusText.setPosition({200, 180});
  if (processor->isRunning()) {
    if (processor->isPaused()) {
      statusText.setString("Status: PAUSED");
      statusText.setFillColor(sf::Color::Yellow);
    } else {
      statusText.setString("Status: RUNNING");
      statusText.setFillColor(sf::Color::Green);
    }
  } else {
    statusText.setString("Status: STOPPED");
    statusText.setFillColor(sf::Color::Red);
  }
  debugWindow.draw(statusText);

  if (!processor->getLastError().empty()) {
    sf::Text errorText(debugFont);
    errorText.setCharacterSize(12);
    errorText.setFillColor(sf::Color::Red);
    errorText.setPosition({10, 250});
    errorText.setString("Error: " + processor->getLastError());
    debugWindow.draw(errorText);
  }

  // Disassembly output
  int pc = state.registers.PC;
  sf::Text asmText(debugFont);
  asmText.setCharacterSize(12);
  asmText.setFillColor(sf::Color::Yellow);
  asmText.setPosition({200, 10});

  std::string asmStr = "Disassembly:\n";
  for (int i = 0; i < 10; i++) {
    byte op = state.memory[pc + i];
    char hex[16];
    snprintf(hex, sizeof(hex), "%04X: %02X ", pc + i, op);
    asmStr += hex;

    OpCode *opcode = processor->getOpCode(op);
    if (opcode) {
      asmStr += opcode->getName();
    } else {
      asmStr += "???";
    }
    asmStr += "\n";
  }
  asmText.setString(asmStr);
  debugWindow.draw(asmText);

  // Buttons (Simple text buttons for now)
  sf::Text btnText(debugFont);
  btnText.setCharacterSize(16);
  btnText.setPosition({10, 200});

  if (processor->isPaused()) {
    btnText.setString("[RESUME]");
    btnText.setFillColor(sf::Color::Green);
    btnText.setPosition({10, 200});
    debugWindow.draw(btnText);

    btnText.setString("[STEP]");
    btnText.setFillColor(sf::Color::Green);
    btnText.setPosition({130, 200});
    debugWindow.draw(btnText);

    btnText.setString("[RESET]");
    btnText.setFillColor(sf::Color::Green);
    btnText.setPosition({210, 200});
    debugWindow.draw(btnText);

  } else {
    btnText.setString("[PAUSE]");
    btnText.setFillColor(sf::Color::Red); // Changed from Red to be consistent?
                                          // No, Red for Pause is fine.
    btnText.setPosition({10, 200});
    debugWindow.draw(btnText);
  }

  debugWindow.display();
}

bool WindowsScreen::processEvents() {
  // Main Window Events
  while (const auto event = theWindow.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      theWindow.close();
      if (debugWindow.isOpen())
        debugWindow.close();
      return false;
    } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
      /* D key removed */
    } else if (const auto *mouseButton =
                   event->getIf<sf::Event::MouseButtonPressed>()) {
      if (mouseButton->button == sf::Mouse::Button::Left) {
        // Debug Button removed
      }
    }
  }

  // Debug Window Events
  if (showDebug && debugWindow.isOpen()) {
    while (const auto event = debugWindow.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        showDebug = false;
        debugWindow.close();
        if (processor)
          processor->resume();
      }
      // Simple click handling for buttons
      else if (const auto *mouseButton =
                   event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseButton->button == sf::Mouse::Button::Left) {
          printf("Debug Win Click: %d, %d. Processor: %p\n",
                 mouseButton->position.x, mouseButton->position.y,
                 (void *)processor);
          int y = mouseButton->position.y;
          if (y > 190 && y < 230) { // Rough button area
            if (processor) {
              if (processor->isPaused()) {
                // Resume or Step?
                // Resume or Step?
                if (mouseButton->position.x < 120) {
                  printf("Resume requested\n");
                  processor->resume();
                } else if (mouseButton->position.x >= 120 &&
                           mouseButton->position.x < 200) {
                  printf("Step requested\n");
                  processor->step();
                } else if (mouseButton->position.x >= 200) {
                  printf("Reset requested\n");
                  processor->reset();
                  processor->pause();
                }
              } else {
                printf("Pause requested\n");
                processor->pause();
              }
            } else {
              printf("Error: Processor is null\n");
            }
          }
        }
      }
    }
    drawDebugWindow();
  }

  return theWindow.isOpen();
}

void WindowsScreen::setDebugMode(bool debug) {
  showDebug = debug;
  if (showDebug) {
    if (!debugWindow.isOpen()) {
      debugWindow.create(sf::VideoMode({400, 300}), "Debugger");
      initDebug();
    }
    debugWindow.requestFocus();
  } else {
    if (debugWindow.isOpen())
      debugWindow.close();
  }
}
