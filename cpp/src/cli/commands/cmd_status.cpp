#include "cmd_status.hpp"
#include "common.hpp"
#include "core/objects/blob.hpp"
#include "utils/fs_utils.hpp"
#include <unordered_map>
#include <iostream>

namespace kit::cmd_status {

int run(int, char**) {
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;

    auto idx = repo->load_index();
    if (!idx.empty()) {
        std::cout << "Changes staged for commit:\n";
        for (const auto& [path, _] : idx.entries())
            std::cout << "  staged:   " << path << "\n";
    }

    // HEAD tree: name -> blob hash
    std::unordered_map<std::string, std::string> head_files;
    auto head_r = repo->refs().resolve_head();
    if (head_r.ok() && !head_r->empty()) {
        auto tree_r = repo->read_commit_tree(*head_r);
        if (tree_r.ok())
            for (const auto& e : tree_r->entries)
                head_files[e.name] = e.hash;
    }

    bool any_changes = false;
    for (const auto& [rel, abs] : kit::fs::working_files(repo->path())) {
        auto blob = kit::Blob::from_file(abs.string());
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
