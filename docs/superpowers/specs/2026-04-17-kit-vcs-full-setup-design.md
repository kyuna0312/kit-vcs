# kit-vcs — Full Setup Design

**Date:** 2026-04-17
**Status:** Approved

---

## Overview

Transform kit-vcs into a full sandbox ecosystem: three language implementations (C++, Rust, Python), a dual-protocol server (REST + binary TCP), a Vue SPA collaboration UI, and cross-platform packaging. The project lives in a single monorepo.

**Sub-project build order:**
1. Remote Protocol (C++ core extension)
2. Server (`kit-server` REST + `kit-daemon` TCP)
3. Web UI (Vue SPA)
4. Packaging (Linux + macOS + Windows)

Python and Rust implementations are developed in parallel with server work.

---

## Monorepo Structure

```
kit-vcs/
├── cpp/          ← existing src/ migrated here
├── rust/         ← new Rust crate (interoperable with C++ via spec/)
├── python/       ← educational Python implementation (independent format)
├── server/       ← kit-server (REST) + kit-daemon (TCP), link kit_core
├── web/          ← Vue 3 SPA
├── spec/         ← .kit/ format spec + shared test fixture corpus
├── docs/
└── packaging/    ← Linux .deb/AUR, macOS Homebrew, Windows NSIS/Chocolatey
```

Root `CMakeLists.txt` delegates to `cpp/` and `server/`. Rust uses Cargo workspace. Python uses pyproject.toml.

---

## Section 1: C++ Migration + Shared Spec

### C++ Migration
Existing `src/` → `cpp/src/`, `tests/` → `cpp/tests/`, `CMakeLists.txt` → `cpp/CMakeLists.txt`. No logic changes — structural move only.

### Shared Spec (`spec/`)
Defines the `.kit/` object format that C++ and Rust both implement:

| Object | Wire format |
|--------|-------------|
| Blob | `"blob <size>\0<content>"` |
| Tree | `"tree <size>\0<entries>"` — each entry: `"<mode> <name>\0<sha1>"` |
| Commit | `"commit <size>\0<content>"` — fields: tree, parent, author, timestamp, message |
| Index | Newline-delimited staged file paths |
| Refs | `refs/heads/<branch>` — one file per branch, SHA1 hash as content |
| HEAD | `ref: refs/heads/<branch>` or bare SHA1 |

**Test fixture corpus:** canonical `.kit/` repo snapshots that both C++ and Rust integration tests must pass against. Stored in `spec/fixtures/`.

Python does **not** use `spec/` — it has its own independent `.kit/` format.

---

## Section 2: Rust Implementation

Interoperable with C++ via `spec/`. Same CLI surface: `kit init`, `kit add`, `kit commit`, `kit status`, `kit log`, `kit branch`, `kit checkout`, `kit diff`, `kit merge`, `kit reset`, `kit stash`.

### Structure
```
rust/
├── Cargo.toml
├── src/
│   ├── main.rs
│   ├── core/
│   │   ├── objects/       ← blob.rs, tree.rs, commit.rs
│   │   ├── index.rs
│   │   ├── refs.rs
│   │   └── repository.rs
│   ├── cli/
│   │   └── commands/      ← cmd_init.rs, cmd_add.rs, cmd_commit.rs, etc.
│   └── utils/
│       ├── hash.rs        ← SHA1 via sha1 crate
│       ├── fs_utils.rs
│       └── diff.rs
└── tests/
    ├── unit/
    └── integration/       ← loads fixtures from spec/fixtures/
```

### Key Decisions
- `clap` for CLI parsing (mirrors cxxopts role in C++)
- `Result<T, KitError>` with custom error enum — mirrors C++ `Result<T>`
- Integration tests validate against `spec/fixtures/` — same corpus C++ tests use
- Binary name: `kit-rust` to coexist with C++ `kit` at install time
- No unsafe code — idiomatic Rust throughout

---

## Section 3: Python Implementation

Educational — own `.kit/` format, own learning exercise. No interop requirement with C++ or Rust. Prioritizes readability and learning over production quality.

### Structure
```
python/
├── pyproject.toml
├── kit/
│   ├── __main__.py        ← entry: python -m kit
│   ├── core/
│   │   ├── objects.py     ← blob, tree, commit in one file (educational)
│   │   ├── index.py
│   │   ├── refs.py
│   │   └── repository.py
│   ├── cli/
│   │   └── commands/      ← one file per command
│   └── utils/
│       ├── hash.py        ← hashlib SHA1, stdlib only
│       ├── fs_utils.py
│       └── diff.py
└── tests/
    ├── unit/
    └── integration/
```

### Key Decisions
- `argparse` only — zero external dependencies (educational clarity)
- Heavily commented, verbose — learning tool not production code
- Python 3.10+ — match statements for command dispatch
- No mypy/type stubs — keeps it approachable for beginners
- PyPI package name: `kit-vcs-py` (avoids name collision)

---

## Section 4: Server (`kit-server` + `kit-daemon`)

Two binaries in `server/`, both link `kit_core` static lib from `cpp/`.

