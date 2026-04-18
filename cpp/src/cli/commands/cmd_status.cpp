#include "cmd_status.hpp"
#include "core/repository.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include <filesystem>
#include <unordered_map>
#include <iostream>

namespace kit::cmd_status {

int run(int, char**) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);

    // Staged files
    auto idx = repo.load_index();
    if (!idx.empty()) {
        std::cout << "Changes staged for commit:\n";
        for (const auto& [path, _] : idx.entries())
            std::cout << "  staged:   " << path << "\n";
    }

    // Head tree files for comparison
    std::unordered_map<std::string, std::string> head_files; // name -> blob hash
    auto head_r = repo.refs().resolve_head();
    if (head_r.ok() && !head_r.value.value().empty()) {
        auto commit_r = repo.read_object(head_r.value.value());
        if (commit_r.ok()) {
            auto c = kit::Commit::deserialize(commit_r.value.value());
            auto tree_r = repo.read_object(c.tree_hash);
            if (tree_r.ok()) {
                auto tree = kit::Tree::deserialize(tree_r.value.value());
                for (const auto& e : tree.entries)
                    head_files[e.name] = e.hash;
            }
        }
    }

    bool any_changes = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
        if (!entry.is_regular_file()) continue;
        auto rel = std::filesystem::relative(entry.path(), cwd).string();
        if (rel.rfind(".kit", 0) == 0) continue;
        auto blob = kit::Blob::from_file(entry.path().string());
        if (head_files.count(rel)) {
            if (head_files[rel] != blob.hash()) {
                std::cout << "  modified: " << rel << "\n";
                any_changes = true;
            }
        } else if (!idx.has(rel)) {
            std::cout << "  untracked: " << rel << "\n";
            any_changes = true;
        }
    }

    if (idx.empty() && !any_changes)
        std::cout << "Nothing to commit, working directory clean.\n";
    return 0;
}

} // namespace kit::cmd_status
