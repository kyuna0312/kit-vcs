#include "cmd_diff.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/diff.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <iostream>
#include <unordered_map>

namespace kit::cmd_diff {

int run(int, char**) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);

    // Get HEAD tree files (name -> content)
    std::unordered_map<std::string, std::string> head_files;
    auto head_r = repo.refs().resolve_head();
    if (head_r.ok() && !head_r.value.value().empty()) {
        auto commit_r = repo.read_object(head_r.value.value());
        if (commit_r.ok()) {
            auto c = kit::Commit::deserialize(commit_r.value.value());
            auto tree_r = repo.read_object(c.tree_hash);
            if (tree_r.ok()) {
                auto tree = kit::Tree::deserialize(tree_r.value.value());
                for (const auto& e : tree.entries) {
                    auto obj_r = repo.read_object(e.hash);
                    if (obj_r.ok())
                        head_files[e.name] = kit::Blob::deserialize(obj_r.value.value()).content;
                }
            }
        }
    }

    bool any_diff = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
        if (!entry.is_regular_file()) continue;
        auto rel = std::filesystem::relative(entry.path(), cwd).string();
        if (rel.rfind(".kit", 0) == 0) continue;

        std::string working_content = kit::fs::read_file(entry.path());
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
