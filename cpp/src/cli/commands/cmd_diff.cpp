#include "cmd_diff.hpp"
#include "common.hpp"
#include "utils/diff.hpp"
#include "utils/fs_utils.hpp"
#include <iostream>
#include <unordered_map>

namespace kit::cmd_diff {

int run(int, char**) {
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;

    std::unordered_map<std::string, std::string> head_files;
    auto head_r = repo->refs().resolve_head();
    if (head_r.ok()) head_files = repo->commit_contents(*head_r);

    bool any_diff = false;
    for (const auto& [rel, abs] : kit::fs::working_files(repo->path())) {
        std::string working_content = kit::fs::read_file(abs);
        std::string head_content = head_files.count(rel) ? head_files[rel] : "";

        if (working_content == head_content) continue;

        auto hunks = kit::diff::diff_lines(head_content, working_content);
        auto formatted = kit::diff::format_unified(rel, hunks);
        if (!formatted.empty()) {
            std::cout << formatted;
            any_diff = true;
        }
    }

    if (!any_diff) std::cout << "No differences.\n";
    return 0;
}

} // namespace kit::cmd_diff
