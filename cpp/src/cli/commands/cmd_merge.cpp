#include "cmd_merge.hpp"
#include "common.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace kit::cmd_merge {

static std::string find_common_ancestor(
    kit::Repository& repo,
    const std::string& a, const std::string& b)
{
    std::unordered_set<std::string> a_ancestors;
    std::string cur = a;
    while (!cur.empty()) {
        if (a_ancestors.count(cur)) break;
        a_ancestors.insert(cur);
        auto r = repo.read_commit(cur);
        if (!r.ok()) break;
        cur = r->parent_hash;
    }
    cur = b;
    while (!cur.empty()) {
        if (a_ancestors.count(cur)) return cur;
        auto r = repo.read_commit(cur);
        if (!r.ok()) break;
        cur = r->parent_hash;
    }
    return "";
}

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit merge <branch>");
        return 1;
    }
    auto repo = kit::cmd::open_repo();
    if (!repo) return 1;

    std::string target_branch = argv[1];
    auto target_r = repo->refs().resolve_branch(target_branch);
    if (!target_r.ok()) { logger::error("Branch not found: " + target_branch); return 1; }

    auto head_r = repo->refs().resolve_head();
    if (!head_r.ok() || head_r->empty()) {
        logger::error("No commits on current branch.");
        return 1;
    }

    std::string current_hash = *head_r;
    std::string target_hash  = *target_r;
    if (current_hash == target_hash) { logger::info("Already up to date."); return 0; }

    std::string base_hash = find_common_ancestor(*repo, current_hash, target_hash);
    auto base    = repo->commit_contents(base_hash);
    auto current = repo->commit_contents(current_hash);
    auto target  = repo->commit_contents(target_hash);

    std::unordered_set<std::string> all_files;
    for (auto& [k,_] : base)    all_files.insert(k);
    for (auto& [k,_] : current) all_files.insert(k);
    for (auto& [k,_] : target)  all_files.insert(k);

    bool conflict = false;
    kit::Index new_idx = repo->load_index();

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

        kit::fs::write_file(repo->path() / name, merged);
        kit::Blob blob{merged};
        repo->write_object(blob.hash(), blob.serialize());
        new_idx.add(name, blob.hash());
    }

    repo->save_index(new_idx);
    if (conflict) {
        logger::warn("Merge completed with conflicts. Resolve then 'kit commit'.");
    } else {
        logger::info("Merged " + target_branch + " into " + repo->refs().current_branch() + ".");
        logger::info("Review changes and run 'kit commit' to finalize.");
    }
    return conflict ? 1 : 0;
}

} // namespace kit::cmd_merge
