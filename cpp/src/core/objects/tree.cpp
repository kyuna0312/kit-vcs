#include "tree.hpp"
#include "utils/hash.hpp"
#include <sstream>

namespace kit {

std::string Tree::serialize() const {
    std::ostringstream ss;
    for (const auto& e : entries)
        ss << e.mode << " " << e.hash << " " << e.name << "\n";
    return ss.str();
}

std::string Tree::hash() const {
    return kit::hash::sha1(serialize());
}

Tree Tree::deserialize(const std::string& raw) {
    Tree t;
    if (raw.empty()) return t;
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        TreeEntry e;
        ls >> e.mode >> e.hash >> e.name;
        t.entries.push_back(e);
    }
    return t;
}

} // namespace kit
