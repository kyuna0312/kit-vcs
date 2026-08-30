#pragma once
#include "core/repository.hpp"
#include "utils/logger.hpp"
#include <filesystem>
#include <optional>

namespace kit::cmd {

// Open the repository in the current directory, logging an error if absent
inline std::optional<Repository> open_repo() {
    auto cwd = std::filesystem::current_path();
    if (!Repository::exists(cwd)) {
        logger::error("Not a kit repository. Run 'kit init' first.");
        return std::nullopt;
    }
    return Repository(cwd);
}

} // namespace kit::cmd
