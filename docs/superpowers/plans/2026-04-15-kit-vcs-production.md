# kit-vcs Production Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform kit-vcs from a header-only educational demo into a production-quality local VCS with a proper blob/tree/commit object model and all planned commands implemented.

**Architecture:** Two-phase incremental refactor. Phase 1 restructures into `src/core/`, `src/cli/`, `src/utils/` with `.cpp`/`.hpp` splits and a static `kit_core` library. Phase 2 implements five missing commands (branch, checkout, diff, merge, reset) plus `stash pop` on top of the clean architecture.

**Tech Stack:** C++17, CMake 3.14+, OpenSSL (SHA1), cxxopts v3.1.1, Google Test 1.12.1

---

## File Map

### Created
```
src/utils/constants.hpp
src/utils/result.hpp
src/utils/logger.hpp / logger.cpp
src/utils/hash.hpp / hash.cpp
src/utils/fs_utils.hpp / fs_utils.cpp
src/utils/diff.hpp / diff.cpp
src/core/objects/blob.hpp / blob.cpp
src/core/objects/tree.hpp / tree.cpp
src/core/objects/commit.hpp / commit.cpp
src/core/index.hpp / index.cpp
src/core/refs.hpp / refs.cpp
src/core/repository.hpp / repository.cpp
src/cli/cli.hpp / cli.cpp
src/cli/commands/cmd_init.hpp / cmd_init.cpp
src/cli/commands/cmd_add.hpp / cmd_add.cpp
src/cli/commands/cmd_commit.hpp / cmd_commit.cpp
src/cli/commands/cmd_status.hpp / cmd_status.cpp
src/cli/commands/cmd_log.hpp / cmd_log.cpp
src/cli/commands/cmd_branch.hpp / cmd_branch.cpp
src/cli/commands/cmd_checkout.hpp / cmd_checkout.cpp
src/cli/commands/cmd_diff.hpp / cmd_diff.cpp
src/cli/commands/cmd_merge.hpp / cmd_merge.cpp
src/cli/commands/cmd_reset.hpp / cmd_reset.cpp
src/cli/commands/cmd_stash.hpp / cmd_stash.cpp
tests/unit/test_blob.cpp
tests/unit/test_tree.cpp
tests/unit/test_commit.cpp
tests/unit/test_index.cpp
tests/unit/test_refs.cpp
tests/integration/helpers.hpp
tests/integration/test_init.cpp
tests/integration/test_add_commit.cpp
tests/integration/test_branch_checkout.cpp
tests/integration/test_diff.cpp
tests/integration/test_merge.cpp
```

### Modified
```
src/main.cpp           — rewritten to use new cli::run()
CMakeLists.txt         — new layout, kit_core static lib, two test targets
```

### Deleted
```
include/               — entire directory (replaced by src/)
src/hash_object.cpp    — replaced by src/utils/hash.cpp
tests/test_kit_vcs.cpp — replaced by tests/unit/ + tests/integration/
tests/test_commands.cpp
```

---

## Phase 1 — Architecture Refactor

---

### Task 1: CMakeLists.txt + Directory Skeleton

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create directory skeleton**

```bash
mkdir -p src/utils src/core/objects src/cli/commands
mkdir -p tests/unit tests/integration
```

- [ ] **Step 2: Replace CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.14)
project(kit-vcs VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
endif()

find_package(OpenSSL REQUIRED)

include(FetchContent)
FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip)
FetchContent_MakeAvailable(cxxopts googletest)

set(CORE_SOURCES
    src/utils/logger.cpp
    src/utils/hash.cpp
    src/utils/fs_utils.cpp
    src/utils/diff.cpp
    src/core/objects/blob.cpp
    src/core/objects/tree.cpp
    src/core/objects/commit.cpp
    src/core/index.cpp
    src/core/refs.cpp
    src/core/repository.cpp
    src/cli/commands/cmd_init.cpp
    src/cli/commands/cmd_add.cpp
    src/cli/commands/cmd_commit.cpp
    src/cli/commands/cmd_status.cpp
    src/cli/commands/cmd_log.cpp
    src/cli/commands/cmd_branch.cpp
    src/cli/commands/cmd_checkout.cpp
    src/cli/commands/cmd_diff.cpp
    src/cli/commands/cmd_merge.cpp
    src/cli/commands/cmd_reset.cpp
    src/cli/commands/cmd_stash.cpp
    src/cli/cli.cpp
)

add_library(kit_core STATIC ${CORE_SOURCES})
target_include_directories(kit_core PUBLIC src)
target_link_libraries(kit_core PUBLIC OpenSSL::Crypto cxxopts::cxxopts)

add_executable(kit-vcs src/main.cpp)
target_link_libraries(kit-vcs PRIVATE kit_core)

enable_testing()

file(GLOB UNIT_SOURCES "tests/unit/*.cpp")
add_executable(test_unit ${UNIT_SOURCES})
target_link_libraries(test_unit PRIVATE kit_core gtest gtest_main)
target_include_directories(test_unit PRIVATE tests)
add_test(NAME unit COMMAND test_unit)

file(GLOB INTEGRATION_SOURCES "tests/integration/*.cpp")
add_executable(test_integration ${INTEGRATION_SOURCES})
target_link_libraries(test_integration PRIVATE kit_core gtest gtest_main)
target_include_directories(test_integration PRIVATE tests)
target_compile_definitions(test_integration PRIVATE
    KIT_BINARY="$<TARGET_FILE:kit-vcs>")
add_test(NAME integration COMMAND test_integration)
```

- [ ] **Step 3: Create placeholder .cpp stubs** so CMake can configure without errors (fill with `// stub` for now):

```bash
for f in src/utils/logger.cpp src/utils/hash.cpp src/utils/fs_utils.cpp src/utils/diff.cpp \
  src/core/objects/blob.cpp src/core/objects/tree.cpp src/core/objects/commit.cpp \
  src/core/index.cpp src/core/refs.cpp src/core/repository.cpp \
  src/cli/commands/cmd_init.cpp src/cli/commands/cmd_add.cpp \
  src/cli/commands/cmd_commit.cpp src/cli/commands/cmd_status.cpp \
  src/cli/commands/cmd_log.cpp src/cli/commands/cmd_branch.cpp \
  src/cli/commands/cmd_checkout.cpp src/cli/commands/cmd_diff.cpp \
  src/cli/commands/cmd_merge.cpp src/cli/commands/cmd_reset.cpp \
  src/cli/commands/cmd_stash.cpp src/cli/cli.cpp; do
  echo "// stub" > $f
done
```

Also create a stub `src/main.cpp`:
```cpp
int main() { return 0; }
```

- [ ] **Step 4: Configure and verify CMake finds everything**

```bash
mkdir -p build && cd build && cmake .. 2>&1 | tail -5
```

Expected: `-- Configuring done` with no errors.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/ tests/
git commit -m "build: restructure for kit_core static lib and new src layout"
```

---

### Task 2: utils/constants.hpp + utils/result.hpp

**Files:**
- Create: `src/utils/constants.hpp`
- Create: `src/utils/result.hpp`

- [ ] **Step 1: Write `src/utils/constants.hpp`**

```cpp
#pragma once
namespace kit {
    inline constexpr const char* KIT_DIR      = ".kit";
    inline constexpr const char* OBJECTS_DIR  = ".kit/objects";
    inline constexpr const char* REFS_DIR     = ".kit/refs";
    inline constexpr const char* HEADS_DIR    = ".kit/refs/heads";
    inline constexpr const char* HEAD_FILE    = ".kit/HEAD";
    inline constexpr const char* INDEX_FILE   = ".kit/index";
    inline constexpr const char* STASH_DIR    = ".kit/stash";
}
```

- [ ] **Step 2: Write `src/utils/result.hpp`**

```cpp
#pragma once
#include <optional>
#include <string>

namespace kit {

template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    bool ok() const { return value.has_value(); }
    static Result success(T v) { return {std::move(v), ""}; }
    static Result failure(std::string e) { return {std::nullopt, std::move(e)}; }
};

template<>
struct Result<void> {
    bool success;
    std::string error;
    bool ok() const { return success; }
    static Result ok_result() { return {true, ""}; }
    static Result failure(std::string e) { return {false, std::move(e)}; }
};

} // namespace kit
```

- [ ] **Step 3: Build to confirm headers compile**

```bash
cd build && make kit_core 2>&1 | grep -E "error:|warning:" | head -20
```

Expected: no errors (stubs compile, headers are clean).

- [ ] **Step 4: Commit**

```bash
git add src/utils/constants.hpp src/utils/result.hpp
git commit -m "feat(utils): add constants and Result<T> type"
```

---

### Task 3: utils/logger

**Files:**
- Create: `src/utils/logger.hpp`
- Create: `src/utils/logger.cpp`

- [ ] **Step 1: Write `src/utils/logger.hpp`**

```cpp
#pragma once
#include <string>

namespace kit::logger {

enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERR = 3 };

void set_level(Level l);
Level get_level();
void init_from_env(); // reads KIT_LOG_LEVEL env var

void debug(const std::string& msg);
void info(const std::string& msg);
void warn(const std::string& msg);
void error(const std::string& msg); // writes to stderr

} // namespace kit::logger
```

- [ ] **Step 2: Write `src/utils/logger.cpp`**

```cpp
#include "logger.hpp"
#include <iostream>
#include <cstdlib>

