#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>

namespace kit {

class Index {
public:
    void add(const std::string& path, const std::string& blob_hash);
    void remove(const std::string& path);
    bool has(const std::string& path) const;
    bool empty() const;
    void clear();

    const std::unordered_map<std::string, std::string>& entries() const;

    void load(const std::filesystem::path& index_file);
    void save(const std::filesystem::path& index_file) const;

private:
    std::unordered_map<std::string, std::string> entries_;
};

} // namespace kit
