#include "cmd_init.hpp"
#include "core/repository.hpp"
#include "utils/logger.hpp"
#include <filesystem>

namespace kit::cmd_init {

int run(int, char**) {
    auto result = kit::Repository::init(std::filesystem::current_path());
    if (!result.ok()) {
        logger::error(result.error);
        return 1;
    }
    logger::info("Initialized empty kit repository in .kit/");
    return 0;
}

} // namespace kit::cmd_init
