#include "spectrum/Processor.h"
#include "spectrum/Rom.h"
#include "spectrum/video/Screen.h"
#include "utils/BinaryFileLoader.h"
#include "utils/Logger.h"
#include "utils/RegisterUtils.h"

using namespace std;
using namespace utils;

int main(int argc, char *argv[]) {
  try {
    const char *romFileLocation = "./roms/48k.bin";
    bool debugMode = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-d" || arg == "--debug") {
        debugMode = true;
      }
    }

    Logger::write("Starting ZX Spectrum Emulator v0.1");

    // Create a processor and load the basic ROM
    Processor processor;
    processor.init(romFileLocation);

    // VISUAL TEST PATTERN
    // 0x4000 is start of video RAM
    // 6144 bytes of pixels
    printf("Injecting Video Test Pattern...\n");
    for (int i = 0; i < 6144; ++i) {
      processor.getState().memory[0x4000 + i] = rand() % 256;
    }
    // Set attributes (Ink/Paper) to ensure visibility
    // 768 bytes of attributes at 0x5800
    for (int i = 0; i < 768; ++i) {
      processor.getState().memory[0x5800 + i] = rand() % 256;
    }

    // Debug: Check ROM integrity at 0x0672
    // byte b = processor.getState().memory.getByte(0x0672); // Need access?
    // ProcessorState exposes memory. Memory exposes [] or dump.
    // memory[0x0672]
    printf("ROM[0672] = %02X\n", (int)processor.getState().memory[0x0672]);

    // Create the screen
    Screen *screen = Screen::Factory();
    screen->init(processor.getVideoBuffer());
    screen->setProcessor(&processor);

    screen->show();

    if (debugMode) {
      processor.pause();
      screen->setDebugMode(true);
    }

    while (screen->processEvents()) {
      processor.executeFrame();
      screen->update();
    }

  } catch (exception &ex) {
    printf("Error: %s", ex.what());
  }
  return 0;
}
