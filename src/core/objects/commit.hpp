#pragma once
#include <string>
#include <cstdint>

namespace kit {

struct Commit {
    std::string tree_hash;
    std::string parent_hash;  // empty for root commit
    std::string author;
    int64_t     timestamp{0};
    std::string message;

    std::string serialize() const;
    std::string hash() const;
    static Commit deserialize(const std::string& raw);
    // Resolve author from KIT_AUTHOR env var, fallback to whoami
    static std::string resolve_author();
};

} // namespace kit
