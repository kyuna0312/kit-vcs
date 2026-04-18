# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
mkdir cpp/build && cd cpp/build
cmake ..
make
```

Binary output: `cpp/build/kit-vcs`

On Linux, OpenSSL is found via system packages. The `CMakeLists.txt` hardcodes macOS Homebrew paths — override if needed:
```bash
cmake .. -DOPENSSL_ROOT_DIR=/usr
```

## Run Tests

```bash
cd cpp/build
ctest          # run all tests via CTest
./test_unit    # run unit tests directly for verbose output
./test_integration  # run integration tests
```

Tests use Google Test (fetched via CMake FetchContent). Unit tests live in `cpp/tests/unit/` and integration tests in `cpp/tests/integration/`. Each test function operates on a temporary `.kit/` directory created in the working directory.

## Architecture

### Object Storage Model

`.kit/` mirrors Git's object store:
- `objects/` — blob, tree, and commit objects stored as flat files named by SHA1 hash
- `refs/heads/` — branch refs, one file per branch containing commit hash
- `HEAD` — either a ref string (`ref: refs/heads/master`) or a bare commit hash
- `index` — staged file entries (path + blob hash pairs)
- `stash/` — stashed snapshots

### Code Layout

The codebase uses **split compilation**: each module has a `.hpp` declaration and a `.cpp` definition. All source lives under `cpp/src/`.

| Path | Role |
|------|------|
| `cpp/src/main.cpp` | Entry point — calls `kit::cli::run()` |
| `cpp/src/cli/cli.hpp` / `cli.cpp` | `kit::cli` namespace — command dispatch, routes argv to per-command `run()` |
| `cpp/src/cli/commands/cmd_*.hpp` / `cmd_*.cpp` | Per-command logic: `add`, `branch`, `checkout`, `commit`, `diff`, `init`, `log`, `merge`, `reset`, `stash`, `status` |
| `cpp/src/core/repository.hpp` / `repository.cpp` | `kit::Repository` class — opens/inits the `.kit/` store, reads/writes objects, manages index and refs |
| `cpp/src/core/refs.hpp` / `refs.cpp` | `kit::Refs` — reads and writes `HEAD` and `refs/heads/` branch pointers |
| `cpp/src/core/index.hpp` / `index.cpp` | `kit::Index` — staged file list (path → blob hash) |
| `cpp/src/core/objects/blob.hpp` / `blob.cpp` | `kit::Blob` — file content object |
| `cpp/src/core/objects/tree.hpp` / `tree.cpp` | `kit::Tree` / `TreeEntry` — directory snapshot (list of blob hash + filename entries) |
| `cpp/src/core/objects/commit.hpp` / `commit.cpp` | `kit::Commit` — commit object (tree hash, parent, author, timestamp, message) |
| `cpp/src/utils/hash.hpp` / `hash.cpp` | SHA1 hashing via OpenSSL |
| `cpp/src/utils/diff.hpp` / `diff.cpp` | Line-level diff utilities |
| `cpp/src/utils/fs_utils.hpp` / `fs_utils.cpp` | Filesystem helpers |
| `cpp/src/utils/logger.hpp` / `logger.cpp` | Structured logging; reads `KIT_LOG` env var |
| `cpp/src/utils/constants.hpp` | Path constants (`KIT_DIR`, `OBJECTS_DIR`, `HEAD_FILE`, `INDEX_FILE`, `HEADS_DIR`, `STASH_DIR`) |
| `cpp/src/utils/result.hpp` | `Result<T>` error-handling type (header-only) |

### Tests Layout

| Path | Role |
|------|------|
| `cpp/tests/unit/test_blob.cpp` | Unit tests for Blob object |
| `cpp/tests/unit/test_commit.cpp` | Unit tests for Commit object |
| `cpp/tests/unit/test_index.cpp` | Unit tests for Index |
| `cpp/tests/unit/test_refs.cpp` | Unit tests for Refs |
| `cpp/tests/unit/test_tree.cpp` | Unit tests for Tree object |
| `cpp/tests/integration/test_add_commit.cpp` | Integration: add + commit workflow |
| `cpp/tests/integration/test_branch_checkout.cpp` | Integration: branch and checkout |
| `cpp/tests/integration/test_diff.cpp` | Integration: diff output |
| `cpp/tests/integration/test_init.cpp` | Integration: repository init |
| `cpp/tests/integration/test_merge.cpp` | Integration: branch merge |

### Control Flow

`cpp/src/main.cpp` → `kit::cli::run()` → `cmd_<name>::run()` → `kit::Repository` / `kit::Refs` / `kit::Index` / object types / utils

### Key Design Notes

- **Split compilation**: each module is a `.hpp`/`.cpp` pair. Adding a new command means creating `cpp/src/cli/commands/cmd_<name>.hpp` + `.cpp` and wiring it into `cpp/src/cli/cli.cpp`.
- **Tree objects present**: commits reference a `kit::Tree` (snapshot of staged files), which in turn references `kit::Blob` objects — a three-layer object model (commit → tree → blobs).
- **Result<T> error handling**: functions return `Result<T>` instead of throwing, keeping error paths explicit throughout core and CLI layers.
