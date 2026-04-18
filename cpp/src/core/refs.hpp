#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "utils/result.hpp"

namespace kit {

class Refs {
public:
    explicit Refs(const std::filesystem::path& kit_dir);

    std::string current_branch() const;  // empty if detached HEAD
    Result<std::string> resolve_head() const;
    Result<std::string> resolve_branch(const std::string& name) const;

    Result<void> set_head_symbolic(const std::string& branch);
    Result<void> update_head_commit(const std::string& commit_hash);
    Result<void> create_branch(const std::string& name, const std::string& commit_hash);
    Result<void> update_branch(const std::string& name, const std::string& commit_hash);
    Result<void> delete_branch(const std::string& name);

    std::vector<std::string> list_branches() const;

private:
    std::filesystem::path kit_dir_;
    std::filesystem::path head_path() const;
    std::filesystem::path branch_path(const std::string& name) const;
};

} // namespace kit