### Structure
```
server/
├── CMakeLists.txt
├── src/
│   ├── rest/
│   │   ├── server.hpp/cpp      ← HTTP server (cpp-httplib, header-only)
│   │   ├── router.hpp/cpp      ← route registration
│   │   └── handlers/
│   │       ├── repos.cpp
│   │       ├── commits.cpp
│   │       ├── branches.cpp
│   │       ├── pulls.cpp
│   │       └── issues.cpp
│   ├── daemon/
│   │   ├── tcp_server.hpp/cpp  ← raw TCP, custom binary protocol
│   │   ├── protocol.hpp/cpp    ← packet framing + command dispatch
│   │   └── handlers/
│   │       ├── push.cpp
│   │       ├── fetch.cpp
│   │       └── clone.cpp
│   └── auth/
│       ├── tokens.hpp/cpp      ← bearer token auth (REST)
│       └── keys.hpp/cpp        ← SSH-style pubkey auth (daemon)
└── tests/
```

### REST API
Base URL: `http://localhost:8080`

| Method | Route | Action |
|--------|-------|--------|
| POST | `/repos` | Create repo |
| GET | `/repos/:name` | Repo info |
| GET | `/repos/:name/commits` | Commit log |
| GET | `/repos/:name/branches` | Branch list |
| POST | `/repos/:name/pulls` | Create PR |
| PATCH | `/repos/:name/pulls/:id` | Merge or close PR |
| POST | `/repos/:name/issues` | Create issue |
| PATCH | `/repos/:name/issues/:id` | Update issue |

Auth: `Authorization: Bearer <token>` header.

### Binary Protocol (TCP daemon, port 9418)
```
[4 bytes: payload length][1 byte: command][payload bytes]

Commands:
  0x01  CLONE   ← clone full repo
  0x02  FETCH   ← fetch objects since ref
  0x03  PUSH    ← push objects + update ref
```

Designed so Git Smart HTTP can be added as a thin translation layer later without changing handlers.

---

## Section 5: Web UI (Vue SPA)

Full GitHub-lite collaboration UI. Talks exclusively to `kit-server` REST API.

### Structure
```
web/
├── package.json
├── vite.config.ts
├── src/
│   ├── main.ts
│   ├── router/             ← Vue Router
│   ├── stores/             ← Pinia state management
│   ├── api/                ← typed fetch wrappers
│   │   ├── repos.ts
│   │   ├── commits.ts
│   │   ├── branches.ts
│   │   ├── pulls.ts
│   │   └── issues.ts
│   ├── components/
│   │   ├── common/         ← Button, Badge, Avatar, Modal
│   │   └── domain/         ← CommitGraph, DiffViewer, FileBrowser
│   └── views/
│       ├── Home.vue         ← repo list
│       ├── Repo.vue         ← repo overview + file tree
│       ├── Commits.vue      ← commit log + graph
│       ├── Diff.vue         ← side-by-side diff viewer
│       ├── Branches.vue
│       ├── PullRequest.vue  ← PR detail + inline code review
│       ├── Issues.vue
│       └── Settings.vue     ← repo settings + danger zone
```

### Tech Stack
- Vite + Vue 3 + TypeScript
- Pinia for global state
- Vue Router for navigation
- `highlight.js` for syntax highlighting in diffs
- No UI component library — custom components (lightweight + educational)
- `kit-server` serves built SPA from `dist/` as static files

---

## Section 6: Packaging

### Structure
```
packaging/
├── linux/
│   ├── debian/         ← .deb (control, rules, install)
│   ├── arch/           ← PKGBUILD for AUR
│   └── install.sh
├── macos/
│   ├── Formula/
│   │   └── kit-vcs.rb  ← Homebrew formula
│   └── install.sh
├── windows/
│   ├── kit-vcs.nsi     ← NSIS installer
│   └── chocolatey/     ← Chocolatey package
└── ci/
    ├── build-linux.yml
    ├── build-macos.yml
    └── build-windows.yml
```

### Artifacts Per Platform

| Artifact | Linux | macOS | Windows |
|----------|-------|-------|---------|
| `kit` (C++) | .deb + AUR | Homebrew | NSIS + Choco |
| `kit-rust` | binary release | binary release | binary release |
| `kit-server` | systemd service | launchd plist | Windows Service |
| `kit-daemon` | systemd service | launchd plist | Windows Service |
| Python (`kit-vcs-py`) | PyPI | PyPI | PyPI |

### CI Pipeline
GitHub Actions triggers on version tag push (`v*`):
1. Build C++ on Linux/macOS/Windows
2. Build Rust via `cargo build --release` cross-platform
3. Package per platform
4. Upload all artifacts to GitHub Release

---

## Implementation Order

| Phase | Work |
|-------|------|
| 0 | Migrate C++ `src/` → `cpp/`, write `spec/` format doc + fixtures |
| 1 | Rust core + CLI (passes spec fixtures) |
| 2 | Python implementation (independent) |
| 3 | Server REST API (`kit-server`) |
| 4 | Server TCP daemon (`kit-daemon`) + remote protocol in C++ + Rust |
| 5 | Vue SPA (`web/`) |
| 6 | Packaging + CI (`packaging/`) |
