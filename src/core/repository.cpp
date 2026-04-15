#include "repository.hpp"
#include "utils/fs_utils.hpp"
#include <stdexcept>

namespace kit {

Result<void> Repository::init(const std::filesystem::path& path) {
    auto kit = path / ".kit";
    if (std::filesystem::exists(kit))
        return Result<void>::failure("Repository already initialized.");
    try {
        kit::fs::ensure_dir(kit / "objects");
        kit::fs::ensure_dir(kit / "refs" / "heads");
        kit::fs::write_file(kit / "HEAD", "ref: refs/heads/master\n");
        kit::fs::write_file(kit / "index", "");
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

bool Repository::exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path / ".kit");
}

Repository::Repository(const std::filesystem::path& path)
    : path_(path), refs_(path / ".kit") {
    if (!exists(path))
        throw std::runtime_error("Not a kit repository: " + path.string());
}

Result<void> Repository::write_object(const std::string& hash, const std::string& data) {
    try {
        kit::fs::write_file(objects_dir() / hash, data);
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

Result<std::string> Repository::read_object(const std::string& hash) const {
    auto p = objects_dir() / hash;
    if (!std::filesystem::exists(p))
        return Result<std::string>::failure("Object not found: " + hash);
    try {
        return Result<std::string>::success(kit::fs::read_file(p));
    } catch (const std::exception& e) {
        return Result<std::string>::failure(e.what());
    }
}

bool Repository::object_exists(const std::string& hash) const {
    return std::filesystem::exists(objects_dir() / hash);
}

std::filesystem::path Repository::path() const { return path_; }
std::filesystem::path Repository::kit_dir() const { return path_ / ".kit"; }
std::filesystem::path Repository::objects_dir() const { return path_ / ".kit" / "objects"; }
std::filesystem::path Repository::index_path() const { return path_ / ".kit" / "index"; }

Refs& Repository::refs() { return refs_; }
const Refs& Repository::refs() const { return refs_; }

Index Repository::load_index() const {
    Index idx;
    idx.load(index_path());
    return idx;
}

void Repository::save_index(const Index& idx) const {
    idx.save(index_path());
}

} // namespace kit
