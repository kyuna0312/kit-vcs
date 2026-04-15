#pragma once
#include <string>
#include <filesystem>

namespace kit::fs {
    // Read entire file; throws std::runtime_error on failure
    std::string read_file(const std::filesystem::path& path);
    // Write content to path, creating parent dirs as needed
    void write_file(const std::filesystem::path& path, const std::string& content);
    // Create directory and all parents; no-op if exists
    void ensure_dir(const std::filesystem::path& path);
} // namespace kit::fs
