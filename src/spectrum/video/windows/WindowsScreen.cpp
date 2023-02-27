// File: (WindowsScreen.cpp)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstdio>
#include <SFML/Graphics/Color.hpp>


#include "WindowsScreen.h"
#include "../../../utils/PeriodTimer.h"

using namespace std::chrono;

static const int numberOfRows = 192;
static const int bytesPerRow = 32;

void WindowsScreen::init(VideoBuffer *buffer) {
    printf("Init window()\n");
    this->videoBuffer = buffer;

    // Create the texture and assign it to a scaled sprite
    texture.create(FULL_WIDTH, FULL_HEIGHT);
    sprite.setTexture(texture);
    sprite.setScale(sf::Vector2f(WINDOW_SCALE, WINDOW_SCALE));
}

void WindowsScreen::show() {
    printf("Creating window()\n");
    theWindow.create(
            sf::VideoMode(FULL_WIDTH * WINDOW_SCALE, FULL_HEIGHT * WINDOW_SCALE),
            "ZX Emulator");

    theWindow.setFramerateLimit(30);
    theWindow.clear(sf::Color::White);
}

void WindowsScreen::hide() {
    printf("Hide window()\n");
}

/**
 * Redraw the screen from the Video buffer
 * The way we do this read the video memory and draw to a byte buffer (screenBuffer)
 * The byte buffer is copied to a sprint texture which is then copied to the window
 * The window copy is scaled to increase its size
 */
void WindowsScreen::update() {
    printf("Starting frame update\n");

    // Start a timer
    PeriodTimer timer;
    timer.start();

    // Get a pointer for the screen buffer
    byte *screenBufferPtr = this->videoBuffer->getBuffer();
    byte *currentLinePtr = this->videoBuffer->getBuffer();

//    for (int row = 0; row < numberOfRows; row++) {
//
//        // Move to the next row
//        currentLinePtr += bytesPerRow;
//    }


    // Redraw the double buffer
    sf::Uint8 *currentPixelPtr = pixelBuffer;
    int bufferRow = 0;

    // Loop through drawing all the rows of the screen
    for (int y = 0; y < FULL_HEIGHT; y++) {
        if ((y < BORDER_WIDTH) || (y >= (FULL_HEIGHT - BORDER_WIDTH))) {
            // Draw the top and bottom borders
            drawBorderRow(&currentPixelPtr);
        } else {
            // Draw a row from the video buffer
            drawRow(&currentPixelPtr, bufferRow++);
        }

         screenBufferPtr += BYTES_PER_ROW;
    }

    texture.update(pixelBuffer);


    theWindow.draw(sprite);
    theWindow.display();

    // Get starting timepoint
    printf("Finished frame update - execution %d microseconds\n", timer.stop());

    waitForEvent();

}

/**
 * Draw a full border row for use at the top and bottom on the screen
 * May not be needed when I sort it all out
 * @param pixels
 * @return
 */
void WindowsScreen::drawBorderRow(sf::Uint8 **pixels) const {
    sf::Color borderColour = sf::Color::Cyan;

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
void WindowsScreen::drawRow(sf::Uint8 **pixels, int y) const {
    sf::Color borderColour = sf::Color::Cyan;

    drawBorder(pixels, borderColour);

    for (int x = 0; x < VIEWPORT_WIDTH / 8; x++) {
        byte data = (*videoBuffer).getByte(x, y);
        for (int bits = 0; bits < 8; bits++) {
            sf::Color colour = (data & (0b00000001 << bits)) ? sf::Color::Black : sf::Color::White;
            setPixel(pixels, colour);
        }
    }

    drawBorder(pixels, borderColour);
}

/**
 * Draw the border pixels into the buffer
 * @param pixel
 * @param colour
 */
void WindowsScreen::drawBorder(sf::Uint8 **pixel, sf::Color colour) const {
    for (int x = 0; x < BORDER_WIDTH; x++) {
        setPixel(pixel, colour);
    }
}

/**
 * Set the colour of a specific pixel
 * @param pixel
 * @param colour
 */
void WindowsScreen::setPixel(sf::Uint8 **pixel, sf::Color colour) const {
    sf::Uint8 *&ptr = *pixel;
    // printf("Colour : %d\n" , colour.toInteger());
    *(ptr++) = colour.r;
    *(ptr++) = colour.g;
    *(ptr++) = colour.b;
    *(ptr++) = colour.a;
}

void WindowsScreen::waitForEvent() {// run the program as long as the window is open
    while (theWindow.isOpen()) {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (theWindow.pollEvent(event)) {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                theWindow.close();
        }
    }
}
