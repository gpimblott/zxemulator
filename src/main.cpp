#include "spectrum/Processor.h"
#include "spectrum/Rom.h"
#include "spectrum/video/Screen.h"
#include "utils/BinaryFileLoader.h"
#include "utils/Logger.h"
#include "utils/RegisterUtils.h"

using namespace std;
using namespace utils;

int main() {
  try {
    const char *romFileLocation = "./roms/48k.bin";
    Logger::write("Starting ZX Spectrum Emulator v0.1");

    // Create a processor and load the basic ROM
    Processor processor;
    processor.init(romFileLocation);

    // Create the screen
    Screen *screen = Screen::Factory();
    screen->init(processor.getVideoBuffer());
    screen->setProcessor(&processor);

    screen->show();

    while (screen->processEvents()) {
      processor.executeFrame();
      screen->update();
    }

  } catch (exception &ex) {
    printf("Error: %s", ex.what());
  }
  return 0;
}
