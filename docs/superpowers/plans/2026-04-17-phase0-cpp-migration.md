# Phase 0: C++ Migration + Shared Spec — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move existing C++ source into `cpp/` subdirectory and create `spec/` with the shared `.kit/` format spec and test fixture corpus for Rust interop.

**Architecture:** Git-move `src/` and `tests/` into `cpp/`. Root `CMakeLists.txt` becomes a delegating wrapper. `spec/` documents the wire format and contains canonical fixture repos both C++ and Rust tests validate against.

**Tech Stack:** C++17, CMake 3.14+, OpenSSL, cxxopts v3.1.1, Google Test 1.12.1

---

## File Map

### Created
```
cpp/CMakeLists.txt         ← existing root CMakeLists.txt, paths adjusted
cpp/src/                   ← existing src/ moved here (git mv)
cpp/tests/                 ← existing tests/ moved here (git mv)
CMakeLists.txt             ← new root, delegates to cpp/ and server/
spec/FORMAT.md             ← .kit/ wire format reference
spec/fixtures/             ← canonical repo snapshots for interop tests
spec/fixtures/empty-repo/  ← just .kit/HEAD + refs/heads/master
spec/fixtures/one-commit/  ← init + add README.md + commit
spec/fixtures/two-branch/  ← master + feature branch, diverged 1 commit each
```

### Modified
```
.gitignore                 ← add cpp/build/
CLAUDE.md                  ← update paths to reflect cpp/ layout
```

---

### Task 1: Move C++ sources into `cpp/`

**Files:**
- Modify: `CMakeLists.txt` (root, rewritten)
- Create: `cpp/CMakeLists.txt` (moved + adjusted)
- Move: `src/` → `cpp/src/`
- Move: `tests/` → `cpp/tests/`

