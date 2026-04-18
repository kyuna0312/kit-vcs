#pragma once
#include <string>
#include <filesystem>
#include "utils/result.hpp"
#include "core/refs.hpp"
#include "core/index.hpp"

namespace kit {

class Repository {
public:
    static Result<void> init(const std::filesystem::path& path);
    static bool exists(const std::filesystem::path& path);
    explicit Repository(const std::filesystem::path& path);

    Result<void>        write_object(const std::string& hash, const std::string& data);
    Result<std::string> read_object(const std::string& hash) const;
    bool                object_exists(const std::string& hash) const;

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
