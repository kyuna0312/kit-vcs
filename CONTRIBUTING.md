# Contributing to kit-vcs

Thanks for your interest in improving `kit`! This document covers the conventions the codebase follows. For build and test instructions, see the [README](README.md).

## Code Conventions

### Split compilation

Every module is a `.hpp`/`.cpp` pair: declarations in the header, definitions in the source file. All source lives under `cpp/src/`.

| Path | Role |
|------|------|
| `cpp/src/cli/` | Command dispatch (`kit::cli`) and per-command logic |
| `cpp/src/core/` | `Repository`, `Refs`, `Index`, and the object types (`Blob`, `Tree`, `Commit`) |
| `cpp/src/utils/` | SHA-1 hashing, diff, filesystem helpers, logging, constants, `Result<T>` |

### Adding a new command

1. Create `cpp/src/cli/commands/cmd_<name>.hpp` and `cmd_<name>.cpp` with a `run()` function.
2. Wire it into the dispatch table in `cpp/src/cli/cli.cpp`.
3. Add the new `.cpp` to `cpp/CMakeLists.txt`.
4. Add an integration test under `cpp/tests/integration/`.

### Error handling: `Result<T>`

Functions return `Result<T>` (`cpp/src/utils/result.hpp`) instead of throwing. This keeps error paths explicit throughout the core and CLI layers — follow this pattern in new code; don't introduce exceptions on the happy path.

### Other conventions

- Path constants (`KIT_DIR`, `OBJECTS_DIR`, `HEAD_FILE`, ...) live in `cpp/src/utils/constants.hpp` — use them, don't re-hardcode `.kit/` paths.
- Logging goes through `cpp/src/utils/logger.hpp` (controlled by the `KIT_LOG` env var), not raw `std::cerr`.

## Object Format

The on-disk format is specified in [`spec/FORMAT.md`](spec/FORMAT.md), with byte-exact fixtures under `spec/fixtures/`. Any change that affects the bytes written to `.kit/` must update the spec and fixtures in the same PR — other implementations depend on them for interoperability.

## Tests

- Unit tests: `cpp/tests/unit/` — one file per module (`test_blob.cpp`, `test_refs.cpp`, ...).
- Integration tests: `cpp/tests/integration/` — end-to-end command workflows.
- Tests use Google Test (fetched via CMake FetchContent). Each test operates on a temporary `.kit/` directory created in the working directory.
- Run with `ctest` from `cpp/build/`, or `./test_unit` / `./test_integration` directly for verbose output.

New behaviour needs a test; bug fixes need a regression test.

## Pull Requests

- Keep PRs focused — one logical change per PR.
- Use conventional-commit-style messages (`feat:`, `fix:`, `test:`, `docs:`, `chore:`).
- Make sure `ctest` passes before opening a PR.