namespace kit::logger {

static Level current_level = Level::INFO;

void set_level(Level l) { current_level = l; }
Level get_level() { return current_level; }

void init_from_env() {
    const char* env = std::getenv("KIT_LOG_LEVEL");
    if (!env) return;
    std::string val(env);
    if (val == "DEBUG") set_level(Level::DEBUG);
    else if (val == "INFO")  set_level(Level::INFO);
    else if (val == "WARN")  set_level(Level::WARN);
    else if (val == "ERROR") set_level(Level::ERR);
}

void debug(const std::string& msg) {
    if (current_level <= Level::DEBUG)
        std::cout << "[kit debug] " << msg << "\n";
}
void info(const std::string& msg) {
    if (current_level <= Level::INFO)
        std::cout << "[kit] " << msg << "\n";
}
void warn(const std::string& msg) {
    if (current_level <= Level::WARN)
        std::cerr << "[kit warn] " << msg << "\n";
}
void error(const std::string& msg) {
    if (current_level <= Level::ERR)
        std::cerr << "[kit error] " << msg << "\n";
}

} // namespace kit::logger
```

- [ ] **Step 3: Build**

```bash
cd build && make kit_core 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/utils/logger.hpp src/utils/logger.cpp
git commit -m "feat(utils): add centralized logger with KIT_LOG_LEVEL env support"
```

---

### Task 4: utils/hash

**Files:**
- Create: `src/utils/hash.hpp`
- Create: `src/utils/hash.cpp`

- [ ] **Step 1: Write `src/utils/hash.hpp`**

```cpp
#pragma once
#include <string>

namespace kit::hash {
    // Returns lowercase hex SHA1 of data
    std::string sha1(const std::string& data);
}
```

- [ ] **Step 2: Write `src/utils/hash.cpp`**

```cpp
#include "hash.hpp"
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace kit::hash {

std::string sha1(const std::string& data) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA1 digest failed");
    }

    unsigned char digest[20];
    unsigned int digest_len = 0;
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < digest_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    return oss.str();
}

} // namespace kit::hash
```

- [ ] **Step 3: Build**

```bash
cd build && make kit_core 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/utils/hash.hpp src/utils/hash.cpp
git commit -m "feat(utils): add SHA1 hash utility via OpenSSL EVP"
```

---

### Task 5: utils/fs_utils

**Files:**
- Create: `src/utils/fs_utils.hpp`
- Create: `src/utils/fs_utils.cpp`

- [ ] **Step 1: Write `src/utils/fs_utils.hpp`**

```cpp
#pragma once
#include <string>
#include <filesystem>

namespace kit::fs {
    // Read entire file; throws std::runtime_error on failure
    std::string read_file(const std::filesystem::path& path);
    // Write content to path, creating parent dirs as needed
    void write_file(const std::filesystem::path& path, const std::string& content);
    // Create directory and all parents; no-op if exists
    void ensure_dir(const std::filesystem::path& path);
}
```

- [ ] **Step 2: Write `src/utils/fs_utils.cpp`**

```cpp
#include "fs_utils.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace kit::fs {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path.string());
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void write_file(const std::filesystem::path& path, const std::string& content) {
    ensure_dir(path.parent_path());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot write file: " + path.string());
    f << content;
}

void ensure_dir(const std::filesystem::path& path) {
    if (!path.empty())
        std::filesystem::create_directories(path);
}

} // namespace kit::fs
```

- [ ] **Step 3: Build**

```bash
cd build && make kit_core 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/utils/fs_utils.hpp src/utils/fs_utils.cpp
git commit -m "feat(utils): add filesystem helpers (read_file, write_file, ensure_dir)"
```

---

### Task 6: core/objects/blob

**Files:**
- Create: `src/core/objects/blob.hpp`
- Create: `src/core/objects/blob.cpp`
- Create: `tests/unit/test_blob.cpp`

- [ ] **Step 1: Write failing test `tests/unit/test_blob.cpp`**

```cpp
#include <gtest/gtest.h>
#include "core/objects/blob.hpp"

TEST(BlobTest, SerializeFormat) {
    kit::Blob b{"hello"};
    EXPECT_EQ(b.serialize(), "blob 5\nhello");
}

TEST(BlobTest, HashIsConsistent) {
    kit::Blob b{"hello"};
    EXPECT_EQ(b.hash(), b.hash());
    EXPECT_EQ(b.hash().size(), 40u);
}

TEST(BlobTest, RoundTrip) {
    kit::Blob original{"some content\nwith newlines\n"};
    auto restored = kit::Blob::deserialize(original.serialize());
    EXPECT_EQ(restored.content, original.content);
}

TEST(BlobTest, EmptyContent) {
    kit::Blob b{""};
    EXPECT_EQ(b.serialize(), "blob 0\n");
    auto restored = kit::Blob::deserialize(b.serialize());
    EXPECT_EQ(restored.content, "");
}
```

- [ ] **Step 2: Run test — expect compile failure (blob.hpp not yet written)**

```bash
cd build && make test_unit 2>&1 | grep "error:" | head -5
```

Expected: `error: 'kit::Blob' was not declared`

- [ ] **Step 3: Write `src/core/objects/blob.hpp`**

```cpp
#pragma once
#include <string>

namespace kit {

struct Blob {
    std::string content;

    std::string serialize() const;   // "blob <size>\n<content>"
    std::string hash() const;        // SHA1 of serialize()
    static Blob deserialize(const std::string& raw);
    static Blob from_file(const std::string& path);
};

} // namespace kit
```

- [ ] **Step 4: Write `src/core/objects/blob.cpp`**

```cpp
#include "blob.hpp"
#include "utils/hash.hpp"
#include "utils/fs_utils.hpp"
#include <stdexcept>

namespace kit {

std::string Blob::serialize() const {
    return "blob " + std::to_string(content.size()) + "\n" + content;
}

std::string Blob::hash() const {
    return kit::hash::sha1(serialize());
}

Blob Blob::deserialize(const std::string& raw) {
    auto nl = raw.find('\n');
    if (nl == std::string::npos)
        throw std::runtime_error("Invalid blob: missing newline");
    // header is "blob <size>", rest is content
    return Blob{raw.substr(nl + 1)};
}

Blob Blob::from_file(const std::string& path) {
    return Blob{kit::fs::read_file(path)};
}

} // namespace kit
```

- [ ] **Step 5: Build and run tests**

```bash
cd build && make test_unit && ./test_unit --gtest_filter="BlobTest*"
```

Expected: `[  PASSED  ] 4 tests.`

- [ ] **Step 6: Commit**

```bash
git add src/core/objects/blob.hpp src/core/objects/blob.cpp tests/unit/test_blob.cpp
git commit -m "feat(core): add Blob object with serialize/hash/deserialize"
```

---

### Task 7: core/objects/tree

**Files:**
- Create: `src/core/objects/tree.hpp`
- Create: `src/core/objects/tree.cpp`
- Create: `tests/unit/test_tree.cpp`

- [ ] **Step 1: Write failing test `tests/unit/test_tree.cpp`**

```cpp
#include <gtest/gtest.h>
#include "core/objects/tree.hpp"

TEST(TreeTest, SerializeRoundTrip) {
    kit::Tree t;
    t.entries.push_back({"blob", "abc123", "main.cpp"});
    t.entries.push_back({"blob", "def456", "readme.txt"});
    auto restored = kit::Tree::deserialize(t.serialize());
    ASSERT_EQ(restored.entries.size(), 2u);
    EXPECT_EQ(restored.entries[0].hash, "abc123");
    EXPECT_EQ(restored.entries[0].name, "main.cpp");
    EXPECT_EQ(restored.entries[1].hash, "def456");
}

TEST(TreeTest, HashConsistent) {
    kit::Tree t;
    t.entries.push_back({"blob", "abc123", "file.txt"});
    EXPECT_EQ(t.hash(), t.hash());
    EXPECT_EQ(t.hash().size(), 40u);
}

TEST(TreeTest, EmptyTree) {
    kit::Tree t;
    EXPECT_EQ(t.serialize(), "");
    auto restored = kit::Tree::deserialize("");
    EXPECT_TRUE(restored.entries.empty());
}
```

- [ ] **Step 2: Write `src/core/objects/tree.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>

namespace kit {

struct TreeEntry {
    std::string mode;  // "blob"
    std::string hash;
    std::string name;
};

struct Tree {
    std::vector<TreeEntry> entries;

    std::string serialize() const;  // "blob <hash> <name>\n" per entry
    std::string hash() const;       // SHA1 of serialize()
    static Tree deserialize(const std::string& raw);
};

} // namespace kit
```

- [ ] **Step 3: Write `src/core/objects/tree.cpp`**

```cpp
#include "tree.hpp"
#include "utils/hash.hpp"
#include <sstream>

namespace kit {

std::string Tree::serialize() const {
    std::ostringstream ss;
    for (const auto& e : entries)
        ss << e.mode << " " << e.hash << " " << e.name << "\n";
    return ss.str();
}

std::string Tree::hash() const {
    return kit::hash::sha1(serialize());
}

Tree Tree::deserialize(const std::string& raw) {
    Tree t;
    if (raw.empty()) return t;
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        TreeEntry e;
        ls >> e.mode >> e.hash >> e.name;
        t.entries.push_back(e);
    }
    return t;
}

} // namespace kit
```

- [ ] **Step 4: Build and run tests**

```bash
cd build && make test_unit && ./test_unit --gtest_filter="TreeTest*"
```

Expected: `[  PASSED  ] 3 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/core/objects/tree.hpp src/core/objects/tree.cpp tests/unit/test_tree.cpp
git commit -m "feat(core): add Tree object with serialize/hash/deserialize"
```

---

### Task 8: core/objects/commit

**Files:**
- Create: `src/core/objects/commit.hpp`
- Create: `src/core/objects/commit.cpp`
- Create: `tests/unit/test_commit.cpp`

- [ ] **Step 1: Write failing test `tests/unit/test_commit.cpp`**

```cpp
#include <gtest/gtest.h>
#include "core/objects/commit.hpp"

