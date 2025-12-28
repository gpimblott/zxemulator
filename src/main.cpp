/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "spectrum/Processor.h"
#include "spectrum/TapeLoader.h"
#include "spectrum/video/Screen.h"
#include "utils/Logger.h"
#include "utils/ResourceUtils.h"
#include <chrono>
#include <thread>
#ifdef __APPLE__
#include "platform/mac/MacFileOpenHandler.h"
#endif

using namespace std;
using namespace utils;

// Forward declaration if needed, or better, make load logic separate
// For now, let's keep it simple.
// We need a way to pass the file from the handler to the main loop.

static std::string g_pendingLoadFile = "";

void handleMacOpenFile(std::string path) {
  Logger::write(("Mac Open File Event: " + path).c_str());
  g_pendingLoadFile = path;
}

int main(int argc, char *argv[]) {
  try {
    std::string romFileLocation = getResourcePath("roms/48k.bin");
    bool debugMode = false;
    bool fastLoad = false;

    std::string tapeFile = "";
    std::string snapshotFile = "";

#ifdef __APPLE__
    platform::mac::installFileHandler(handleMacOpenFile);
#endif

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
      } else {
        // Positional argument - try to detect type by extension
        std::string ext = "";
        if (arg.find_last_of(".") != std::string::npos) {
          ext = arg.substr(arg.find_last_of(".") + 1);
          // To lowercase
          for (auto &c : ext)
            c = tolower(c);
        }

        if (ext == "z80" || ext == "sna") {
          snapshotFile = arg;
        } else if (ext == "tap" || ext == "tzx") {
          tapeFile = arg;
        } else if (ext == "bin" || ext == "rom") {
          romFileLocation = arg;
        }
      }
    }

    // Parse command line arguments
    // ...

    FILE *log = fopen("/tmp/zxdebug.log", "a");
    if (log) {
      fprintf(log, "ZXEmulator starting...\n");
      fprintf(log, "ROM Path: %s\n", romFileLocation.c_str());
      fclose(log);
    }

    Logger::write("Starting ZX Spectrum Emulator v0.3.5");
    Logger::write(("Loading ROM from: " + romFileLocation).c_str());

    // Create a processor and load the basic ROM
    Processor processor;
    processor.init(romFileLocation.c_str());
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

    FILE *loopLog = fopen("/tmp/zxdebug.log", "a");
    if (loopLog) {
      fprintf(loopLog, "Entering main loop\n");
      fclose(loopLog);
    }

    while (screen->processEvents()) {
      // Check for pending file load (from Drag & Drop or Mac Open Event)
      if (!g_pendingLoadFile.empty()) {
        std::string fileToLoad = g_pendingLoadFile;
        g_pendingLoadFile = "";

        Logger::write(("Loading pending file: " + fileToLoad).c_str());
        std::string fn = fileToLoad;
        std::string ext = "";
        if (fn.find_last_of(".") != std::string::npos) {
          ext = fn.substr(fn.find_last_of(".") + 1);
          for (auto &c : ext)
            c = tolower(c);
        }

        // Simple detection logic duplicated from main arg parsing
        // Ideal refactor: move 'loadFile' to Processor or Loader class
        if (ext == "tap" || ext == "tzx") {
          Tape tape = TapeLoader::load(fileToLoad.c_str());
          processor.loadTape(tape);
        } else {
          processor.loadSnapshot(fileToLoad.c_str());
        }
      }

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