- [ ] **Step 1: Git-move src/ and tests/**

```bash
mkdir cpp
git mv src cpp/src
git mv tests cpp/tests
```

- [ ] **Step 2: Copy root CMakeLists.txt into cpp/**

```bash
cp CMakeLists.txt cpp/CMakeLists.txt
```

- [ ] **Step 3: Fix paths in cpp/CMakeLists.txt**

In `cpp/CMakeLists.txt`, change the `target_include_directories` line:

Old:
```cmake
target_include_directories(kit_core PUBLIC src)
```

New:
```cmake
target_include_directories(kit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
```

Change test include directories:

Old:
```cmake
target_include_directories(test_unit PRIVATE tests src)
...
target_include_directories(test_integration PRIVATE tests src)
```

New:
```cmake
target_include_directories(test_unit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
...
target_include_directories(test_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
```

- [ ] **Step 4: Rewrite root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.14)
project(kit-vcs-monorepo VERSION 2.0.0)

add_subdirectory(cpp)
# add_subdirectory(server)  # uncomment in Phase 3
```

- [ ] **Step 5: Update .gitignore**

Add line:
```
cpp/build/
```

- [ ] **Step 6: Verify build**

```bash
mkdir -p cpp/build && cd cpp/build
cmake ..
make -j$(nproc)
```

Expected: `kit-vcs` binary produced, no errors.

- [ ] **Step 7: Run tests**

```bash
cd cpp/build
ctest --output-on-failure
```

Expected: all tests PASS.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "refactor: migrate C++ sources into cpp/ subdirectory"
```

---

### Task 2: Update CLAUDE.md paths

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update build instructions**

In `CLAUDE.md`, change the Build section:

Old:
```bash
mkdir build && cd build
cmake ..
make
```

New:
```bash
mkdir cpp/build && cd cpp/build
cmake ..
make
```

- [ ] **Step 2: Update test instructions**

Old:
```bash
cd build
ctest
./test_kit_vcs
```

New:
```bash
cd cpp/build
ctest
./test_unit
./test_integration
```

- [ ] **Step 3: Update Code Layout table**

Change all `include/` references to `cpp/src/`. Change `src/main.cpp` → `cpp/src/main.cpp`. Change `tests/` → `cpp/tests/`.

- [ ] **Step 4: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: update CLAUDE.md paths for cpp/ monorepo layout"
```

---

### Task 3: Write `spec/FORMAT.md`

**Files:**
- Create: `spec/FORMAT.md`

- [ ] **Step 1: Create spec/FORMAT.md**

```markdown
# kit-vcs Object Format Specification

Version: 1.0
Status: Canonical

Both the C++ and Rust implementations MUST conform to this spec.
The Python implementation uses its own independent format.

---

## Repository Layout

A kit repository is a directory containing a `.kit/` folder:

```
.kit/
├── HEAD              ← current branch or detached commit hash
├── index             ← staged file paths (newline-delimited)
├── objects/          ← content-addressed object store
│   └── <sha1>        ← one file per object, named by SHA1 hex digest
└── refs/
    └── heads/
        └── <branch>  ← one file per branch, contains SHA1 hash + newline
```

---

## HEAD File

Format A — branch ref:
```
ref: refs/heads/<branch-name>\n
```

Format B — detached HEAD:
```
<40-char-sha1-hex>\n
```

---

## Index File

Newline-delimited list of staged file paths relative to repo root.
Empty index = empty file (zero bytes).

Example:
```
README.md
src/main.cpp
src/utils/hash.hpp
```

---

## Object Storage

All objects stored at `.kit/objects/<sha1>` as raw bytes.
SHA1 computed over the full serialized object bytes (header + content).

---

## Blob Object

Stores raw file content.

Serialization:
```
blob <content-byte-count>\n<content-bytes>
```

Example — file containing "hello\n" (6 bytes):
```
blob 6\nhello\n
```

SHA1 input: the full serialized bytes above.

---

## Tree Object

Stores a directory snapshot: maps file names to blob SHA1 hashes.

Serialization:
```
tree <entry-count>\n<entry1>\n<entry2>\n...
```

Each entry:
```
<mode> <filename>\0<40-char-sha1-hex>
```

- `mode` is always `100644` (regular file)
- entries sorted lexicographically by filename
- `\0` is a null byte separating name from hash

Example — tree with one file README.md:
```
tree 1\n100644 README.md\0<sha1-of-blob>
```

SHA1 input: full serialized bytes above.

---

## Commit Object

Stores metadata for a snapshot.

Serialization:
```
commit <content-byte-count>\ntree <tree-sha1>\nparent <parent-sha1>\nauthor <name>\ntimestamp <unix-epoch-seconds>\n\n<message>\n
```

- `parent` line omitted for root commit (first commit in repo)
- `timestamp` is Unix epoch as decimal integer
- blank line separates header from message
- message ends with `\n`

Example — root commit:
```
commit 89\ntree abc123...\nauthor Alice\ntimestamp 1713300000\n\nInitial commit\n
```

Example — non-root commit:
```
commit 123\ntree def456...\nparent abc789...\nauthor Alice\ntimestamp 1713300060\n\nAdd feature\n
```

SHA1 input: full serialized bytes above.

---

## Refs

Branch ref file at `.kit/refs/heads/<branch>`:
```
<40-char-sha1-hex>\n
```

---

## Invariants

1. All SHA1 hashes are lowercase hex, 40 characters.
2. Object files are immutable once written.
3. Index contains only paths that exist as blobs in the object store (after `kit add`).
4. HEAD always points to a valid branch ref or a valid object hash.
```

- [ ] **Step 2: Commit**

```bash
git add spec/FORMAT.md
git commit -m "docs: add kit-vcs object format spec for C++/Rust interop"
```

---

### Task 4: Create spec fixture — `empty-repo`

**Files:**
- Create: `spec/fixtures/empty-repo/.kit/HEAD`
- Create: `spec/fixtures/empty-repo/.kit/refs/heads/.gitkeep`
- Create: `spec/fixtures/empty-repo/.kit/objects/.gitkeep`
- Create: `spec/fixtures/empty-repo/.kit/index`

- [ ] **Step 1: Create fixture directory**

```bash
mkdir -p spec/fixtures/empty-repo/.kit/objects
mkdir -p spec/fixtures/empty-repo/.kit/refs/heads
touch spec/fixtures/empty-repo/.kit/objects/.gitkeep
touch spec/fixtures/empty-repo/.kit/refs/heads/.gitkeep
```

- [ ] **Step 2: Create HEAD**

File `spec/fixtures/empty-repo/.kit/HEAD` content (exactly):
```
ref: refs/heads/master
```
(no trailing newline beyond the `\n` after "master")

```bash
printf 'ref: refs/heads/master\n' > spec/fixtures/empty-repo/.kit/HEAD
```

- [ ] **Step 3: Create empty index**

```bash
touch spec/fixtures/empty-repo/.kit/index
```

- [ ] **Step 4: Verify using C++ kit**

```bash
cd cpp/build
./kit-vcs --version  # confirm binary works
```

- [ ] **Step 5: Commit**

```bash
git add spec/fixtures/empty-repo
git commit -m "test: add empty-repo spec fixture"
```

---

### Task 5: Create spec fixture — `one-commit`

The `one-commit` fixture is a repo with one file `README.md` containing `"# kit-vcs\n"` committed on branch `master`.

**Files:**
- Create: `spec/fixtures/one-commit/` (full `.kit/` structure with real object hashes)
- Create: `spec/fixtures/one-commit/README.md`

- [ ] **Step 1: Generate fixture using kit binary**

```bash
cd /tmp
rm -rf fixture-gen
mkdir fixture-gen && cd fixture-gen
printf '# kit-vcs\n' > README.md
/home/kyuna/Desktop/kit-vcs/cpp/build/kit-vcs init
/home/kyuna/Desktop/kit-vcs/cpp/build/kit-vcs add README.md
/home/kyuna/Desktop/kit-vcs/cpp/build/kit-vcs commit -m "Initial commit" --author "fixture"
```

- [ ] **Step 2: Copy fixture into spec/**

```bash
cp -r /tmp/fixture-gen /home/kyuna/Desktop/kit-vcs/spec/fixtures/one-commit
```

- [ ] **Step 3: Record expected hashes**

```bash
cat spec/fixtures/one-commit/.kit/refs/heads/master
# note this SHA1 — Rust tests must reproduce it for the same input
```

Create `spec/fixtures/one-commit/EXPECTED.md`:
```markdown
# one-commit fixture — expected values

README.md content: `# kit-vcs\n` (9 bytes)

| Object | SHA1 |
|--------|------|
| blob (README.md) | <paste blob sha1 here> |
| tree | <paste tree sha1 here> |
| commit | <paste commit sha1 here> |

Author: fixture
Message: Initial commit
```

Fill in actual SHA1s from the generated fixture.

- [ ] **Step 4: Commit**

```bash
git add spec/fixtures/one-commit
git commit -m "test: add one-commit spec fixture with expected hashes"
```

---

### Task 6: Create spec fixture — `two-branch`

Two branches: `master` (1 commit) and `feature` (branched from master + 1 more commit).

**Files:**
- Create: `spec/fixtures/two-branch/` (full `.kit/` structure)

- [ ] **Step 1: Generate fixture**

```bash
cd /tmp
rm -rf fixture-branch
mkdir fixture-branch && cd fixture-branch
printf '# kit-vcs\n' > README.md
KIT=/home/kyuna/Desktop/kit-vcs/cpp/build/kit-vcs
$KIT init
$KIT add README.md
$KIT commit -m "Initial commit" --author "fixture"
$KIT branch feature
$KIT checkout feature
printf 'feature content\n' > feature.txt
$KIT add feature.txt
$KIT commit -m "Add feature" --author "fixture"
```

- [ ] **Step 2: Copy and record**

```bash
cp -r /tmp/fixture-branch /home/kyuna/Desktop/kit-vcs/spec/fixtures/two-branch
```

Create `spec/fixtures/two-branch/EXPECTED.md` recording:
- master HEAD SHA1
- feature HEAD SHA1
- shared parent commit SHA1

- [ ] **Step 3: Commit**

```bash
git add spec/fixtures/two-branch
git commit -m "test: add two-branch spec fixture"
```