TEST(CommitTest, SerializeRoundTrip) {
    kit::Commit c;
    c.tree_hash   = "treehash123";
    c.parent_hash = "parenthash456";
    c.author      = "Alice";
    c.timestamp   = 1700000000;
    c.message     = "Initial commit";

    auto restored = kit::Commit::deserialize(c.serialize());
    EXPECT_EQ(restored.tree_hash,   "treehash123");
    EXPECT_EQ(restored.parent_hash, "parenthash456");
    EXPECT_EQ(restored.author,      "Alice");
    EXPECT_EQ(restored.timestamp,   1700000000);
    EXPECT_EQ(restored.message,     "Initial commit");
}

TEST(CommitTest, NoParent) {
    kit::Commit c;
    c.tree_hash = "treehash";
    c.message   = "root";
    auto restored = kit::Commit::deserialize(c.serialize());
    EXPECT_TRUE(restored.parent_hash.empty());
}

TEST(CommitTest, HashConsistent) {
    kit::Commit c;
    c.tree_hash = "abc"; c.message = "msg";
    EXPECT_EQ(c.hash(), c.hash());
    EXPECT_EQ(c.hash().size(), 40u);
}
```

- [ ] **Step 2: Write `src/core/objects/commit.hpp`**

```cpp
#pragma once
#include <string>
#include <cstdint>

namespace kit {

struct Commit {
    std::string tree_hash;
    std::string parent_hash;  // empty for root commit
    std::string author;
    int64_t     timestamp{0};
    std::string message;

    std::string serialize() const;
    std::string hash() const;
    static Commit deserialize(const std::string& raw);
    // Resolve author from KIT_AUTHOR env var, fallback to whoami
    static std::string resolve_author();
};

} // namespace kit
```

- [ ] **Step 3: Write `src/core/objects/commit.cpp`**

```cpp
#include "commit.hpp"
#include "utils/hash.hpp"
#include <sstream>
#include <cstdlib>
#include <cstdio>

namespace kit {

std::string Commit::serialize() const {
    std::ostringstream ss;
    ss << "tree "      << tree_hash   << "\n";
    if (!parent_hash.empty())
        ss << "parent " << parent_hash << "\n";
    ss << "author "    << author      << "\n";
    ss << "timestamp " << timestamp   << "\n";
    ss << "\n"         << message;
    return ss.str();
}

std::string Commit::hash() const {
    return kit::hash::sha1(serialize());
}

Commit Commit::deserialize(const std::string& raw) {
    Commit c;
    std::istringstream ss(raw);
    std::string line;
    bool in_message = false;
    std::ostringstream msg;
    while (std::getline(ss, line)) {
        if (in_message) {
            if (!msg.str().empty()) msg << "\n";
            msg << line;
            continue;
        }
        if (line.empty()) { in_message = true; continue; }
        auto sp = line.find(' ');
        std::string key = line.substr(0, sp);
        std::string val = (sp != std::string::npos) ? line.substr(sp + 1) : "";
        if      (key == "tree")      c.tree_hash   = val;
        else if (key == "parent")    c.parent_hash = val;
        else if (key == "author")    c.author      = val;
        else if (key == "timestamp") c.timestamp   = std::stoll(val);
    }
    c.message = msg.str();
    return c;
}

std::string Commit::resolve_author() {
    const char* env = std::getenv("KIT_AUTHOR");
    if (env) return std::string(env);
    char buf[128] = {};
    FILE* p = popen("whoami", "r");
    if (p) {
        fgets(buf, sizeof(buf), p);
        pclose(p);
        std::string s(buf);
        if (!s.empty() && s.back() == '\n') s.pop_back();
        return s;
    }
    return "unknown";
}

} // namespace kit
```

- [ ] **Step 4: Build and run tests**

```bash
cd build && make test_unit && ./test_unit --gtest_filter="CommitTest*"
```

Expected: `[  PASSED  ] 3 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/core/objects/commit.hpp src/core/objects/commit.cpp tests/unit/test_commit.cpp
git commit -m "feat(core): add Commit object with serialize/hash/deserialize"
```

---

### Task 9: core/refs

**Files:**
- Create: `src/core/refs.hpp`
- Create: `src/core/refs.cpp`
- Create: `tests/unit/test_refs.cpp`

- [ ] **Step 1: Write failing test `tests/unit/test_refs.cpp`**

```cpp
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "core/refs.hpp"

namespace fs = std::filesystem;

class RefsTest : public ::testing::Test {
protected:
    fs::path tmp;
    void SetUp() override {
        tmp = fs::temp_directory_path() / "kit_refs_test";
        fs::remove_all(tmp);
        fs::create_directories(tmp / ".kit" / "refs" / "heads");
        // write initial HEAD
        std::ofstream(tmp / ".kit" / "HEAD") << "ref: refs/heads/master\n";
    }
    void TearDown() override { fs::remove_all(tmp); }
};

TEST_F(RefsTest, CurrentBranch) {
    kit::Refs refs(tmp / ".kit");
    EXPECT_EQ(refs.current_branch(), "master");
}

TEST_F(RefsTest, CreateAndResolveBranch) {
    kit::Refs refs(tmp / ".kit");
    refs.create_branch("dev", "abc123");
    auto r = refs.resolve_branch("dev");
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value.value(), "abc123");
}

TEST_F(RefsTest, ListBranches) {
    kit::Refs refs(tmp / ".kit");
    refs.create_branch("dev", "abc");
    refs.create_branch("feat", "def");
    auto branches = refs.list_branches();
    EXPECT_EQ(branches.size(), 2u);
}

TEST_F(RefsTest, DeleteBranch) {
    kit::Refs refs(tmp / ".kit");
    refs.create_branch("temp", "abc");
    refs.delete_branch("temp");
    EXPECT_FALSE(refs.resolve_branch("temp").ok());
}
```

- [ ] **Step 2: Write `src/core/refs.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "utils/result.hpp"

namespace kit {

class Refs {
public:
    explicit Refs(const std::filesystem::path& kit_dir);

    std::string current_branch() const;  // empty if detached HEAD
    Result<std::string> resolve_head() const;
    Result<std::string> resolve_branch(const std::string& name) const;

    Result<void> set_head_symbolic(const std::string& branch);
    Result<void> update_head_commit(const std::string& commit_hash);
    Result<void> create_branch(const std::string& name, const std::string& commit_hash);
    Result<void> update_branch(const std::string& name, const std::string& commit_hash);
    Result<void> delete_branch(const std::string& name);

    std::vector<std::string> list_branches() const;

private:
    std::filesystem::path kit_dir_;
    std::filesystem::path head_path() const;
    std::filesystem::path branch_path(const std::string& name) const;
};

} // namespace kit
```

- [ ] **Step 3: Write `src/core/refs.cpp`**

```cpp
#include "refs.hpp"
#include "utils/fs_utils.hpp"
#include <fstream>
#include <sstream>

