#include "cmd_merge.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace kit::cmd_merge {

static std::unordered_map<std::string, std::string>
tree_files(kit::Repository& repo, const std::string& commit_hash) {
    std::unordered_map<std::string, std::string> out;
    if (commit_hash.empty()) return out;
    auto cr = repo.read_object(commit_hash);
    if (!cr.ok()) return out;
    auto c = kit::Commit::deserialize(cr.value.value());
    auto tr = repo.read_object(c.tree_hash);
    if (!tr.ok()) return out;
    auto tree = kit::Tree::deserialize(tr.value.value());
    for (const auto& e : tree.entries) {
        auto br = repo.read_object(e.hash);
        if (br.ok())
            out[e.name] = kit::Blob::deserialize(br.value.value()).content;
    }
    return out;
}

static std::string find_common_ancestor(
    kit::Repository& repo,
    const std::string& a, const std::string& b)
{
    std::unordered_set<std::string> a_ancestors;
    std::string cur = a;
    while (!cur.empty()) {
        if (a_ancestors.count(cur)) break;
        a_ancestors.insert(cur);
        auto r = repo.read_object(cur);
        if (!r.ok()) break;
        cur = kit::Commit::deserialize(r.value.value()).parent_hash;
    }
    cur = b;
    while (!cur.empty()) {
        if (a_ancestors.count(cur)) return cur;
        auto r = repo.read_object(cur);
        if (!r.ok()) break;
        cur = kit::Commit::deserialize(r.value.value()).parent_hash;
    }
    return "";
}

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit merge <branch>");
        return 1;
    }
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) { logger::error("Not a kit repository."); return 1; }
    kit::Repository repo(cwd);

    std::string target_branch = argv[1];
    auto target_r = repo.refs().resolve_branch(target_branch);
    if (!target_r.ok()) { logger::error("Branch not found: " + target_branch); return 1; }

    auto head_r = repo.refs().resolve_head();
    if (!head_r.ok() || head_r.value.value().empty()) {
        logger::error("No commits on current branch.");
        return 1;
    }

    std::string current_hash = head_r.value.value();
    std::string target_hash  = target_r.value.value();
    if (current_hash == target_hash) { logger::info("Already up to date."); return 0; }

    std::string base_hash = find_common_ancestor(repo, current_hash, target_hash);
    auto base    = tree_files(repo, base_hash);
    auto current = tree_files(repo, current_hash);
    auto target  = tree_files(repo, target_hash);

    std::unordered_set<std::string> all_files;
    for (auto& [k,_] : base)    all_files.insert(k);
    for (auto& [k,_] : current) all_files.insert(k);
    for (auto& [k,_] : target)  all_files.insert(k);

    bool conflict = false;
    kit::Index new_idx = repo.load_index();

    for (const auto& name : all_files) {
        std::string bc = base.count(name)    ? base[name]    : "";
        std::string cc = current.count(name) ? current[name] : "";
        std::string tc = target.count(name)  ? target[name]  : "";

        std::string merged;
        if (cc == tc)       merged = cc;
        else if (bc == cc)  merged = tc;
        else if (bc == tc)  merged = cc;
        else {
            merged = "<<<<<<< HEAD\n" + cc + "=======\n" + tc + ">>>>>>> " + target_branch + "\n";
            logger::warn("Conflict in: " + name);
            conflict = true;
        }

        kit::fs::write_file(cwd / name, merged);
        kit::Blob blob{merged};
        repo.write_object(blob.hash(), blob.serialize());
        new_idx.add(name, blob.hash());
    }

    repo.save_index(new_idx);
    if (conflict) {
        logger::warn("Merge completed with conflicts. Resolve then 'kit commit'.");
    } else {
        logger::info("Merged " + target_branch + " into " + repo.refs().current_branch() + ".");
        logger::info("Review changes and run 'kit commit' to finalize.");
    }
    return conflict ? 1 : 0;
}

} // namespace kit::cmd_merge
