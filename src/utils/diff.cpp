#include "diff.hpp"
#include <sstream>
#include <algorithm>
#include <vector>

namespace kit::diff {

static std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

// LCS dynamic programming table
static std::vector<std::vector<int>> lcs_table(
    const std::vector<std::string>& a,
    const std::vector<std::string>& b)
{
    int m = (int)a.size(), n = (int)b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (int i = 1; i <= m; ++i)
        for (int j = 1; j <= n; ++j)
            dp[i][j] = (a[i-1] == b[j-1])
                ? dp[i-1][j-1] + 1
                : std::max(dp[i-1][j], dp[i][j-1]);
    return dp;
}

static void backtrack(
    const std::vector<std::vector<int>>& dp,
    const std::vector<std::string>& a,
    const std::vector<std::string>& b,
    int i, int j,
    std::vector<Hunk>& out)
{
    if (i == 0 && j == 0) return;
    if (i > 0 && j > 0 && a[i-1] == b[j-1]) {
        backtrack(dp, a, b, i-1, j-1, out);
        out.push_back({Hunk::Type::CONTEXT, a[i-1]});
    } else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
        backtrack(dp, a, b, i, j-1, out);
        out.push_back({Hunk::Type::ADD, b[j-1]});
    } else {
        backtrack(dp, a, b, i-1, j, out);
        out.push_back({Hunk::Type::REMOVE, a[i-1]});
    }
}

std::vector<Hunk> diff_lines(const std::string& old_text, const std::string& new_text) {
    auto a = split_lines(old_text);
    auto b = split_lines(new_text);
    auto dp = lcs_table(a, b);
    std::vector<Hunk> hunks;
    backtrack(dp, a, b, (int)a.size(), (int)b.size(), hunks);
    return hunks;
}

std::string format_unified(const std::string& filename, const std::vector<Hunk>& hunks) {
    bool has_changes = false;
    for (const auto& h : hunks)
        if (h.type != Hunk::Type::CONTEXT) { has_changes = true; break; }
    if (!has_changes) return "";

    std::ostringstream ss;
    ss << "--- a/" << filename << "\n+++ b/" << filename << "\n";
    for (const auto& h : hunks) {
        switch (h.type) {
            case Hunk::Type::CONTEXT: ss << " " << h.line << "\n"; break;
            case Hunk::Type::ADD:     ss << "+" << h.line << "\n"; break;
            case Hunk::Type::REMOVE:  ss << "-" << h.line << "\n"; break;
        }
    }
    return ss.str();
}

} // namespace kit::diff
