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

#ifndef UTILS_FILEDIALOG_H
#define UTILS_FILEDIALOG_H

#include <array>
#include <cstdio>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace utils {

class FileDialog {
public:
  /**
   * Open a native Save File Dialog
   * @param title Dialog Title
   * @param defaultName Default filename
   * @return Selected path or empty string if cancelled
   */
  static std::string saveFile(const std::string &title,
                              const std::string &defaultName) {
#ifdef __APPLE__
    // macOS via AppleScript
    char buffer[1024];
    std::string result = "";

    // osascript -e 'POSIX path of (choose file name with prompt "Save Snapshot"
    // default name "snapshot.sna")'
    std::string cmd =
        "osascript -e 'POSIX path of (choose file name with prompt \"" + title +
        "\" default name \"" + defaultName + "\")' 2>/dev/null";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
      return "";

    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result = buffer;
    }
    pclose(pipe);

    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
      result.pop_back();
    }
    return result;

#elif __linux__
    // Linux via Zenity
    char buffer[1024];
    std::string result = "";

    std::string cmd =
        "zenity --file-selection --save --confirm-overwrite --title=\"" +
        title + "\" --filename=\"" + defaultName + "\" 2>/dev/null";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
      return "";

    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result = buffer;
    }
    pclose(pipe);

    if (!result.empty() && result.back() == '\n') {
      result.pop_back();
    }
    return result;

#elif _WIN32
    // Windows via Win32 API
    OPENFILENAMEA ofn;
    char szFile[260];
    strncpy(szFile, defaultName.c_str(), sizeof(szFile));

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // Console window or NULL
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter =
        "Spectrum Snapshot (*.sna)\0*.sna\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn) == TRUE) {
      return std::string(ofn.lpstrFile);
    }
    return "";
#else
    return "";
#endif
  }
};

} // namespace utils

#endif // UTILS_FILEDIALOG_H
