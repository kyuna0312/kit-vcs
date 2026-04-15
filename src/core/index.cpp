#include "index.hpp"
#include "utils/fs_utils.hpp"
#include <sstream>

namespace kit {

void Index::add(const std::string& path, const std::string& blob_hash) {
    entries_[path] = blob_hash;
}
void Index::remove(const std::string& path) { entries_.erase(path); }
bool Index::has(const std::string& path) const { return entries_.count(path) > 0; }
bool Index::empty() const { return entries_.empty(); }
void Index::clear() { entries_.clear(); }

const std::unordered_map<std::string, std::string>& Index::entries() const {
    return entries_;
}

void Index::load(const std::filesystem::path& p) {
    entries_.clear();
    if (!std::filesystem::exists(p)) return;
    std::string raw = kit::fs::read_file(p);
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        auto sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string hash = line.substr(0, sp);
        std::string path = line.substr(sp + 1);
        entries_[path] = hash;
    }
}

void Index::save(const std::filesystem::path& p) const {
    std::ostringstream ss;
    for (const auto& [path, hash] : entries_)
        ss << hash << " " << path << "\n";
    kit::fs::write_file(p, ss.str());
}

} // namespace kit
