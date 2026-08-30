#include "cmd_branch.hpp"
#include "common.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <string>

namespace kit::cmd_branch {

int run(int argc, char** argv) {
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;
    auto& refs = repo->refs();

    if (argc == 1) {
        auto current = refs.current_branch();
        for (const auto& b : refs.list_branches())
            std::cout << (b == current ? "* " : "  ") << b << "\n";
        return 0;
    }

    std::string flag = argv[1];
    if (flag == "-d" && argc == 3) {
        auto r = refs.delete_branch(argv[2]);
        if (!r.ok()) { logger::error(r.error); return 1; }
        logger::info("Deleted branch: " + std::string(argv[2]));
        return 0;
    }

    // Create branch at HEAD
    std::string name = argv[1];
    auto head_r = refs.resolve_head();
    if (!head_r.ok() || head_r->empty()) {
        logger::error("No commits yet — cannot create branch.");
        return 1;
    }
    auto r = refs.create_branch(name, *head_r);
    if (!r.ok()) { logger::error(r.error); return 1; }
    logger::info("Created branch: " + name);
    return 0;
}

} // namespace kit::cmd_branch