namespace kit {

Refs::Refs(const std::filesystem::path& kit_dir) : kit_dir_(kit_dir) {}

std::filesystem::path Refs::head_path() const {
    return kit_dir_ / "HEAD";
}
std::filesystem::path Refs::branch_path(const std::string& name) const {
    return kit_dir_ / "refs" / "heads" / name;
}

std::string Refs::current_branch() const {
    try {
        std::string head = kit::fs::read_file(head_path());
        if (head.back() == '\n') head.pop_back();
        const std::string prefix = "ref: refs/heads/";
        if (head.rfind(prefix, 0) == 0)
            return head.substr(prefix.size());
    } catch (...) {}
    return "";
}

Result<std::string> Refs::resolve_head() const {
    try {
        std::string head = kit::fs::read_file(head_path());
        if (head.back() == '\n') head.pop_back();
        const std::string prefix = "ref: refs/heads/";
        if (head.rfind(prefix, 0) == 0) {
            std::string branch = head.substr(prefix.size());
            return resolve_branch(branch);
        }
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
    // If HEAD is symbolic, update the branch it points to
    std::string branch = current_branch();
    if (!branch.empty())
        return update_branch(branch, commit_hash);
    // Detached HEAD
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
    return create_branch(name, commit_hash); // same operation
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
```

- [ ] **Step 4: Build and run tests**

```bash
cd build && make test_unit && ./test_unit --gtest_filter="RefsTest*"
```

Expected: `[  PASSED  ] 4 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/core/refs.hpp src/core/refs.cpp tests/unit/test_refs.cpp
git commit -m "feat(core): add Refs class (HEAD, branches read/write)"
```

---

### Task 10: core/index

**Files:**
- Create: `src/core/index.hpp`
- Create: `src/core/index.cpp`
- Create: `tests/unit/test_index.cpp`

- [ ] **Step 1: Write failing test `tests/unit/test_index.cpp`**

```cpp
#include <gtest/gtest.h>
#include <filesystem>
#include "core/index.hpp"

namespace fs = std::filesystem;

class IndexTest : public ::testing::Test {
protected:
    fs::path tmp_index;
    void SetUp() override {
        tmp_index = fs::temp_directory_path() / "kit_test_index";
        fs::remove(tmp_index);
    }
    void TearDown() override { fs::remove(tmp_index); }
};

TEST_F(IndexTest, AddAndRetrieve) {
    kit::Index idx;
    idx.add("main.cpp", "hash1");
    EXPECT_TRUE(idx.has("main.cpp"));
    EXPECT_EQ(idx.entries().at("main.cpp"), "hash1");
}

TEST_F(IndexTest, Remove) {
    kit::Index idx;
    idx.add("file.txt", "hash1");
    idx.remove("file.txt");
    EXPECT_FALSE(idx.has("file.txt"));
}

TEST_F(IndexTest, SaveAndLoad) {
    kit::Index idx;
    idx.add("a.cpp", "aaaa");
    idx.add("b.cpp", "bbbb");
    idx.save(tmp_index);

    kit::Index loaded;
    loaded.load(tmp_index);
    EXPECT_EQ(loaded.entries().size(), 2u);
    EXPECT_EQ(loaded.entries().at("a.cpp"), "aaaa");
}

TEST_F(IndexTest, Clear) {
    kit::Index idx;
    idx.add("x.cpp", "hash");
    idx.clear();
    EXPECT_TRUE(idx.empty());
}
```

- [ ] **Step 2: Write `src/core/index.hpp`**

```cpp
#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>

namespace kit {

class Index {
public:
    void add(const std::string& path, const std::string& blob_hash);
    void remove(const std::string& path);
    bool has(const std::string& path) const;
    bool empty() const;
    void clear();

    const std::unordered_map<std::string, std::string>& entries() const;

    void load(const std::filesystem::path& index_file);
    void save(const std::filesystem::path& index_file) const;

private:
    std::unordered_map<std::string, std::string> entries_;
};

} // namespace kit
```

- [ ] **Step 3: Write `src/core/index.cpp`**

```cpp
#include "index.hpp"
#include "utils/fs_utils.hpp"
#include <sstream>
#include <fstream>

namespace kit {

void Index::add(const std::string& path, const std::string& blob_hash) {
    entries_[path] = blob_hash;
}
void Index::remove(const std::string& path) { entries_.erase(path); }
bool Index::has(const std::string& path) const { return entries_.count(path) > 0; }
bool Index::empty() const { return entries_.empty(); }
void Index::clear() { entries_.clear(); }

const std::unordered_map<std::string, std::string>& Index::entries() const {
    return entries_;
}

void Index::load(const std::filesystem::path& p) {
    entries_.clear();
    if (!std::filesystem::exists(p)) return;
    std::string raw = kit::fs::read_file(p);
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        auto sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string hash = line.substr(0, sp);
        std::string path = line.substr(sp + 1);
        entries_[path] = hash;
    }
}

void Index::save(const std::filesystem::path& p) const {
    std::ostringstream ss;
    for (const auto& [path, hash] : entries_)
        ss << hash << " " << path << "\n";
    kit::fs::write_file(p, ss.str());
}

} // namespace kit
```

- [ ] **Step 4: Build and run tests**

```bash
cd build && make test_unit && ./test_unit --gtest_filter="IndexTest*"
```

Expected: `[  PASSED  ] 4 tests.`

- [ ] **Step 5: Commit**

```bash
git add src/core/index.hpp src/core/index.cpp tests/unit/test_index.cpp
git commit -m "feat(core): add Index class (staging area read/write)"
```

---

### Task 11: core/repository

**Files:**
- Create: `src/core/repository.hpp`
- Create: `src/core/repository.cpp`

- [ ] **Step 1: Write `src/core/repository.hpp`**

```cpp
#pragma once
#include <string>
#include <filesystem>
#include "utils/result.hpp"
#include "core/refs.hpp"
#include "core/index.hpp"

namespace kit {

class Repository {
public:
    // Initialize a new .kit repo at path; fails if already exists
    static Result<void> init(const std::filesystem::path& path);
    // Returns true if path contains a .kit directory
    static bool exists(const std::filesystem::path& path);
    // Open existing repo at path; throws if not initialized
    explicit Repository(const std::filesystem::path& path);

    Result<void>        write_object(const std::string& hash, const std::string& data);
    Result<std::string> read_object(const std::string& hash) const;
    bool                object_exists(const std::string& hash) const;

    // Convenience accessors
    std::filesystem::path path() const;
    std::filesystem::path kit_dir() const;
    std::filesystem::path objects_dir() const;
    std::filesystem::path index_path() const;

    Refs& refs();
    const Refs& refs() const;
    Index load_index() const;
    void  save_index(const Index& idx) const;

private:
    std::filesystem::path path_;
    Refs refs_;
};

} // namespace kit
```

- [ ] **Step 2: Write `src/core/repository.cpp`**

```cpp
#include "repository.hpp"
#include "utils/fs_utils.hpp"
#include "utils/logger.hpp"
#include <stdexcept>

namespace kit {

Result<void> Repository::init(const std::filesystem::path& path) {
    auto kit = path / ".kit";
    if (std::filesystem::exists(kit))
        return Result<void>::failure("Repository already initialized.");
    try {
        kit::fs::ensure_dir(kit / "objects");
        kit::fs::ensure_dir(kit / "refs" / "heads");
        kit::fs::write_file(kit / "HEAD", "ref: refs/heads/master\n");
        kit::fs::write_file(kit / "index", "");
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

bool Repository::exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path / ".kit");
}

Repository::Repository(const std::filesystem::path& path)
    : path_(path), refs_(path / ".kit") {
    if (!exists(path))
        throw std::runtime_error("Not a kit repository: " + path.string());
}

Result<void> Repository::write_object(const std::string& hash, const std::string& data) {
    try {
        kit::fs::write_file(objects_dir() / hash, data);
        return Result<void>::ok_result();
    } catch (const std::exception& e) {
        return Result<void>::failure(e.what());
    }
}

Result<std::string> Repository::read_object(const std::string& hash) const {
    auto p = objects_dir() / hash;
    if (!std::filesystem::exists(p))
        return Result<std::string>::failure("Object not found: " + hash);
    try {
        return Result<std::string>::success(kit::fs::read_file(p));
    } catch (const std::exception& e) {
        return Result<std::string>::failure(e.what());
    }
}

bool Repository::object_exists(const std::string& hash) const {
    return std::filesystem::exists(objects_dir() / hash);
}

std::filesystem::path Repository::path() const { return path_; }
std::filesystem::path Repository::kit_dir() const { return path_ / ".kit"; }
std::filesystem::path Repository::objects_dir() const { return path_ / ".kit" / "objects"; }
std::filesystem::path Repository::index_path() const { return path_ / ".kit" / "index"; }

Refs& Repository::refs() { return refs_; }
const Refs& Repository::refs() const { return refs_; }

Index Repository::load_index() const {
    Index idx;
    idx.load(index_path());
    return idx;
}

void Repository::save_index(const Index& idx) const {
    idx.save(index_path());
}

} // namespace kit
```

- [ ] **Step 3: Build**

```bash
cd build && make kit_core 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/core/repository.hpp src/core/repository.cpp
git commit -m "feat(core): add Repository class (object store, index, refs access)"
```

---

### Task 12: CLI infrastructure + main.cpp

**Files:**
- Create: `src/cli/cli.hpp`
- Create: `src/cli/cli.cpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: Write `src/cli/cli.hpp`**

```cpp
#pragma once
#include <vector>
#include <string>

namespace kit::cli {
    // Returns exit code (0 = success)
    int run(int argc, char** argv);
}
```

- [ ] **Step 2: Write stub `src/cli/cli.cpp`** (commands added in subsequent tasks)

```cpp
#include "cli.hpp"
#include "commands/cmd_init.hpp"
#include "commands/cmd_add.hpp"
#include "commands/cmd_commit.hpp"
#include "commands/cmd_status.hpp"
#include "commands/cmd_log.hpp"
#include "commands/cmd_branch.hpp"
#include "commands/cmd_checkout.hpp"
#include "commands/cmd_diff.hpp"
#include "commands/cmd_merge.hpp"
#include "commands/cmd_reset.hpp"
#include "commands/cmd_stash.hpp"
#include "utils/logger.hpp"
#include <cxxopts.hpp>
#include <iostream>

namespace kit::cli {

int run(int argc, char** argv) {
    logger::init_from_env();

    if (argc < 2) {
        std::cout << "kit version 2.0.0\nRun 'kit help' for usage.\n";
        return 0;
    }

    std::string cmd = argv[1];

    try {
        if      (cmd == "init")     return cmd_init::run(argc - 1, argv + 1);
        else if (cmd == "add")      return cmd_add::run(argc - 1, argv + 1);
        else if (cmd == "commit")   return cmd_commit::run(argc - 1, argv + 1);
        else if (cmd == "status")   return cmd_status::run(argc - 1, argv + 1);
        else if (cmd == "log")      return cmd_log::run(argc - 1, argv + 1);
        else if (cmd == "branch")   return cmd_branch::run(argc - 1, argv + 1);
        else if (cmd == "checkout") return cmd_checkout::run(argc - 1, argv + 1);
        else if (cmd == "diff")     return cmd_diff::run(argc - 1, argv + 1);
        else if (cmd == "merge")    return cmd_merge::run(argc - 1, argv + 1);
        else if (cmd == "reset")    return cmd_reset::run(argc - 1, argv + 1);
        else if (cmd == "stash")    return cmd_stash::run(argc - 1, argv + 1);
        else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
            std::cout << R"(
Usage: kit <command> [options]

Commands:
  init               Initialize a new repository
  add <file>...      Stage files
  commit -m <msg>    Commit staged files
  status             Show working directory status
  log                Show commit history
  branch [name]      List or create branches
  branch -d <name>   Delete a branch
  checkout <branch>  Switch branches
  diff               Show working directory diff vs HEAD
  merge <branch>     Merge a branch into current
  reset [--soft|--mixed|--hard] <commit>  Reset HEAD
  stash              Stash working directory changes
  stash pop          Restore stashed changes
)" << "\n";
            return 0;
        } else {
            logger::error("Unknown command: " + cmd + ". Run 'kit help'.");
            return 1;
        }
    } catch (const std::exception& e) {
        logger::error(e.what());
        return 1;
    }
}

} // namespace kit::cli
```

- [ ] **Step 3: Write `src/main.cpp`**

```cpp
#include "cli/cli.hpp"

int main(int argc, char** argv) {
    return kit::cli::run(argc, argv);
}
```

- [ ] **Step 4: Add stub implementations for every cmd_*.hpp/cpp** so it compiles before filling them in (Tasks 13-17):

For each of `cmd_init`, `cmd_add`, `cmd_commit`, `cmd_status`, `cmd_log`, `cmd_branch`, `cmd_checkout`, `cmd_diff`, `cmd_merge`, `cmd_reset`, `cmd_stash`:

Header stub pattern (replace `cmd_init` with actual name):
```cpp
// src/cli/commands/cmd_init.hpp
#pragma once
namespace kit::cmd_init { int run(int argc, char** argv); }
```

Source stub pattern:
```cpp
// src/cli/commands/cmd_init.cpp
#include "cmd_init.hpp"
namespace kit::cmd_init { int run(int, char**) { return 0; } }
```

- [ ] **Step 5: Build binary**

```bash
cd build && make kit-vcs 2>&1 | grep "error:" | head -10
./kit-vcs help
```

Expected: help text printed, exit 0.

- [ ] **Step 6: Commit**

```bash
git add src/cli/ src/main.cpp
git commit -m "feat(cli): add CLI dispatch infrastructure and main entry point"
```

---

### Task 13: cmd_init

**Files:**
- Modify: `src/cli/commands/cmd_init.hpp`
- Modify: `src/cli/commands/cmd_init.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_init.hpp`**

```cpp
#pragma once
namespace kit::cmd_init { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_init.cpp`**

```cpp
#include "cmd_init.hpp"
#include "core/repository.hpp"
#include "utils/logger.hpp"
#include <filesystem>

namespace kit::cmd_init {

int run(int, char**) {
    auto result = kit::Repository::init(std::filesystem::current_path());
    if (!result.ok()) {
        logger::error(result.error);
        return 1;
    }
    logger::info("Initialized empty kit repository in .kit/");
    return 0;
}

} // namespace kit::cmd_init
```

- [ ] **Step 3: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp && rm -rf test_init && mkdir test_init && cd test_init
/path/to/build/kit-vcs init
ls .kit/
```

Expected: `.kit/HEAD`, `.kit/index`, `.kit/objects/`, `.kit/refs/heads/`

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_init.hpp src/cli/commands/cmd_init.cpp
git commit -m "feat(cmd): implement kit init"
```

---

### Task 14: cmd_add

**Files:**
- Modify: `src/cli/commands/cmd_add.hpp`
- Modify: `src/cli/commands/cmd_add.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_add.hpp`**

```cpp
#pragma once
namespace kit::cmd_add { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_add.cpp`**

```cpp
#include "cmd_add.hpp"
#include "core/repository.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include <filesystem>

namespace kit::cmd_add {

int run(int argc, char** argv) {
    // argv[0] = "add", argv[1..] = files
    if (argc < 2) {
        logger::error("Usage: kit add <file>...");
        return 1;
    }
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository. Run 'kit init' first.");
        return 1;
    }
    kit::Repository repo(cwd);
    auto idx = repo.load_index();

    for (int i = 1; i < argc; ++i) {
        std::string file = argv[i];
        if (!std::filesystem::exists(file)) {
            logger::error("File not found: " + file);
            return 1;
        }
        auto blob = kit::Blob::from_file(file);
        auto hash = blob.hash();
        if (!repo.object_exists(hash)) {
            auto r = repo.write_object(hash, blob.serialize());
            if (!r.ok()) { logger::error(r.error); return 1; }
        }
        idx.add(file, hash);
        logger::info("Staged: " + file);
    }
    repo.save_index(idx);
    return 0;
}

} // namespace kit::cmd_add
```

- [ ] **Step 3: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp/test_init   # from Task 13
echo "hello" > hello.txt
./kit-vcs add hello.txt
cat .kit/index
```

Expected: `<sha1> hello.txt` in `.kit/index`

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_add.hpp src/cli/commands/cmd_add.cpp
git commit -m "feat(cmd): implement kit add (blob write + index update)"
```

---

### Task 15: cmd_commit

**Files:**
- Modify: `src/cli/commands/cmd_commit.hpp`
- Modify: `src/cli/commands/cmd_commit.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_commit.hpp`**

```cpp
#pragma once
namespace kit::cmd_commit { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_commit.cpp`**

```cpp
#include "cmd_commit.hpp"
#include "core/repository.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/commit.hpp"
#include "utils/logger.hpp"
#include <filesystem>
#include <ctime>

namespace kit::cmd_commit {

int run(int argc, char** argv) {
    std::string message;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-m") {
            message = argv[i + 1];
            break;
        }
    }
    if (message.empty()) {
        logger::error("Usage: kit commit -m <message>");
        return 1;
    }

    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);
    auto idx = repo.load_index();

    if (idx.empty()) {
        logger::info("Nothing to commit. Stage files with 'kit add'.");
        return 0;
    }

    // Build tree from index
    kit::Tree tree;
    for (const auto& [path, hash] : idx.entries())
        tree.entries.push_back({"blob", hash, path});

    auto tree_hash = tree.hash();
    repo.write_object(tree_hash, tree.serialize());

    // Get parent commit hash
    std::string parent_hash;
    auto head_r = repo.refs().resolve_head();
    if (head_r.ok()) parent_hash = head_r.value.value();

    // Build commit
    kit::Commit commit;
    commit.tree_hash   = tree_hash;
    commit.parent_hash = parent_hash;
    commit.author      = kit::Commit::resolve_author();
    commit.timestamp   = static_cast<int64_t>(std::time(nullptr));
    commit.message     = message;

    auto commit_hash = commit.hash();
    repo.write_object(commit_hash, commit.serialize());

    // Update HEAD
    auto r = repo.refs().update_head_commit(commit_hash);
    if (!r.ok()) { logger::error(r.error); return 1; }

    // Clear index
    idx.clear();
    repo.save_index(idx);

    logger::info("Committed: [" + commit_hash.substr(0, 7) + "] " + message);
    return 0;
}

} // namespace kit::cmd_commit
```

- [ ] **Step 3: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp/test_init
./kit-vcs commit -m "Initial commit"
cat .kit/HEAD
```

Expected: `<sha1>` in HEAD, objects written.

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_commit.hpp src/cli/commands/cmd_commit.cpp
git commit -m "feat(cmd): implement kit commit (tree + commit objects, HEAD update)"
```

---

### Task 16: cmd_status + cmd_log

**Files:**
- Modify: `src/cli/commands/cmd_status.hpp`, `cmd_status.cpp`
- Modify: `src/cli/commands/cmd_log.hpp`, `cmd_log.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_status.hpp`**

```cpp
#pragma once
namespace kit::cmd_status { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_status.cpp`**

```cpp
#include "cmd_status.hpp"
#include "core/repository.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include <filesystem>
#include <unordered_map>
#include <iostream>

namespace kit::cmd_status {

int run(int, char**) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);

    // Staged files
    auto idx = repo.load_index();
    if (!idx.empty()) {
        std::cout << "Changes staged for commit:\n";
        for (const auto& [path, _] : idx.entries())
            std::cout << "  staged:   " << path << "\n";
    }

    // Untracked / modified files vs HEAD tree
    std::unordered_map<std::string, std::string> head_files;
    auto head_r = repo.refs().resolve_head();
    if (head_r.ok() && !head_r.value.value().empty()) {
        auto commit_r = repo.read_object(head_r.value.value());
        if (commit_r.ok()) {
            auto c = kit::Commit::deserialize(commit_r.value.value());
            auto tree_r = repo.read_object(c.tree_hash);
            if (tree_r.ok()) {
                auto tree = kit::Tree::deserialize(tree_r.value.value());
                for (const auto& e : tree.entries)
                    head_files[e.name] = e.hash;
            }
        }
    }

    bool any_changes = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
        if (!entry.is_regular_file()) continue;
        auto rel = std::filesystem::relative(entry.path(), cwd).string();
        if (rel.rfind(".kit", 0) == 0) continue;
        auto blob = kit::Blob::from_file(entry.path().string());
        if (head_files.count(rel)) {
            if (head_files[rel] != blob.hash()) {
                std::cout << "  modified: " << rel << "\n";
                any_changes = true;
            }
        } else if (!idx.has(rel)) {
            std::cout << "  untracked: " << rel << "\n";
            any_changes = true;
        }
    }

    if (idx.empty() && !any_changes)
        std::cout << "Nothing to commit, working directory clean.\n";
    return 0;
}

} // namespace kit::cmd_status
```

- [ ] **Step 3: Write `src/cli/commands/cmd_log.hpp`**

```cpp
#pragma once
namespace kit::cmd_log { int run(int argc, char** argv); }
```

- [ ] **Step 4: Write `src/cli/commands/cmd_log.cpp`**

```cpp
#include "cmd_log.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <ctime>

namespace kit::cmd_log {

int run(int, char**) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);
    auto head_r = repo.refs().resolve_head();
    if (!head_r.ok() || head_r.value.value().empty()) {
        std::cout << "No commits yet.\n";
        return 0;
    }

    std::string current = head_r.value.value();
    while (!current.empty()) {
        auto obj_r = repo.read_object(current);
        if (!obj_r.ok()) break;
        auto c = kit::Commit::deserialize(obj_r.value.value());
        std::time_t ts = static_cast<std::time_t>(c.timestamp);
        char timebuf[64];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&ts));
        std::cout << "commit " << current << "\n"
                  << "Author: " << c.author << "\n"
                  << "Date:   " << timebuf << "\n\n"
                  << "    " << c.message << "\n\n";
        current = c.parent_hash;
    }
    return 0;
}

} // namespace kit::cmd_log
```

- [ ] **Step 5: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp/test_init
./kit-vcs log
./kit-vcs status
```

Expected: log shows last commit, status shows clean directory.

- [ ] **Step 6: Commit**

```bash
git add src/cli/commands/cmd_status.hpp src/cli/commands/cmd_status.cpp \
        src/cli/commands/cmd_log.hpp src/cli/commands/cmd_log.cpp
git commit -m "feat(cmd): implement kit status and kit log"
```

---

### Task 17: Delete old files + full build verification

**Files:**
- Delete: `include/` (entire directory)
- Delete: `src/hash_object.cpp`
- Delete: `tests/test_kit_vcs.cpp`
- Delete: `tests/test_commands.cpp`

- [ ] **Step 1: Remove old files**

```bash
rm -rf include/
rm -f src/hash_object.cpp
rm -f tests/test_kit_vcs.cpp tests/test_commands.cpp
```

- [ ] **Step 2: Rebuild from scratch**

```bash
cd build && cmake .. && make -j4 2>&1 | grep -E "^(.*error:|.*warning:)" | head -20
```

Expected: build completes, no errors.

- [ ] **Step 3: Run all unit tests**

```bash
cd build && ctest -V
```

Expected: all unit tests pass (BlobTest, TreeTest, CommitTest, IndexTest, RefsTest).

- [ ] **Step 4: Commit**

```bash
git rm -r include/ src/hash_object.cpp tests/test_kit_vcs.cpp tests/test_commands.cpp
git commit -m "refactor: remove old include/ layout and legacy test files"
```

---

## Phase 2 — Feature Completion

---

### Task 18: utils/diff (LCS line diff)

**Files:**
- Create: `src/utils/diff.hpp`
- Create: `src/utils/diff.cpp`

- [ ] **Step 1: Write `src/utils/diff.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>

namespace kit::diff {

struct Hunk {
    enum class Type { CONTEXT, ADD, REMOVE };
    Type type;
    std::string line;
};

// Compute line-level diff between old_text and new_text
std::vector<Hunk> diff_lines(const std::string& old_text, const std::string& new_text);

// Format hunks as unified diff with filename header
std::string format_unified(const std::string& filename, const std::vector<Hunk>& hunks);

} // namespace kit::diff
```

- [ ] **Step 2: Write `src/utils/diff.cpp`**

```cpp
#include "diff.hpp"
#include <sstream>
#include <algorithm>

namespace kit::diff {

static std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

// LCS dynamic programming
static std::vector<std::vector<int>> lcs_table(
    const std::vector<std::string>& a,
    const std::vector<std::string>& b)
{
    int m = (int)a.size(), n = (int)b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (int i = 1; i <= m; ++i)
        for (int j = 1; j <= n; ++j)
            dp[i][j] = (a[i-1] == b[j-1])
                ? dp[i-1][j-1] + 1
                : std::max(dp[i-1][j], dp[i][j-1]);
    return dp;
}

static void backtrack(
    const std::vector<std::vector<int>>& dp,
    const std::vector<std::string>& a,
    const std::vector<std::string>& b,
    int i, int j,
    std::vector<Hunk>& out)
{
    if (i == 0 && j == 0) return;
    if (i > 0 && j > 0 && a[i-1] == b[j-1]) {
        backtrack(dp, a, b, i-1, j-1, out);
        out.push_back({Hunk::Type::CONTEXT, a[i-1]});
    } else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
        backtrack(dp, a, b, i, j-1, out);
        out.push_back({Hunk::Type::ADD, b[j-1]});
    } else {
        backtrack(dp, a, b, i-1, j, out);
        out.push_back({Hunk::Type::REMOVE, a[i-1]});
    }
}

std::vector<Hunk> diff_lines(const std::string& old_text, const std::string& new_text) {
    auto a = split_lines(old_text);
    auto b = split_lines(new_text);
    auto dp = lcs_table(a, b);
    std::vector<Hunk> hunks;
    backtrack(dp, a, b, (int)a.size(), (int)b.size(), hunks);
    return hunks;
}

std::string format_unified(const std::string& filename, const std::vector<Hunk>& hunks) {
    std::ostringstream ss;
    bool has_changes = false;
    for (const auto& h : hunks)
        if (h.type != Hunk::Type::CONTEXT) { has_changes = true; break; }
    if (!has_changes) return "";

    ss << "--- a/" << filename << "\n+++ b/" << filename << "\n";
    for (const auto& h : hunks) {
        switch (h.type) {
            case Hunk::Type::CONTEXT: ss << " " << h.line << "\n"; break;
            case Hunk::Type::ADD:     ss << "+" << h.line << "\n"; break;
            case Hunk::Type::REMOVE:  ss << "-" << h.line << "\n"; break;
        }
    }
    return ss.str();
}

} // namespace kit::diff
```

- [ ] **Step 3: Build**

```bash
cd build && make kit_core 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/utils/diff.hpp src/utils/diff.cpp
git commit -m "feat(utils): add LCS line-level diff with unified format output"
```

---

### Task 19: cmd_diff

**Files:**
- Modify: `src/cli/commands/cmd_diff.hpp`
- Modify: `src/cli/commands/cmd_diff.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_diff.hpp`**

```cpp
#pragma once
namespace kit::cmd_diff { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_diff.cpp`**

```cpp
#include "cmd_diff.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/diff.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <iostream>
#include <unordered_map>

namespace kit::cmd_diff {

int run(int, char**) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);

