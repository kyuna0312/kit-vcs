#pragma once
#include <string>
#include <utility>
#include <vector>
#include <filesystem>

namespace kit::fs {
    // Read entire file; throws std::runtime_error on failure
    std::string read_file(const std::filesystem::path& path);
    // Write content to path, creating parent dirs as needed
    void write_file(const std::filesystem::path& path, const std::string& content);
    // Create directory and all parents; no-op if exists
    void ensure_dir(const std::filesystem::path& path);
    // Regular files under root as (relative path, absolute path), skipping .kit/
    std::vector<std::pair<std::string, std::filesystem::path>>
    working_files(const std::filesystem::path& root);
} // namespace kit::fs
