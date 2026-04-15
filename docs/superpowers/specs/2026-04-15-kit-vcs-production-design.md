# kit-vcs → Production Redesign

**Date:** 2026-04-15
**Status:** Approved

## Overview

Transform kit-vcs from an educational demo into a production-quality local version control system. The project keeps the `kit` name and CLI. The work proceeds in two phases:

- **Phase 1:** Architecture refactor — proper object model, `.cpp`/`.hpp` split, clean namespaces, centralized logger
- **Phase 2:** Complete missing features on top of the clean architecture

Approach: incremental refactor (never break the working state; tests pass throughout).

---

## Directory & File Structure

```
kit-vcs/
├── src/
│   ├── core/
│   │   ├── objects/
│   │   │   ├── blob.cpp / blob.hpp
│   │   │   ├── tree.cpp / tree.hpp
│   │   │   └── commit.cpp / commit.hpp
│   │   ├── index.cpp / index.hpp
│   │   ├── repository.cpp / repository.hpp
│   │   └── refs.cpp / refs.hpp
│   ├── cli/
│   │   ├── commands/
│   │   │   ├── cmd_init.cpp / cmd_init.hpp
│   │   │   ├── cmd_add.cpp / cmd_add.hpp
│   │   │   ├── cmd_commit.cpp / cmd_commit.hpp
│   │   │   ├── cmd_status.cpp / cmd_status.hpp
│   │   │   ├── cmd_log.cpp / cmd_log.hpp
│   │   │   ├── cmd_branch.cpp / cmd_branch.hpp
│   │   │   ├── cmd_checkout.cpp / cmd_checkout.hpp
│   │   │   ├── cmd_merge.cpp / cmd_merge.hpp
│   │   │   ├── cmd_diff.cpp / cmd_diff.hpp
│   │   │   ├── cmd_reset.cpp / cmd_reset.hpp
│   │   │   └── cmd_stash.cpp / cmd_stash.hpp
│   │   └── cli.cpp / cli.hpp
│   ├── utils/
│   │   ├── hash.cpp / hash.hpp
│   │   ├── logger.cpp / logger.hpp
│   │   ├── fs_utils.cpp / fs_utils.hpp
│   │   ├── diff.cpp / diff.hpp
│   │   └── constants.hpp
│   └── main.cpp
├── tests/
│   ├── unit/
│   │   ├── test_blob.cpp
│   │   ├── test_tree.cpp
│   │   ├── test_commit.cpp
│   │   ├── test_index.cpp
│   │   └── test_refs.cpp
│   └── integration/
│       ├── test_init.cpp
│       ├── test_add_commit.cpp
│       ├── test_branch_checkout.cpp
│       ├── test_merge.cpp
│       └── test_diff.cpp
└── CMakeLists.txt
```

---

## Object Model

All objects are content-addressed, stored as `.kit/objects/<sha1>`.

### Blob
Stores raw file content. Hash input: `"blob <size>\0<content>"`.

```
.kit/objects/<sha1>   ← raw file bytes
```

### Tree
Stores a directory snapshot as newline-delimited entries:
```
blob <hash> <filename>
blob <hash> <filename>
```

### Commit
```
tree <hash>
parent <hash>       ← omitted on first commit
author <name>       ← from KIT_AUTHOR env var, fallback to whoami
timestamp <unix_time>
message <text>
```

### Refs
- `.kit/refs/heads/<branch>` — contains the commit hash the branch points to
- `.kit/HEAD` — either `ref: refs/heads/master` (symbolic) or a bare hash (detached HEAD)

### Index (staging area)
`.kit/index` — flat file of `<blob_hash> <filepath>` entries, one per line.

### Add → Commit flow
```
kit add file.txt
  → SHA1(file.txt content) → write .kit/objects/<hash>  (blob)
  → append "<hash> file.txt" to .kit/index

kit commit -m "msg"
  → read index → serialize tree entries → SHA1 → write tree object
  → build commit (tree_hash + parent + msg + timestamp) → SHA1 → write commit object
  → update HEAD ref → clear index
```

---

## Missing Features

### `kit branch`
- `kit branch` — list `.kit/refs/heads/`, mark current branch
- `kit branch <name>` — create: write HEAD commit hash to `.kit/refs/heads/<name>`
- `kit branch -d <name>` — delete `.kit/refs/heads/<name>`

