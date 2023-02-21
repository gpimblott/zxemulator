// File: (VideoBuffer.cpp)
// Created by G.Pimblott on 28/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#include <cstring>
#include "VideoBuffer.h"

/**
 * Overlay the Video videoBuffer onto the main memory map
 * @param memoryMap A pointer to the memory map being used
 */
VideoBuffer::VideoBuffer(emulator_types::byte *memoryMap) {
    this->videoBuffer = memoryMap + VIDEO_PIXEL_START;
    this->colourAttributes = memoryMap + VIDEO_ATTR_START;

    // Blank memory out
    memset( videoBuffer , 0 , VIDEO_BITMAP_DATA);
    memset(colourAttributes, 0, VIDEO_ATTR_DATA);
}

/**
 * The screen address is not linear and is encoded as follows
 *
 *            H            |           L
 * 15 14 13 12 11 10  9  8 |  7  6  5  4  3  2  1  0
 *  0  1  0 Y7 Y6 Y2 Y1 Y0 | Y5 Y4 Y3 X4 X3 X2 X1 X0
 *
 * Bits 13,14 & 15 encode the 0x4000 start address
 *
 * @param x X screen position
 * @param y Y screen position
 * @return The byte for the supplied position
 */
emulator_types::byte VideoBuffer::getByte(int x, int y) const {

    emulator_types::word address = encodeAddress(x, y);
    emulator_types::byte result = *(videoBuffer + address);

    return result;
}

/**
 * Set the byte at the current x,y position in the video videoBuffer
 * @param x X Position
 * @param y Y Position
 * @param data The byte to set the target to
 */
void VideoBuffer::setByte(int x, int y, emulator_types::byte data) {

    emulator_types::word address = encodeAddress(x, y);
    *(videoBuffer + address) = data;
}

/**
 * Get the colour attributes for the character at the given x,y position
 * @param x X Position
 * @param y Y Position
 * @return The colour attribute byte for the specified position
 */
emulator_types::byte VideoBuffer::getAttribute(int x, int y) const {
    long addressOffset = ((y*VIDEO_WIDTH_CHARS)+x);

    // NOT IMPLEMENTED

    // return the byte at this 'addressOffset in the attribute button
    return 0;
}

/**
 * Override the [] operator to allow direct access the the video videoBuffer
 * @param index
 * @return
 */
emulator_types::byte& VideoBuffer::operator[] (int index) {
    if( index > VIDEO_BITMAP_DATA) {
        printf("Video videoBuffer index out of range\n");
        exit(0);
    }
    return videoBuffer[index];
}

/**
 * The memory layout is not linear - calculate the position in memory
 * given the x,y coordinates
 * @param x X Position
 * @param y Y Position
 * @return The address offset of the given x,y coordinates
 */
emulator_types::word VideoBuffer::encodeAddress(int x, int y) const {
    emulator_types::word address = 0x00;
    // copy X bits X0-X4 to 0-4 in the address
    address |= (x & 0b11111);

    // Copy Y bits Y3-Y5 to 5-7 in the address
    address |= ((y & 0b00111000) << 2);

    // Copy Y bits Y0-Y2 to 8-10 in the address
    address |= ((y & 0b00000111) << 8);

    // Copy Y bits Y6-Y7 to 11-12 in the address
    address |= ((y & 0b11000000) << 5);

    return address;
}

/**
 * Debug routine to output memory as a but pattern
 * @param msg The message to display before the pattern
 * @param size The number of bytes to display
 * @param ptr Pointer to the data to output
 */
void VideoBuffer::printBits(std::string msg, size_t const size, void const *const ptr) const {
    unsigned char *b = (unsigned char *) ptr;
    unsigned char byte;
    int i, j;

    printf("%s : ", msg.c_str());
    for (i = size - 1; i >= 0; i--) {
        printf( " ");
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    printf("\n");
}






