#include "cmd_checkout.hpp"
#include "common.hpp"
#include "utils/logger.hpp"
#include <string>

namespace kit::cmd_checkout {

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit checkout <branch>");
        return 1;
    }
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;

    if (!repo->load_index().empty()) {
        logger::error("Cannot checkout: you have staged changes. Commit or stash first.");
        return 1;
    }

    std::string branch = argv[1];
    auto branch_r = repo->refs().resolve_branch(branch);
    if (!branch_r.ok()) {
        logger::error("Branch not found: " + branch);
        return 1;
    }

    auto r = repo->checkout_tree(*branch_r);
    if (!r.ok()) { logger::error(r.error); return 1; }

    auto h = repo->refs().set_head_symbolic(branch);
    if (!h.ok()) { logger::error(h.error); return 1; }
    logger::info("Switched to branch: " + branch);
    return 0;
}

} // namespace kit::cmd_checkout
