#include "refs.hpp"
#include "utils/fs_utils.hpp"

namespace kit {

Refs::Refs(const std::filesystem::path& kit_dir) : kit_dir_(kit_dir) {}

std::filesystem::path Refs::head_path() const { return kit_dir_ / "HEAD"; }
std::filesystem::path Refs::branch_path(const std::string& name) const {
    return kit_dir_ / "refs" / "heads" / name;
}

std::string Refs::current_branch() const {
    try {
        std::string head = kit::fs::read_file(head_path());
        if (!head.empty() && head.back() == '\n') head.pop_back();
        const std::string prefix = "ref: refs/heads/";
        if (head.rfind(prefix, 0) == 0) return head.substr(prefix.size());
    } catch (...) {}
    return "";
}

Result<std::string> Refs::resolve_head() const {
    try {
        std::string head = kit::fs::read_file(head_path());
        if (!head.empty() && head.back() == '\n') head.pop_back();
        const std::string prefix = "ref: refs/heads/";
        if (head.rfind(prefix, 0) == 0)
            return resolve_branch(head.substr(prefix.size()));
        if (head.empty()) return Result<std::string>::success("");
        return Result<std::string>::success(head); // bare hash (detached)
    } catch (const std::exception& e) {
        return Result<std::string>::failure(e.what());
    }
}

Result<std::string> Refs::resolve_branch(const std::string& name) const {
    auto p = branch_path(name);
    if (!std::filesystem::exists(p))
        return Result<std::string>::failure("Branch not found: " + name);
    try {
        std::string hash = kit::fs::read_file(p);
        if (!hash.empty() && hash.back() == '\n') hash.pop_back();
        return Result<std::string>::success(hash);
    } catch (const std::exception& e) {
        return Result<std::string>::failure(e.what());
    }
}

Result<void> Refs::set_head_symbolic(const std::string& branch) {
    try {
        kit::fs::write_file(head_path(), "ref: refs/heads/" + branch + "\n");
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

Result<void> Refs::update_head_commit(const std::string& commit_hash) {
    std::string branch = current_branch();
    if (!branch.empty()) return update_branch(branch, commit_hash);
    try {
        kit::fs::write_file(head_path(), commit_hash + "\n");
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

Result<void> Refs::create_branch(const std::string& name, const std::string& commit_hash) {
    try {
        kit::fs::write_file(branch_path(name), commit_hash + "\n");
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

Result<void> Refs::update_branch(const std::string& name, const std::string& commit_hash) {
    return create_branch(name, commit_hash);
}

Result<void> Refs::delete_branch(const std::string& name) {
    auto p = branch_path(name);
    if (!std::filesystem::exists(p))
        return Result<void>::failure("Branch not found: " + name);
    std::filesystem::remove(p);
    return Result<void>::ok_result();
}

std::vector<std::string> Refs::list_branches() const {
    std::vector<std::string> out;
    auto heads = kit_dir_ / "refs" / "heads";
    if (!std::filesystem::exists(heads)) return out;
    for (const auto& e : std::filesystem::directory_iterator(heads))
        if (e.is_regular_file())
            out.push_back(e.path().filename().string());
    return out;
}

} // namespace kit
