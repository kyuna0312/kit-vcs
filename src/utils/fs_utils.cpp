#include "fs_utils.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace kit::fs {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path.string());
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void write_file(const std::filesystem::path& path, const std::string& content) {
    ensure_dir(path.parent_path());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot write file: " + path.string());
    f << content;
}

void ensure_dir(const std::filesystem::path& path) {
    if (!path.empty())
        std::filesystem::create_directories(path);
}

} // namespace kit::fs
