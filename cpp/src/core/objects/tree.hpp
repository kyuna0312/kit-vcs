#pragma once
#include <string>
#include <vector>

namespace kit {

struct TreeEntry {
    std::string mode;  // "blob"
    std::string hash;
    std::string name;
};

struct Tree {
    std::vector<TreeEntry> entries;

    std::string serialize() const;  // "blob <hash> <name>\n" per entry
    std::string hash() const;       // SHA1 of serialize()
    static Tree deserialize(const std::string& raw);
};

} // namespace kit
