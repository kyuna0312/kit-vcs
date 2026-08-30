#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>
#include "utils/result.hpp"
#include "core/refs.hpp"
#include "core/index.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"

namespace kit {

class Repository {
public:
    static Result<void> init(const std::filesystem::path& path);
    static bool exists(const std::filesystem::path& path);
    explicit Repository(const std::filesystem::path& path);

    Result<void>        write_object(const std::string& hash, const std::string& data);
    Result<std::string> read_object(const std::string& hash) const;
    bool                object_exists(const std::string& hash) const;

    Result<Commit> read_commit(const std::string& hash) const;
    Result<Tree>   read_commit_tree(const std::string& commit_hash) const;
    // File name -> blob content for a commit's tree; empty map if hash is
    // empty or any object is missing (treated as an empty snapshot)
    std::unordered_map<std::string, std::string>
    commit_contents(const std::string& commit_hash) const;
    // Write every file of a commit's tree into the working directory
    Result<void> checkout_tree(const std::string& commit_hash) const;

    std::filesystem::path path() const;
    std::filesystem::path kit_dir() const;
    std::filesystem::path objects_dir() const;
    std::filesystem::path index_path() const;

    Refs& refs();
    const Refs& refs() const;
    Index load_index() const;
    void  save_index(const Index& idx) const;

private:
    std::filesystem::path path_;
    Refs refs_;
};

} // namespace kit
