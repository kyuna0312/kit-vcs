#include "cmd_log.hpp"
#include "common.hpp"
#include "core/objects/commit.hpp"
#include <iostream>
#include <ctime>

namespace kit::cmd_log {

int run(int, char**) {
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;
    auto head_r = repo->refs().resolve_head();
    if (!head_r.ok() || head_r->empty()) {
        std::cout << "No commits yet.\n";
        return 0;
    }

    std::string current = *head_r;
    while (!current.empty()) {
        auto commit_r = repo->read_commit(current);
        if (!commit_r.ok()) break;
        std::time_t ts = static_cast<std::time_t>(commit_r->timestamp);
        char timebuf[64];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
                      std::localtime(&ts));
        std::cout << "commit " << current << "\n"
                  << "Author: " << commit_r->author << "\n"
                  << "Date:   " << timebuf << "\n\n"
                  << "    " << commit_r->message << "\n\n";
        current = commit_r->parent_hash;
    }
    return 0;
}

} // namespace kit::cmd_log