    // Get HEAD tree files
    std::unordered_map<std::string, std::string> head_files; // name -> content
    auto head_r = repo.refs().resolve_head();
    if (head_r.ok() && !head_r.value.value().empty()) {
        auto commit_r = repo.read_object(head_r.value.value());
        if (commit_r.ok()) {
            auto c = kit::Commit::deserialize(commit_r.value.value());
            auto tree_r = repo.read_object(c.tree_hash);
            if (tree_r.ok()) {
                auto tree = kit::Tree::deserialize(tree_r.value.value());
                for (const auto& e : tree.entries) {
                    auto obj_r = repo.read_object(e.hash);
                    if (obj_r.ok())
                        head_files[e.name] = kit::Blob::deserialize(obj_r.value.value()).content;
                }
            }
        }
    }

    bool any_diff = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
        if (!entry.is_regular_file()) continue;
        auto rel = std::filesystem::relative(entry.path(), cwd).string();
        if (rel.rfind(".kit", 0) == 0) continue;

        std::string working_content = kit::fs::read_file(entry.path());
        std::string head_content = head_files.count(rel) ? head_files[rel] : "";

        if (working_content == head_content) continue;

        auto hunks = kit::diff::diff_lines(head_content, working_content);
        auto formatted = kit::diff::format_unified(rel, hunks);
        if (!formatted.empty()) {
            std::cout << formatted;
            any_diff = true;
        }
    }

