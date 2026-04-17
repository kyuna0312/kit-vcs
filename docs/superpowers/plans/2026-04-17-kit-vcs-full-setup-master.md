# kit-vcs Full Setup — Master Plan Index

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement each sub-plan task-by-task.

**Goal:** Transform kit-vcs into a full monorepo ecosystem: C++/Rust/Python implementations, REST + TCP server, Vue SPA, cross-platform packaging.

**Spec:** `docs/superpowers/specs/2026-04-17-kit-vcs-full-setup-design.md`

---

## Execution Order

Each phase produces working, testable software. Complete phases in order — later phases depend on earlier ones.

| Phase | Plan | Depends On | Deliverable |
|-------|------|------------|-------------|
| 0 | [phase0-cpp-migration](2026-04-17-phase0-cpp-migration.md) | — | `cpp/` monorepo layout + `spec/` fixtures |
| 1 | [phase1-rust](2026-04-17-phase1-rust.md) | Phase 0 (`spec/fixtures/`) | Rust `kit-rust` binary, spec-validated |
| 2 | [phase2-python](2026-04-17-phase2-python.md) | Phase 0 (`cpp/` exists) | Python `kit-py` package, pip installable |
| 3 | [phase3-server](2026-04-17-phase3-server.md) | Phase 0 (`kit_core` in `cpp/`) | `kit-server` REST API + `kit-daemon` TCP |
| 4 | [phase4-web-ui](2026-04-17-phase4-web-ui.md) | Phase 3 (REST API on :8080) | Vue SPA served by `kit-server` |
| 5 | [phase5-packaging](2026-04-17-phase5-packaging.md) | Phases 0–4 all building | Release artifacts + GitHub Actions CI |

Phases 1 and 2 can run in parallel after Phase 0.
Phase 3 and Phase 1/2 can also run in parallel.

---

## Quick Start Per Phase

```bash
# Phase 0 — run first, everything depends on this
cd /home/kyuna/Desktop/kit-vcs
git mv src cpp/src && git mv tests cpp/tests
# then follow phase0 plan

# Phase 1 — after Phase 0
cargo init rust --name kit-rust
# follow phase1 plan

# Phase 2 — after Phase 0, can run parallel to Phase 1
mkdir python && cd python
# follow phase2 plan

# Phase 3 — after Phase 0
mkdir server && cd server
# follow phase3 plan

# Phase 4 — after Phase 3
npm create vite@latest web -- --template vue-ts
# follow phase4 plan

# Phase 5 — after all above
mkdir -p packaging .github/workflows
# follow phase5 plan
```

---

## Final Monorepo Layout (after all phases)

```
kit-vcs/
├── cpp/               ← C++ implementation (kit binary)
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── rust/              ← Rust implementation (kit-rust binary)
│   ├── src/
│   ├── tests/
│   └── Cargo.toml
├── python/            ← Python educational implementation
│   ├── kit/
│   ├── tests/
│   └── pyproject.toml
├── server/            ← kit-server + kit-daemon
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── web/               ← Vue 3 SPA
│   ├── src/
│   └── package.json
├── spec/              ← .kit/ format spec + fixture corpus
│   ├── FORMAT.md
│   └── fixtures/
├── packaging/         ← Linux/macOS/Windows packaging
│   ├── linux/
│   ├── macos/
│   └── windows/
├── .github/
│   └── workflows/release.yml
├── docs/
│   └── superpowers/
│       ├── specs/
│       └── plans/
└── CMakeLists.txt     ← root: add_subdirectory(cpp) + add_subdirectory(server)
```
