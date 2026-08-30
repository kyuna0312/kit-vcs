#include "cmd_add.hpp"
#include "common.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include <filesystem>

namespace kit::cmd_add {

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit add <file>...");
        return 1;
    }
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;
    auto idx = repo->load_index();

    for (int i = 1; i < argc; ++i) {
        std::string file = argv[i];
        if (!std::filesystem::exists(file)) {
            logger::error("File not found: " + file);
            return 1;
        }
        auto blob = kit::Blob::from_file(file);
        auto hash = blob.hash();
        if (!repo->object_exists(hash)) {
            auto r = repo->write_object(hash, blob.serialize());
            if (!r.ok()) { logger::error(r.error); return 1; }
        }
        idx.add(file, hash);
        logger::info("Staged: " + file);
    }
    repo->save_index(idx);
    return 0;
}

} // namespace kit::cmd_add
