#include "utils/Logger.h"
#include "utils/BinaryFileLoader.h"
#include "utils/RegisterUtils.h"
#include "spectrum/Rom.h"
#include "spectrum/Memory.h"
#include "exceptions/MemoryException.h"
#include "spectrum/Processor.h"
#include "spectrum/video/Screen.h"
#include "spectrum/video/VideoBuffer.h"

using namespace std;
using namespace utils;

int main() {
    try {
        const char *romFileLocation = "./roms/48k.bin";
        Logger::write("Starting ZX Spectrum Emulator v0.1");


        // Create the main memory map
        Memory *memoryPtr = new Memory();
        VideoBuffer *videoBuffer = (*memoryPtr).getVideoBuffer();

        // Create the screen
        Screen *screen = Screen::Factory();
        screen->init(videoBuffer);
//        screen->show();
//
//        /**
//         * Put a test pattern into the video buffer
//         */
//        emulator_types::byte value = 170;
//        VideoBuffer *buffer = memoryPtr->getVideoBuffer();
//        for (int y = 0; y < SPECTRUM_SCREEN_HEIGHT; y++) {
//            value = y%2? 170:85;
//            for (int x = 0; x < SPECTRUM_SCREEN_WIDTH_BYTES; x++) {
//                (*buffer).setByte(x, y, value);
//            }
//        }

//        screen->update();

        // Create a processor and load the basic ROM
        Processor processor;
        processor.init(romFileLocation);

        processor.run();

    } catch (exception &ex) {
        printf("Error: %s", ex.what());
    }
    return 0;
}
