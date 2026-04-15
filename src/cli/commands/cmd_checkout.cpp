#include "cmd_checkout.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <string>

namespace kit::cmd_checkout {

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit checkout <branch>");
        return 1;
    }
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);

    if (!repo.load_index().empty()) {
        logger::error("Cannot checkout: you have staged changes. Commit or stash first.");
        return 1;
    }

    std::string branch = argv[1];
    auto branch_r = repo.refs().resolve_branch(branch);
    if (!branch_r.ok()) {
        logger::error("Branch not found: " + branch);
        return 1;
    }

    auto commit_r = repo.read_object(branch_r.value.value());
    if (!commit_r.ok()) { logger::error(commit_r.error); return 1; }
    auto c = kit::Commit::deserialize(commit_r.value.value());

    auto tree_r = repo.read_object(c.tree_hash);
    if (!tree_r.ok()) { logger::error(tree_r.error); return 1; }
    auto tree = kit::Tree::deserialize(tree_r.value.value());

    for (const auto& e : tree.entries) {
        auto blob_r = repo.read_object(e.hash);
        if (!blob_r.ok()) { logger::error(blob_r.error); return 1; }
        auto blob = kit::Blob::deserialize(blob_r.value.value());
        kit::fs::write_file(cwd / e.name, blob.content);
    }

    auto r = repo.refs().set_head_symbolic(branch);
    if (!r.ok()) { logger::error(r.error); return 1; }
    logger::info("Switched to branch: " + branch);
    return 0;
}

} // namespace kit::cmd_checkout
