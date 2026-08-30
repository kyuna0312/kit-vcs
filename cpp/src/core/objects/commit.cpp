#include "commit.hpp"
#include "utils/hash.hpp"
#include <sstream>
#include <cstdlib>

namespace kit {

std::string Commit::serialize() const {
    std::ostringstream ss;
    ss << "tree "      << tree_hash   << "\n";
    if (!parent_hash.empty())
        ss << "parent " << parent_hash << "\n";
    ss << "author "    << author      << "\n";
    ss << "timestamp " << timestamp   << "\n";
    ss << "\n"         << message;
    return ss.str();
}

std::string Commit::hash() const {
    return kit::hash::sha1(serialize());
}

Commit Commit::deserialize(const std::string& raw) {
    Commit c;
    std::istringstream ss(raw);
    std::string line;
    bool in_message = false;
    std::ostringstream msg;
    while (std::getline(ss, line)) {
        if (in_message) {
            if (!msg.str().empty()) msg << "\n";
            msg << line;
            continue;
        }
        if (line.empty()) { in_message = true; continue; }
        auto sp = line.find(' ');
        std::string key = line.substr(0, sp);
        std::string val = (sp != std::string::npos) ? line.substr(sp + 1) : "";
        if      (key == "tree")      c.tree_hash   = val;
        else if (key == "parent")    c.parent_hash = val;
        else if (key == "author")    c.author      = val;
        else if (key == "timestamp") c.timestamp   = std::stoll(val);
    }
    c.message = msg.str();
    return c;
}

std::string Commit::resolve_author() {
    for (const char* var : {"KIT_AUTHOR", "USER", "USERNAME"})
        if (const char* v = std::getenv(var); v && *v)
            return v;
    return "unknown";
}

} // namespace kit
