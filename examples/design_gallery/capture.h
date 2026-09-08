#pragma once
#include <filesystem>
#include <vector>
#include <string>

namespace imkit::design {
void SaveBackbuffer(const std::filesystem::path& path, int width, int height);
void MakeContactSheet(const std::vector<std::filesystem::path>& paths,
                      const std::filesystem::path& destination);
}
