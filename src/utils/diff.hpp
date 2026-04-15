#pragma once
#include <string>
#include <vector>

namespace kit::diff {

struct Hunk {
    enum class Type { CONTEXT, ADD, REMOVE };
    Type type;
    std::string line;
};

// Compute line-level diff between old_text and new_text
std::vector<Hunk> diff_lines(const std::string& old_text, const std::string& new_text);

// Format hunks as unified diff with filename header; returns "" if no changes
std::string format_unified(const std::string& filename, const std::vector<Hunk>& hunks);

} // namespace kit::diff
