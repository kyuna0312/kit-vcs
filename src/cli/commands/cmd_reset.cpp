#include "cmd_reset.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <string>

namespace kit::cmd_reset {

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit reset [--soft|--mixed|--hard] <commit>");
        return 1;
    }
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) { logger::error("Not a kit repository."); return 1; }
    kit::Repository repo(cwd);

    std::string mode = "--mixed";
    std::string target_hash;

    if (argc == 3) { mode = argv[1]; target_hash = argv[2]; }
    else            { target_hash = argv[1]; }

    if (mode != "--soft" && mode != "--mixed" && mode != "--hard") {
        logger::error("Unknown reset mode: " + mode);
        return 1;
    }

    if (target_hash == "HEAD") {
        auto r = repo.refs().resolve_head();
        if (!r.ok() || r.value.value().empty()) {
            logger::error("No commits yet.");
            return 1;
        }
        target_hash = r.value.value();
    }

    if (!repo.object_exists(target_hash)) {
        logger::error("Commit not found: " + target_hash);
        return 1;
    }

    auto r = repo.refs().update_head_commit(target_hash);
    if (!r.ok()) { logger::error(r.error); return 1; }

    if (mode == "--soft") {
        logger::info("HEAD moved to " + target_hash.substr(0, 7) + " (soft).");
        return 0;
    }

    kit::Index empty_idx;
    repo.save_index(empty_idx);

    if (mode == "--mixed") {
        logger::info("HEAD moved, index cleared.");
        return 0;
    }

    // --hard: restore working directory
    auto commit_r = repo.read_object(target_hash);
    if (!commit_r.ok()) { logger::error(commit_r.error); return 1; }
    auto c = kit::Commit::deserialize(commit_r.value.value());
    auto tree_r = repo.read_object(c.tree_hash);
    if (!tree_r.ok()) { logger::error(tree_r.error); return 1; }
    auto tree = kit::Tree::deserialize(tree_r.value.value());

    for (const auto& e : tree.entries) {
        auto blob_r = repo.read_object(e.hash);
        if (!blob_r.ok()) { logger::error(blob_r.error); return 1; }
        kit::fs::write_file(cwd / e.name,
                            kit::Blob::deserialize(blob_r.value.value()).content);
    }

    logger::info("HEAD moved, index cleared, working directory restored (hard).");
    return 0;
}

} // namespace kit::cmd_reset
