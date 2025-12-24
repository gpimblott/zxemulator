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

#include "spectrum/Processor.h"
#include "spectrum/TapeLoader.h"
#include "spectrum/video/Screen.h"
#include "utils/Logger.h"
#include <chrono>
#include <thread>

using namespace std;
using namespace utils;

int main(int argc, char *argv[]) {
  try {
    const char *romFileLocation = "./roms/48k.bin";
    bool debugMode = false;
    bool fastLoad = false;

    std::string tapeFile = "";
    std::string snapshotFile = "";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-d" || arg == "--debug") {
        debugMode = true;
      } else if (arg == "-t" || arg == "--tape") {
        if (i + 1 < argc) {
          tapeFile = argv[++i];
        }
      } else if (arg == "-r" || arg == "--rom") {
        if (i + 1 < argc) {
          romFileLocation = argv[++i];
        }
      } else if (arg == "-s" || arg == "--snapshot") {
        if (i + 1 < argc) {
          snapshotFile = argv[++i];
        }
      } else if (arg == "-f" || arg == "--fast-load") {
        if (i + 1 < argc) {
          tapeFile = argv[++i];
          fastLoad = true;
        }
      }
    }

    Logger::write("Starting ZX Spectrum Emulator v0.1");

    // Create a processor and load the basic ROM
    Processor processor;
    processor.init(romFileLocation);
    // processor.setFastLoad(fastLoad); // Will add this method

    if (!tapeFile.empty()) {
      Tape tape = TapeLoader::load(tapeFile.c_str());
      processor.loadTape(tape);
      if (fastLoad) {
        processor.getState().setFastLoad(true);
      }
    }

    if (!snapshotFile.empty()) {
      processor.loadSnapshot(snapshotFile.c_str());
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

    auto frameDuration = std::chrono::milliseconds(20); // 50Hz

    while (screen->processEvents()) {
      auto start = std::chrono::high_resolution_clock::now();

      processor.executeFrame();
      screen->update();

      auto end = std::chrono::high_resolution_clock::now();
      auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      if (elapsed < frameDuration) {
        std::this_thread::sleep_for(frameDuration - elapsed);
      }
    }

  } catch (exception &ex) {
    printf("Error: %s", ex.what());
  }
  return 0;
}