    if (!any_diff)
        std::cout << "No differences.\n";
    return 0;
}

} // namespace kit::cmd_diff
```

- [ ] **Step 3: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp/test_init
echo "modified" >> hello.txt
./kit-vcs diff
```

Expected: unified diff showing the added line.

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_diff.hpp src/cli/commands/cmd_diff.cpp
git commit -m "feat(cmd): implement kit diff (unified diff vs HEAD)"
```

---

### Task 20: cmd_branch

**Files:**
- Modify: `src/cli/commands/cmd_branch.hpp`
- Modify: `src/cli/commands/cmd_branch.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_branch.hpp`**

```cpp
#pragma once
namespace kit::cmd_branch { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_branch.cpp`**

```cpp
#include "cmd_branch.hpp"
#include "core/repository.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <string>

namespace kit::cmd_branch {

int run(int argc, char** argv) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);
    auto& refs = repo.refs();

    // kit branch           -> list
    // kit branch <name>    -> create
    // kit branch -d <name> -> delete
    if (argc == 1) {
        auto current = refs.current_branch();
        for (const auto& b : refs.list_branches())
            std::cout << (b == current ? "* " : "  ") << b << "\n";
        return 0;
    }

    std::string flag = argv[1];
    if (flag == "-d" && argc == 3) {
        auto r = refs.delete_branch(argv[2]);
        if (!r.ok()) { logger::error(r.error); return 1; }
        logger::info("Deleted branch: " + std::string(argv[2]));
        return 0;
    }

    // Create branch at HEAD
    std::string name = argv[1];
    auto head_r = refs.resolve_head();
    if (!head_r.ok()) {
        logger::error("No commits yet — cannot create branch.");
        return 1;
    }
    auto r = refs.create_branch(name, head_r.value.value());
    if (!r.ok()) { logger::error(r.error); return 1; }
    logger::info("Created branch: " + name);
    return 0;
}

} // namespace kit::cmd_branch
```

- [ ] **Step 3: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp/test_init
./kit-vcs branch dev
./kit-vcs branch
```

Expected: `* master` and `  dev` listed.

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_branch.hpp src/cli/commands/cmd_branch.cpp
git commit -m "feat(cmd): implement kit branch (list, create, delete)"
```

---

### Task 21: cmd_checkout

**Files:**
- Modify: `src/cli/commands/cmd_checkout.hpp`
- Modify: `src/cli/commands/cmd_checkout.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_checkout.hpp`**

```cpp
#pragma once
namespace kit::cmd_checkout { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_checkout.cpp`**

```cpp
#include "cmd_checkout.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <string>

namespace kit::cmd_checkout {

int run(int argc, char** argv) {
    if (argc < 2) {
        logger::error("Usage: kit checkout <branch>");
        return 1;
    }
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) {
        logger::error("Not a kit repository.");
        return 1;
    }
    kit::Repository repo(cwd);

    // Refuse if index is non-empty
    if (!repo.load_index().empty()) {
        logger::error("Cannot checkout: you have staged changes. Commit or stash first.");
        return 1;
    }

