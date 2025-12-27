#ifndef RESOURCE_UTILS_H
#define RESOURCE_UTILS_H

#include <string>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#elif _WIN32
#include <windows.h>
#elif __linux__
#include <limits.h>
#include <unistd.h>
#endif
#include <filesystem>
// #include <iostream>

namespace utils {

inline std::string getResourcePath(const std::string &relativePath) {
#ifdef __APPLE__
  char path[PATH_MAX];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0) {
    std::filesystem::path exePath(path);
    std::filesystem::path resourcePath =
        exePath.parent_path().parent_path() / "Resources" / relativePath;

    if (std::filesystem::exists(resourcePath)) {
      return resourcePath.string();
    }
  }
#elif _WIN32
  char path[MAX_PATH];
  if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
    std::filesystem::path exePath(path);
    std::filesystem::path resourcePath = exePath.parent_path() / relativePath;
    if (std::filesystem::exists(resourcePath)) {
      return resourcePath.string();
    }
  }
#elif __linux__
  char path[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
  if (count != -1) {
    path[count] = '\0';
    std::filesystem::path exePath(path);
    std::filesystem::path resourcePath = exePath.parent_path() / relativePath;
    if (std::filesystem::exists(resourcePath)) {
      return resourcePath.string();
    }
    std::filesystem::path installPath =
        std::filesystem::path("/usr/share/zxemulator/resources") / relativePath;
    if (std::filesystem::exists(installPath)) {
      return installPath.string();
    }
  }
#endif
  return relativePath;
}

} // namespace utils

#endif // RESOURCE_UTILS_H
