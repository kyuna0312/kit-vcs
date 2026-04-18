#include "cmd_log.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "utils/logger.hpp"
#include <filesystem>
#include <iostream>
#include <ctime>

namespace kit::cmd_log {

int run(int, char**) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);
    auto head_r = repo.refs().resolve_head();
    if (!head_r.ok() || head_r.value.value().empty()) {
        std::cout << "No commits yet.\n";
        return 0;
    }

    std::string current = head_r.value.value();
    while (!current.empty()) {
        auto obj_r = repo.read_object(current);
        if (!obj_r.ok()) break;
        auto c = kit::Commit::deserialize(obj_r.value.value());
        std::time_t ts = static_cast<std::time_t>(c.timestamp);
        char timebuf[64];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
                      std::localtime(&ts));
        std::cout << "commit " << current << "\n"
                  << "Author: " << c.author << "\n"
                  << "Date:   " << timebuf << "\n\n"
                  << "    " << c.message << "\n\n";
        current = c.parent_hash;
    }
    return 0;
}

} // namespace kit::cmd_log
