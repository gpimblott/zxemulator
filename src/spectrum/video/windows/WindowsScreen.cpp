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

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>
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

  // theWindow.setVerticalSyncEnabled(true);
  // theWindow.setFramerateLimit(50);
  theWindow.setKeyRepeatEnabled(false);
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
      drawBorderRow(&currentPixelPtr, y);
    } else {
      // Draw a row from the video buffer
      // y is absolute screen y. Need relative y for video buffer (0-191)
      int videoY = y - BORDER_WIDTH;
      drawRow(&currentPixelPtr, videoY, y); // Pass absolute y too
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
void WindowsScreen::drawBorderRow(std::uint8_t **pixels, int y) const {
  byte borderIdx = (*videoBuffer).getBorderColorAtLine(y);
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
void WindowsScreen::drawRow(std::uint8_t **pixels, int y, int absY) const {
  byte borderIdx = (*videoBuffer).getBorderColorAtLine(absY);
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
      handleKey(keyPressed->code, true);
    } else if (const auto *keyReleased =
                   event->getIf<sf::Event::KeyReleased>()) {
      handleKey(keyReleased->code, false);
    } else if (const auto *textEntered =
                   event->getIf<sf::Event::TextEntered>()) {
      // Consume to avoid warnings
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

void WindowsScreen::handleKey(sf::Keyboard::Key key, bool pressed) {
  if (!processor)
    return;

  // Kempston Joystick Mapping
  // Right (0), Left (1), Down (2), Up (3), Fire (4)
  if (key == sf::Keyboard::Key::Right)
    processor->getState().keyboard.setKempstonKey(0, pressed);
  if (key == sf::Keyboard::Key::Left)
    processor->getState().keyboard.setKempstonKey(1, pressed);
  if (key == sf::Keyboard::Key::Down)
    processor->getState().keyboard.setKempstonKey(2, pressed);
  if (key == sf::Keyboard::Key::Up)
    processor->getState().keyboard.setKempstonKey(3, pressed);
  if (key == sf::Keyboard::Key::LAlt || key == sf::Keyboard::Key::RAlt ||
      key == sf::Keyboard::Key::RControl)
    processor->getState().keyboard.setKempstonKey(4, pressed);

  // Mapping
  // Line 0 (0xFE): SHIFT (0), Z (1), X (2), C (3), V (4)
  if (key == sf::Keyboard::Key::LShift || key == sf::Keyboard::Key::RShift)
    processor->getState().keyboard.setKey(0, 0, pressed);
  if (key == sf::Keyboard::Key::Z)
    processor->getState().keyboard.setKey(0, 1, pressed);
  if (key == sf::Keyboard::Key::X)
    processor->getState().keyboard.setKey(0, 2, pressed);
  if (key == sf::Keyboard::Key::C)
    processor->getState().keyboard.setKey(0, 3, pressed);
  if (key == sf::Keyboard::Key::V)
    processor->getState().keyboard.setKey(0, 4, pressed);

  // Line 1 (0xFD): A (0), S (1), D (2), F (3), G (4)
  if (key == sf::Keyboard::Key::A)
    processor->getState().keyboard.setKey(1, 0, pressed);
  if (key == sf::Keyboard::Key::S)
    processor->getState().keyboard.setKey(1, 1, pressed);
  if (key == sf::Keyboard::Key::D)
    processor->getState().keyboard.setKey(1, 2, pressed);
  if (key == sf::Keyboard::Key::F)
    processor->getState().keyboard.setKey(1, 3, pressed);
  if (key == sf::Keyboard::Key::G)
    processor->getState().keyboard.setKey(1, 4, pressed);

  // Line 2 (0xFB): Q (0), W (1), E (2), R (3), T (4)
  if (key == sf::Keyboard::Key::Q)
    processor->getState().keyboard.setKey(2, 0, pressed);
  if (key == sf::Keyboard::Key::W)
    processor->getState().keyboard.setKey(2, 1, pressed);
  if (key == sf::Keyboard::Key::E)
    processor->getState().keyboard.setKey(2, 2, pressed);
  if (key == sf::Keyboard::Key::R)
    processor->getState().keyboard.setKey(2, 3, pressed);
  if (key == sf::Keyboard::Key::T)
    processor->getState().keyboard.setKey(2, 4, pressed);

  // Line 3 (0xF7): 1 (0), 2 (1), 3 (2), 4 (3), 5 (4)
  if (key == sf::Keyboard::Key::Num1 || key == sf::Keyboard::Key::Numpad1)
    processor->getState().keyboard.setKey(3, 0, pressed);
  if (key == sf::Keyboard::Key::Num2 || key == sf::Keyboard::Key::Numpad2)
    processor->getState().keyboard.setKey(3, 1, pressed);
  if (key == sf::Keyboard::Key::Num3 || key == sf::Keyboard::Key::Numpad3)
    processor->getState().keyboard.setKey(3, 2, pressed);
  if (key == sf::Keyboard::Key::Num4 || key == sf::Keyboard::Key::Numpad4)
    processor->getState().keyboard.setKey(3, 3, pressed);
  if (key == sf::Keyboard::Key::Num5 || key == sf::Keyboard::Key::Numpad5)
    processor->getState().keyboard.setKey(3, 4, pressed);

  // Line 4 (0xEF): 0 (0), 9 (1), 8 (2), 7 (3), 6 (4)
  if (key == sf::Keyboard::Key::Num0 || key == sf::Keyboard::Key::Numpad0)
    processor->getState().keyboard.setKey(4, 0, pressed);
  if (key == sf::Keyboard::Key::Num9 || key == sf::Keyboard::Key::Numpad9)
    processor->getState().keyboard.setKey(4, 1, pressed);
  if (key == sf::Keyboard::Key::Num8 || key == sf::Keyboard::Key::Numpad8)
    processor->getState().keyboard.setKey(4, 2, pressed);
  if (key == sf::Keyboard::Key::Num7 || key == sf::Keyboard::Key::Numpad7)
    processor->getState().keyboard.setKey(4, 3, pressed);
  if (key == sf::Keyboard::Key::Num6 || key == sf::Keyboard::Key::Numpad6)
    processor->getState().keyboard.setKey(4, 4, pressed);

  // Line 5 (0xDF): P (0), O (1), I (2), U (3), Y (4)
  if (key == sf::Keyboard::Key::P)
    processor->getState().keyboard.setKey(5, 0, pressed);
  if (key == sf::Keyboard::Key::O)
    processor->getState().keyboard.setKey(5, 1, pressed);
  if (key == sf::Keyboard::Key::I)
    processor->getState().keyboard.setKey(5, 2, pressed);
  if (key == sf::Keyboard::Key::U)
    processor->getState().keyboard.setKey(5, 3, pressed);
  if (key == sf::Keyboard::Key::Y)
    processor->getState().keyboard.setKey(5, 4, pressed);

  // Line 6 (0xBF): ENTER (0), L (1), K (2), J (3), H (4)
  if (key == sf::Keyboard::Key::Enter)
    processor->getState().keyboard.setKey(6, 0, pressed);
  if (key == sf::Keyboard::Key::L)
    processor->getState().keyboard.setKey(6, 1, pressed);
  if (key == sf::Keyboard::Key::K)
    processor->getState().keyboard.setKey(6, 2, pressed);
  if (key == sf::Keyboard::Key::J)
    processor->getState().keyboard.setKey(6, 3, pressed);
  if (key == sf::Keyboard::Key::H)
    processor->getState().keyboard.setKey(6, 4, pressed);

  // Line 7 (0x7F): SPACE (0), SYM (1), M (2), N (3), B (4)
  if (key == sf::Keyboard::Key::Space)
    processor->getState().keyboard.setKey(7, 0, pressed);
  if (key == sf::Keyboard::Key::LControl || key == sf::Keyboard::Key::RControl)
    processor->getState().keyboard.setKey(
        7, 1,
        pressed); // Symbol Shift mapped to Ctrl
  if (key == sf::Keyboard::Key::M)
    processor->getState().keyboard.setKey(7, 2, pressed);
  if (key == sf::Keyboard::Key::N)
    processor->getState().keyboard.setKey(7, 3, pressed);
  if (key == sf::Keyboard::Key::B)
    processor->getState().keyboard.setKey(7, 4, pressed);

  // Special Keys
  // Delete (Shift + 0)
  if (key == sf::Keyboard::Key::Backspace) {
    processor->getState().keyboard.setKey(0, 0, pressed); // Shift
    processor->getState().keyboard.setKey(4, 0, pressed); // 0
  }

  // Extended Mode Shortcut (Left Alt / Option) -> Caps Shift + Symbol Shift
  if (key == sf::Keyboard::Key::LAlt || key == sf::Keyboard::Key::RAlt) {
    processor->getState().keyboard.setKey(0, 0, pressed); // Caps Shift
    processor->getState().keyboard.setKey(7, 1, pressed); // Symbol Shift
  }

  // Quote handling (" and ')
  // Quote handling (" and ')
  if (key == sf::Keyboard::Key::Apostrophe) {
    mapSymbol(pressed, 4, 3, 5, 0); // Unshifted: 7 ('), Shifted: P (")
  }

  // Punctuation Mappings
  if (key == sf::Keyboard::Key::Hyphen)
    mapSymbol(pressed, 6, 3, 4, 0); // J (-), 0 (_)
  if (key == sf::Keyboard::Key::Equal)
    mapSymbol(pressed, 6, 1, 6, 2); // L (=), K (+)
  if (key == sf::Keyboard::Key::Semicolon)
    mapSymbol(pressed, 5, 1, 0, 1); // O (;), Z (:)
  if (key == sf::Keyboard::Key::Comma)
    mapSymbol(pressed, 7, 3, 2, 3); // N (,), R (<)
  if (key == sf::Keyboard::Key::Period)
    mapSymbol(pressed, 7, 2, 2, 4); // M (.), T (>)
  if (key == sf::Keyboard::Key::Slash)
    mapSymbol(pressed, 0, 4, 0, 3); // V (/), C (?)
}

void WindowsScreen::mapSymbol(bool pressed, int unshiftedLine, int unshiftedBit,
                              int shiftedLine, int shiftedBit) {
  if (!processor)
    return;

  bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

  // Symbol Shift always needed for both
  processor->getState().keyboard.setKey(7, 1, pressed);

  if (pressed) {
    if (shift) {
      processor->getState().keyboard.setKey(shiftedLine, shiftedBit, true);
      // Force Caps Shift OFF to enable symbol mode without Extended Mode
      processor->getState().keyboard.setKey(0, 0, false);
    } else {
      processor->getState().keyboard.setKey(unshiftedLine, unshiftedBit, true);
    }
  } else {
    // RELEASE: Release BOTH to ensure no keys stuck if Shift changed state
    processor->getState().keyboard.setKey(shiftedLine, shiftedBit, false);
    processor->getState().keyboard.setKey(unshiftedLine, unshiftedBit, false);

    // Restore Caps Shift if it is physically held
    if (shift) {
      processor->getState().keyboard.setKey(0, 0, true);
    }
  }
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
