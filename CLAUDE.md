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

Tests use Google Test (fetched via CMake FetchContent) and a raw `assert`-based runner in `cpp/tests/test_kit_vcs.cpp`. Each test function calls `cleanup_test_environment()` and operates on a temporary `.kit/` directory created in the working directory.

## Architecture

### Object Storage Model

`.kit/` mirrors Git's object store:
- `objects/` — commit objects stored as flat files named by SHA1 hash
- `refs/heads/` — branch refs, one file per branch containing commit hash
- `HEAD` — either a ref string (`ref: refs/heads/master`) or a bare commit hash
- `index` — newline-delimited list of staged file paths

### Code Layout

All logic lives in **header files** under `cpp/src/` (mostly `inline` functions). `cpp/src/main.cpp` and `cpp/src/hash_object.cpp` are the only `.cpp` files.

| Path | Role |
|------|------|
| `cpp/src/kit_vcs.hpp` | Top-level namespace `kit_vcs` — init, stage, commit, log, and aggregates all command headers |
| `cpp/src/cli/cli.hpp` | `cli` namespace — command dispatch using `cxxopts`, one `handle_*` function per command |
| `cpp/src/commands/*.hpp` | Per-command logic (branch, checkout, commit, diff, merge, reset, stash, status) |
| `cpp/src/utils/kit_utils.hpp` | Core file I/O helpers, commit creation, diff, three-way merge |
| `cpp/src/utils/hash_object.hpp` | SHA1 hashing via OpenSSL |
| `cpp/src/utils/constants.hpp` | Path constants (`KIT_DIR`, `OBJECTS_DIR`, `HEAD_FILE`, `INDEX_FILE`, `HEADS_DIR`) |
| `cpp/src/utils/error_handler.hpp` | `error_handler::print_error()` — writes to stderr |
| `cpp/src/utils/mock_kit_utils.hpp` | Mock utilities for testing |

### Control Flow

`cpp/src/main.cpp` → `cli::handle_command()` → `kit_vcs::*()` → `kit_utils::*()` / `hash_object::*()` / filesystem ops

### Key Design Notes

- **Header-only implementation**: nearly all logic is in `cpp/src/` as `inline` functions. Adding new commands means adding a header in `cpp/src/commands/` and wiring it in `cpp/src/cli/cli.hpp` and `cpp/src/kit_vcs.hpp`.
- **No tree objects**: commits store file content directly in the objects directory — not a full Git-compatible format.
- **Planned future structure**: `note.md` outlines a planned refactor into `src/core/`, `src/cli/`, `src/server/` subdirectories — current layout is flatter than this.
