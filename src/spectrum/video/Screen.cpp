// File: (Screen.cpp)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include "Screen.h"
#include "windows/WindowsScreen.h"

/**
 * Create the window appropriate for the environment
 * At the moment this is only windows
 * @return A concrete instance of the screen class
 */
Screen *Screen::Factory() {
    return new WindowsScreen();
}