### `kit checkout <branch>`
- Resolve branch → commit → tree
- Restore each blob from tree to working directory
- Update HEAD to `ref: refs/heads/<branch>`
- Refuse if index is non-empty (dirty state guard)

### `kit diff`
- Compare working directory files vs HEAD tree
- Unified diff format (`+` added, `-` removed)
- Line-level LCS diff implemented in `utils/diff.cpp` — no external tool dependency

### `kit merge <branch>`
- Walk parent chains to find common ancestor commit
- Three-way merge at file level:
  - Only one side changed → take that side
  - Both sides changed → insert conflict markers (`<<<<<<< / ======= / >>>>>>>`)
- Write merged files to working directory, stage them; user resolves conflicts and commits

### `kit reset <commit>`
- `--soft` — move HEAD only, keep index and working dir
- `--mixed` (default) — move HEAD, clear index, keep working dir
- `--hard` — move HEAD, clear index, restore working dir from target commit tree

### `kit stash`
- Keep existing behavior, move to `cmd_stash`
- `kit stash` — save working dir changes to `.kit/stash/`, restore clean state
- `kit stash pop` — restore from stash

---

## Logger & Error Handling

### Logger (`utils/logger.hpp/.cpp`)
```cpp
namespace logger {
    enum class Level { DEBUG, INFO, WARN, ERROR };
    void set_level(Level l);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);  // writes to stderr
}
```

- Controlled by `KIT_LOG_LEVEL` env var or `--verbose` CLI flag
- `DEBUG` off by default — replaces all `#ifdef DEBUG` and `std::cout << "[DEBUG]"` patterns
- `error_handler::print_error()` → `logger::error()`

### Error Handling Strategy
- **Core layer** (`core/objects/`, `core/repository`) — throws `std::runtime_error` on failure
- **CLI layer** — catches at command boundary, calls `logger::error()`, exits non-zero
- No silent failures

### Result type for fallible core operations
```cpp
template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    bool ok() const { return value.has_value(); }
};
```
Used by: `repository::read_object()`, `repository::resolve_ref()`, `index::load()`.

---

## Testing Strategy

### Unit tests (`tests/unit/`)
Test core objects in isolation, no filesystem:

| File | Tests |
|------|-------|
| `test_blob.cpp` | hash computation, serialize/deserialize |
| `test_tree.cpp` | entry parsing, tree building |
| `test_commit.cpp` | serialization, parent chain parsing |
| `test_index.cpp` | add/remove entries, read/write |
| `test_refs.cpp` | resolve HEAD, read/write branch refs |

### Integration tests (`tests/integration/`)
Spawn `kit` binary as subprocess against a temp directory:

```cpp
TEST(InitTest, CreatesKitDirectory) {
    TempDir dir;
    auto result = run_kit({"init"});
    EXPECT_EQ(result.exit_code, 0);
    EXPECT_TRUE(fs::exists(dir.path() / ".kit" / "HEAD"));
}
```

- Each test gets a fresh `TempDir` (RAII cleanup)
- `run_kit()` helper captures stdout/stderr, returns exit code
- Full workflow coverage: init → add → commit → log, branch → checkout → merge

### CMake
```cmake
add_executable(test_unit        tests/unit/...)
add_executable(test_integration tests/integration/...)
add_test(NAME unit        COMMAND test_unit)
add_test(NAME integration COMMAND test_integration)
```

---

## Implementation Phases

### Phase 1 — Architecture Refactor
1. Introduce `utils/logger`, `utils/hash`, `utils/fs_utils`
2. Implement `core/objects/blob`, `core/objects/tree`, `core/objects/commit`
3. Implement `core/index`, `core/refs`, `core/repository`
4. Move CLI commands to `src/cli/commands/cmd_*.cpp`
5. Update `CMakeLists.txt` for new layout
6. Port existing tests to GTest unit tests; delete `tests/test_kit_vcs.cpp` and `tests/test_commands.cpp`

### Phase 2 — Feature Completion
1. `kit branch` (create, list, delete)
2. `kit checkout`
3. `kit diff` (with LCS implementation)
4. `kit merge` (three-way)
5. `kit reset` (soft/mixed/hard)
6. `kit stash pop`
7. Integration test suite
