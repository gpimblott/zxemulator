// File: (WindowsScreen.h)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_WINDOWSSCREEN_H
#define ZXEMULATOR_WINDOWSSCREEN_H

#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "../Screen.h"

#define WINDOW_SCALE 2
#define BORDER_WIDTH 48

#define VIEWPORT_WIDTH (SPECTRUM_SCREEN_WIDTH)
#define VIEWPORT_HEIGHT (SPECTRUM_SCREEN_HEIGHT)
#define FULL_WIDTH (VIEWPORT_WIDTH + (BORDER_WIDTH*2))
#define FULL_HEIGHT (VIEWPORT_HEIGHT + (BORDER_WIDTH*2))

/**
 *
 */
class WindowsScreen : public Screen {
private:
    sf::RenderWindow theWindow;
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Uint8 *pixelBuffer =
            new sf::Uint8[FULL_WIDTH * FULL_HEIGHT * 4]; // * 4 because pixelBuffer have 4 components (RGBA)

    void drawRow(unsigned char **pixels, int y) const;

    void setPixel(unsigned char **pixel, sf::Color colour) const;

    void drawBorderRow(unsigned char **pixels) const;

    void drawBorder(unsigned char **pixel, sf::Color colour) const;

public:
    void init(VideoBuffer *buffer) override;

    void update() override;

    void show() override;

    void hide() override;

    void waitForEvent();

};


#endif //ZXEMULATOR_WINDOWSSCREEN_H