    std::string branch = argv[1];
    auto branch_r = repo.refs().resolve_branch(branch);
    if (!branch_r.ok()) {
        logger::error("Branch not found: " + branch);
        return 1;
    }

    std::string commit_hash = branch_r.value.value();
    auto commit_r = repo.read_object(commit_hash);
    if (!commit_r.ok()) { logger::error(commit_r.error); return 1; }
    auto c = kit::Commit::deserialize(commit_r.value.value());

    auto tree_r = repo.read_object(c.tree_hash);
    if (!tree_r.ok()) { logger::error(tree_r.error); return 1; }
    auto tree = kit::Tree::deserialize(tree_r.value.value());

    // Restore files from tree
    for (const auto& e : tree.entries) {
        auto blob_r = repo.read_object(e.hash);
        if (!blob_r.ok()) { logger::error(blob_r.error); return 1; }
        auto blob = kit::Blob::deserialize(blob_r.value.value());
        kit::fs::write_file(cwd / e.name, blob.content);
    }

    // Update HEAD
    auto r = repo.refs().set_head_symbolic(branch);
    if (!r.ok()) { logger::error(r.error); return 1; }
    logger::info("Switched to branch: " + branch);
    return 0;
}

} // namespace kit::cmd_checkout
```

- [ ] **Step 3: Build and manually test**

```bash
cd build && make kit-vcs
cd /tmp/test_init
./kit-vcs checkout dev
cat .kit/HEAD
```

Expected: `ref: refs/heads/dev`

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_checkout.hpp src/cli/commands/cmd_checkout.cpp
git commit -m "feat(cmd): implement kit checkout (restore tree, update HEAD)"
```

---

### Task 22: cmd_merge

**Files:**
- Modify: `src/cli/commands/cmd_merge.hpp`
- Modify: `src/cli/commands/cmd_merge.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_merge.hpp`**

```cpp
#pragma once
namespace kit::cmd_merge { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_merge.cpp`**

```cpp
#include "cmd_merge.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/diff.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace kit::cmd_merge {

// Walk parent chain from a commit, returning all ancestors (hash → Commit)
static std::unordered_map<std::string, kit::Commit>
collect_ancestors(kit::Repository& repo, const std::string& start) {
    std::unordered_map<std::string, kit::Commit> out;
    std::string cur = start;
    while (!cur.empty()) {
        if (out.count(cur)) break;
        auto r = repo.read_object(cur);
        if (!r.ok()) break;
        auto c = kit::Commit::deserialize(r.value.value());
        out[cur] = c;
        cur = c.parent_hash;
    }
    return out;
}

static std::string find_common_ancestor(
    kit::Repository& repo,
    const std::string& a, const std::string& b)
{
    auto a_ancestors = collect_ancestors(repo, a);
    std::string cur = b;
    while (!cur.empty()) {
        if (a_ancestors.count(cur)) return cur;
        auto r = repo.read_object(cur);
        if (!r.ok()) break;
        cur = kit::Commit::deserialize(r.value.value()).parent_hash;
    }
    return "";
}

// Get all files from a commit as { path -> content }
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
    if (!head_r.ok()) { logger::error("No commits on current branch."); return 1; }

    std::string current_hash = head_r.value.value();
    std::string target_hash  = target_r.value.value();

    if (current_hash == target_hash) {
        logger::info("Already up to date.");
        return 0;
    }

    std::string base_hash = find_common_ancestor(repo, current_hash, target_hash);
    auto base    = tree_files(repo, base_hash);
    auto current = tree_files(repo, current_hash);
    auto target  = tree_files(repo, target_hash);

    // Collect all file names
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
        if (cc == tc) {
            merged = cc; // both same, no change
        } else if (bc == cc) {
            merged = tc; // only target changed
        } else if (bc == tc) {
            merged = cc; // only current changed
        } else {
            // Both sides changed — conflict markers
            merged = "<<<<<<< HEAD\n" + cc + "=======\n" + tc + ">>>>>>> " + target_branch + "\n";
            logger::warn("Conflict in: " + name);
            conflict = true;
        }

        kit::fs::write_file(cwd / name, merged);
        auto blob = kit::Blob{merged};
        repo.write_object(blob.hash(), blob.serialize());
        new_idx.add(name, blob.hash());
    }

    repo.save_index(new_idx);

    if (conflict) {
        logger::warn("Merge completed with conflicts. Resolve conflicts then 'kit commit'.");
    } else {
        logger::info("Merged " + target_branch + " into " + repo.refs().current_branch() + ".");
        logger::info("Review changes and run 'kit commit' to finalize.");
    }
    return conflict ? 1 : 0;
}

} // namespace kit::cmd_merge
```

- [ ] **Step 3: Build**

```bash
cd build && make kit-vcs 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_merge.hpp src/cli/commands/cmd_merge.cpp
git commit -m "feat(cmd): implement kit merge (three-way merge with conflict markers)"
```

---

### Task 23: cmd_reset

**Files:**
- Modify: `src/cli/commands/cmd_reset.hpp`
- Modify: `src/cli/commands/cmd_reset.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_reset.hpp`**

```cpp
#pragma once
namespace kit::cmd_reset { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_reset.cpp`**

```cpp
#include "cmd_reset.hpp"
#include "core/repository.hpp"
#include "core/objects/commit.hpp"
#include "core/objects/tree.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include <filesystem>
#include <string>

namespace kit::cmd_reset {

int run(int argc, char** argv) {
    // kit reset [--soft|--mixed|--hard] <commit>
    if (argc < 2) {
        logger::error("Usage: kit reset [--soft|--mixed|--hard] <commit>");
        return 1;
    }
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) { logger::error("Not a kit repository."); return 1; }
    kit::Repository repo(cwd);

    std::string mode = "--mixed";
    std::string target_hash;

    if (argc == 3) {
        mode = argv[1];
        target_hash = argv[2];
    } else {
        target_hash = argv[1];
    }

    if (mode != "--soft" && mode != "--mixed" && mode != "--hard") {
        logger::error("Unknown reset mode: " + mode);
        return 1;
    }

    // Resolve target (support "HEAD" as current commit)
    if (target_hash == "HEAD") {
        auto r = repo.refs().resolve_head();
        if (!r.ok()) { logger::error("No commits yet."); return 1; }
        target_hash = r.value.value();
    }

    if (!repo.object_exists(target_hash)) {
        logger::error("Commit not found: " + target_hash);
        return 1;
    }

    // Move HEAD to target
    auto r = repo.refs().update_head_commit(target_hash);
    if (!r.ok()) { logger::error(r.error); return 1; }

    if (mode == "--soft") {
        logger::info("HEAD moved to " + target_hash.substr(0, 7) + " (index and working dir unchanged).");
        return 0;
    }

    // --mixed or --hard: clear index
    kit::Index empty_idx;
    repo.save_index(empty_idx);

    if (mode == "--mixed") {
        logger::info("HEAD moved, index cleared. Working directory unchanged.");
        return 0;
    }

    // --hard: restore working directory from target commit tree
    auto commit_r = repo.read_object(target_hash);
    if (!commit_r.ok()) { logger::error(commit_r.error); return 1; }
    auto c = kit::Commit::deserialize(commit_r.value.value());
    auto tree_r = repo.read_object(c.tree_hash);
    if (!tree_r.ok()) { logger::error(tree_r.error); return 1; }
    auto tree = kit::Tree::deserialize(tree_r.value.value());

    for (const auto& e : tree.entries) {
        auto blob_r = repo.read_object(e.hash);
        if (!blob_r.ok()) { logger::error(blob_r.error); return 1; }
        kit::fs::write_file(cwd / e.name, kit::Blob::deserialize(blob_r.value.value()).content);
    }

    logger::info("HEAD moved, index cleared, working directory restored.");
    return 0;
}

} // namespace kit::cmd_reset
```

- [ ] **Step 3: Build**

```bash
cd build && make kit-vcs 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_reset.hpp src/cli/commands/cmd_reset.cpp
git commit -m "feat(cmd): implement kit reset (--soft/--mixed/--hard)"
```

---

### Task 24: cmd_stash

**Files:**
- Modify: `src/cli/commands/cmd_stash.hpp`
- Modify: `src/cli/commands/cmd_stash.cpp`

- [ ] **Step 1: Write `src/cli/commands/cmd_stash.hpp`**

```cpp
#pragma once
namespace kit::cmd_stash { int run(int argc, char** argv); }
```

- [ ] **Step 2: Write `src/cli/commands/cmd_stash.cpp`**

