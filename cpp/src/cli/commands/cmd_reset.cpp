#include "cmd_reset.hpp"
#include "common.hpp"
#include "utils/logger.hpp"
#include <string>

namespace kit::cmd_reset {

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit reset [--soft|--mixed|--hard] <commit>");
        return 1;
    }
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;

    std::string mode = "--mixed";
    std::string target_hash;

    if (argc == 3) { mode = argv[1]; target_hash = argv[2]; }
    else            { target_hash = argv[1]; }

    if (mode != "--soft" && mode != "--mixed" && mode != "--hard") {
        logger::error("Unknown reset mode: " + mode);
        return 1;
    }

    if (target_hash == "HEAD") {
        auto r = repo->refs().resolve_head();
        if (!r.ok() || r->empty()) {
            logger::error("No commits yet.");
            return 1;
        }
        target_hash = *r;
    }

    if (!repo->object_exists(target_hash)) {
        logger::error("Commit not found: " + target_hash);
        return 1;
    }

    auto r = repo->refs().update_head_commit(target_hash);
    if (!r.ok()) { logger::error(r.error); return 1; }

    if (mode == "--soft") {
        logger::info("HEAD moved to " + target_hash.substr(0, 7) + " (soft).");
        return 0;
    }

    kit::Index empty_idx;
    repo->save_index(empty_idx);

    if (mode == "--mixed") {
        logger::info("HEAD moved, index cleared.");
        return 0;
    }

    // --hard: restore working directory
    auto c = repo->checkout_tree(target_hash);
    if (!c.ok()) { logger::error(c.error); return 1; }

    logger::info("HEAD moved, index cleared, working directory restored (hard).");
    return 0;
}

} // namespace kit::cmd_reset
