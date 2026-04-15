#include "cmd_commit.hpp"
#include "core/repository.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/commit.hpp"
#include "utils/logger.hpp"
#include <filesystem>
#include <ctime>
#include <string>

namespace kit::cmd_commit {

int run(int argc, char** argv) {
    std::string message;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-m") {
            message = argv[i + 1];
            break;
        }
    }
    if (message.empty()) {
        logger::error("Usage: kit commit -m <message>");
        return 1;
    }

    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);
    auto idx = repo.load_index();

    if (idx.empty()) {
        logger::info("Nothing to commit. Stage files with 'kit add'.");
        return 0;
    }

    // Build tree from index
    kit::Tree tree;
    for (const auto& [path, hash] : idx.entries())
        tree.entries.push_back({"blob", hash, path});

    auto tree_hash = tree.hash();
    repo.write_object(tree_hash, tree.serialize());

    // Get parent commit hash
    std::string parent_hash;
    auto head_r = repo.refs().resolve_head();
    if (head_r.ok() && !head_r.value.value().empty())
        parent_hash = head_r.value.value();

    // Build commit
    kit::Commit commit;
    commit.tree_hash   = tree_hash;
    commit.parent_hash = parent_hash;
    commit.author      = kit::Commit::resolve_author();
    commit.timestamp   = static_cast<int64_t>(std::time(nullptr));
    commit.message     = message;

    auto commit_hash = commit.hash();
    repo.write_object(commit_hash, commit.serialize());

    // Update HEAD
    auto r = repo.refs().update_head_commit(commit_hash);
    if (!r.ok()) { logger::error(r.error); return 1; }

    // Clear index
    idx.clear();
    repo.save_index(idx);

    logger::info("Committed: [" + commit_hash.substr(0, 7) + "] " + message);
    return 0;
}

} // namespace kit::cmd_commit