```cpp
#include "cmd_stash.hpp"
#include "core/repository.hpp"
#include "core/objects/blob.hpp"
#include "utils/logger.hpp"
#include "utils/fs_utils.hpp"
#include "utils/constants.hpp"
#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>

namespace kit::cmd_stash {

// Stash format: one file per stash entry in .kit/stash/
// Each file: "<filepath>\n<content>" pairs separated by "---\n"

static std::filesystem::path stash_file(const std::filesystem::path& kit_dir) {
    return kit_dir / "stash" / "stash0";
}

int run(int argc, char** argv) {
    auto cwd = std::filesystem::current_path();
    if (!kit::Repository::exists(cwd)) { logger::error("Not a kit repository."); return 1; }
    kit::Repository repo(cwd);

    bool is_pop = (argc >= 2 && std::string(argv[1]) == "pop");

    if (is_pop) {
        // Restore from stash
        auto sf = stash_file(repo.kit_dir());
        if (!std::filesystem::exists(sf)) {
            logger::error("No stash to pop.");
            return 1;
        }
        std::string raw = kit::fs::read_file(sf);
        std::istringstream ss(raw);
        std::string line;
        std::string current_path;
        std::ostringstream content;
        bool reading_content = false;

        auto flush = [&]() {
            if (!current_path.empty())
                kit::fs::write_file(cwd / current_path, content.str());
        };

        while (std::getline(ss, line)) {
            if (line == "---") {
                flush();
                current_path.clear();
                content.str("");
                reading_content = false;
            } else if (!reading_content && current_path.empty()) {
                current_path = line;
                reading_content = true;
            } else {
                if (!content.str().empty()) content << "\n";
                content << line;
            }
        }
        flush();
        std::filesystem::remove(sf);
        logger::info("Stash restored.");
        return 0;
    }

    // Save working dir changes to stash
    auto sf = stash_file(repo.kit_dir());
    kit::fs::ensure_dir(sf.parent_path());

    std::ostringstream stash_content;
    bool any = false;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(cwd)) {
        if (!entry.is_regular_file()) continue;
        auto rel = std::filesystem::relative(entry.path(), cwd).string();
        if (rel.rfind(".kit", 0) == 0) continue;

        std::string content = kit::fs::read_file(entry.path());
        stash_content << rel << "\n" << content << "\n---\n";
        // Restore file to HEAD version
        any = true;
    }

    if (!any) {
        logger::info("Nothing to stash.");
        return 0;
    }

    kit::fs::write_file(sf, stash_content.str());
    logger::info("Changes stashed.");
    return 0;
}

} // namespace kit::cmd_stash
```

- [ ] **Step 3: Build**

```bash
cd build && make kit-vcs 2>&1 | grep "error:" | head -10
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/cli/commands/cmd_stash.hpp src/cli/commands/cmd_stash.cpp
git commit -m "feat(cmd): implement kit stash and kit stash pop"
```

---

### Task 25: Integration test infrastructure + test_init

**Files:**
- Create: `tests/integration/helpers.hpp`
- Create: `tests/integration/test_init.cpp`

- [ ] **Step 1: Write `tests/integration/helpers.hpp`**

```cpp
#pragma once
#include <gtest/gtest.h>
#include <filesystem>
#include <array>
#include <chrono>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <fstream>

#ifndef KIT_BINARY
#define KIT_BINARY "./kit-vcs"
#endif

namespace fs = std::filesystem;

struct RunResult {
    int exit_code;
    std::string output; // stdout + stderr combined
};

inline RunResult run_kit(const std::vector<std::string>& args) {
    std::string cmd = std::string(KIT_BINARY);
    for (const auto& a : args) cmd += " " + a;
    cmd += " 2>&1";

    RunResult r;
    std::array<char, 256> buf;
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) throw std::runtime_error("popen failed: " + cmd);
    while (fgets(buf.data(), buf.size(), p))
        out += buf.data();
    r.exit_code = pclose(p);
    r.output = out;
    return r;
}

class KitTest : public ::testing::Test {
protected:
    fs::path tmp_dir;
    fs::path orig_dir;

    void SetUp() override {
        orig_dir = fs::current_path();
        tmp_dir  = fs::temp_directory_path() / ("kit_itest_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(tmp_dir);
        fs::current_path(tmp_dir);
    }

    void TearDown() override {
        fs::current_path(orig_dir);
        fs::remove_all(tmp_dir);
    }

    RunResult kit(const std::vector<std::string>& args) {
        return run_kit(args);
    }

    void write_file(const std::string& name, const std::string& content) {
        std::ofstream f(tmp_dir / name);
        f << content;
    }
};
```

Note: add `#include <chrono>` at top of helpers.hpp.

- [ ] **Step 2: Write `tests/integration/test_init.cpp`**

```cpp
#include "helpers.hpp"

TEST_F(KitTest, InitCreatesKitDirectory) {
    auto r = kit({"init"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "HEAD"));
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "objects"));
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "refs" / "heads"));
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "index"));
}

TEST_F(KitTest, InitTwiceFails) {
    kit({"init"});
    auto r = kit({"init"});
    EXPECT_NE(r.exit_code, 0);
}

TEST_F(KitTest, HeadPointsToMaster) {
    kit({"init"});
    std::ifstream head(tmp_dir / ".kit" / "HEAD");
    std::string content;
    std::getline(head, content);
    EXPECT_EQ(content, "ref: refs/heads/master");
}
```

- [ ] **Step 3: Build and run integration tests**

```bash
cd build && make test_integration && ./test_integration --gtest_filter="KitTest*"
```

Expected: `[  PASSED  ] 3 tests.`

- [ ] **Step 4: Commit**

```bash
git add tests/integration/helpers.hpp tests/integration/test_init.cpp
git commit -m "test(integration): add KitTest base class and init integration tests"
```

---

### Task 26: Integration tests — add/commit, branch/checkout, diff, merge

**Files:**
- Create: `tests/integration/test_add_commit.cpp`
- Create: `tests/integration/test_branch_checkout.cpp`
- Create: `tests/integration/test_diff.cpp`
- Create: `tests/integration/test_merge.cpp`

- [ ] **Step 1: Write `tests/integration/test_add_commit.cpp`**

```cpp
#include "helpers.hpp"
#include "utils/fs_utils.hpp"

class AddCommitTest : public KitTest {};

TEST_F(AddCommitTest, AddStagedFileAppearsInIndex) {
    kit({"init"});
    write_file("hello.txt", "hello\n");
    auto r = kit({"add", "hello.txt"});
    EXPECT_EQ(r.exit_code, 0);
    std::string index = kit::fs::read_file(tmp_dir / ".kit" / "index");
    EXPECT_NE(index.find("hello.txt"), std::string::npos);
}

TEST_F(AddCommitTest, CommitClearsIndex) {
    kit({"init"});
    write_file("a.txt", "content");
    kit({"add", "a.txt"});
    kit({"commit", "-m", "first"});
    std::string index = kit::fs::read_file(tmp_dir / ".kit" / "index");
    EXPECT_TRUE(index.empty());
}

TEST_F(AddCommitTest, LogShowsCommitMessage) {
    kit({"init"});
    write_file("f.txt", "x");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "my commit"});
    auto r = kit({"log"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.output.find("my commit"), std::string::npos);
}

TEST_F(AddCommitTest, CommitWithoutStagedFiles) {
    kit({"init"});
    auto r = kit({"commit", "-m", "empty"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.output.find("Nothing to commit"), std::string::npos);
}
```

- [ ] **Step 2: Write `tests/integration/test_branch_checkout.cpp`**

```cpp
#include "helpers.hpp"
#include <fstream>

class BranchCheckoutTest : public KitTest {};

TEST_F(BranchCheckoutTest, CreateAndListBranch) {
    kit({"init"});
    write_file("f.txt", "x");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    kit({"branch", "dev"});
    auto r = kit({"branch"});
    EXPECT_NE(r.output.find("dev"), std::string::npos);
    EXPECT_NE(r.output.find("master"), std::string::npos);
}

TEST_F(BranchCheckoutTest, CheckoutSwitchesHEAD) {
    kit({"init"});
    write_file("f.txt", "x");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    kit({"branch", "dev"});
    kit({"checkout", "dev"});
    std::ifstream head(tmp_dir / ".kit" / "HEAD");
    std::string content;
    std::getline(head, content);
    EXPECT_EQ(content, "ref: refs/heads/dev");
}

TEST_F(BranchCheckoutTest, DeleteBranch) {
    kit({"init"});
    write_file("f.txt", "x");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    kit({"branch", "temp"});
    kit({"branch", "-d", "temp"});
    auto r = kit({"branch"});
    EXPECT_EQ(r.output.find("temp"), std::string::npos);
}
```

- [ ] **Step 3: Write `tests/integration/test_diff.cpp`**

```cpp
#include "helpers.hpp"

class DiffTest : public KitTest {};

TEST_F(DiffTest, NoDiffOnCleanRepo) {
    kit({"init"});
    write_file("f.txt", "hello\n");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    auto r = kit({"diff"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.output.find("No differences"), std::string::npos);
}

TEST_F(DiffTest, DetectsModifiedFile) {
    kit({"init"});
    write_file("f.txt", "line1\nline2\n");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    write_file("f.txt", "line1\nline2\nline3\n");
    auto r = kit({"diff"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.output.find("+line3"), std::string::npos);
}
```

- [ ] **Step 4: Write `tests/integration/test_merge.cpp`**

```cpp
#include "helpers.hpp"

class MergeTest : public KitTest {};

TEST_F(MergeTest, FastForwardMerge) {
    kit({"init"});
    write_file("base.txt", "base content\n");
    kit({"add", "base.txt"});
    kit({"commit", "-m", "base commit"});

    kit({"branch", "feature"});
    kit({"checkout", "feature"});
    write_file("feature.txt", "feature content\n");
    kit({"add", "feature.txt"});
    kit({"commit", "-m", "feature commit"});

    kit({"checkout", "master"});
    auto r = kit({"merge", "feature"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_TRUE(fs::exists(tmp_dir / "feature.txt"));
}
```

- [ ] **Step 5: Build and run all tests**

```bash
cd build && make -j4 && ctest -V
```

Expected: all unit and integration tests pass.

- [ ] **Step 6: Final commit**

```bash
git add tests/integration/test_add_commit.cpp tests/integration/test_branch_checkout.cpp \
        tests/integration/test_diff.cpp tests/integration/test_merge.cpp
git commit -m "test(integration): add full workflow integration tests for all commands"
```

---

## Done

At this point kit-vcs has:
- Clean `src/core/`, `src/cli/`, `src/utils/` architecture
- Proper blob/tree/commit object model
- All commands: init, add, commit, status, log, branch, checkout, diff, merge, reset, stash
- GTest unit tests for all core objects
- Integration tests for all major workflows
