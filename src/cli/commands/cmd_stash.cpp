#include "cmd_stash.hpp"
#include "core/repository.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <string>
#include <sstream>

namespace kit::cmd_stash {

static std::filesystem::path stash_file(const std::filesystem::path& kit_dir) {
    return kit_dir / "stash" / "stash0";
}

int run(int argc, char** argv) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) { logger::error("Not a kit repository."); return 1; }
    kit::Repository repo(cwd);

    bool is_pop = (argc >= 2 && std::string(argv[1]) == "pop");

    if (is_pop) {
        auto sf = stash_file(repo.kit_dir());
        if (!std::filesystem::exists(sf)) {
            logger::error("No stash to pop.");
            return 1;
        }
        std::string raw = kit::fs::read_file(sf);
        std::istringstream ss(raw);
        std::string line;
        std::string current_path;
        std::ostringstream content;
        bool reading_content = false;

        auto flush = [&]() {
            if (!current_path.empty())
                kit::fs::write_file(cwd / current_path, content.str());
        };

        while (std::getline(ss, line)) {
            if (line == "---") {
                flush();
                current_path.clear();
                content.str("");
                reading_content = false;
            } else if (!reading_content && current_path.empty()) {
                current_path = line;
                reading_content = true;
            } else {
                if (!content.str().empty()) content << "\n";
                content << line;
            }
        }
        flush();
        std::filesystem::remove(sf);
        logger::info("Stash restored.");
        return 0;
    }

    // Save working dir to stash
    auto sf = stash_file(repo.kit_dir());
    kit::fs::ensure_dir(sf.parent_path());

    std::ostringstream stash_content;
    bool any = false;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
        if (!entry.is_regular_file()) continue;
        auto rel = std::filesystem::relative(entry.path(), cwd).string();
        if (rel.rfind(".kit", 0) == 0) continue;
        std::string content = kit::fs::read_file(entry.path());
        stash_content << rel << "\n" << content << "\n---\n";
        any = true;
    }

    if (!any) { logger::info("Nothing to stash."); return 0; }

    kit::fs::write_file(sf, stash_content.str());
    logger::info("Changes stashed.");
    return 0;
}

} // namespace kit::cmd_stash
