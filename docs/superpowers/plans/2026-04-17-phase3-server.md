# Phase 3: Server (REST API + TCP Daemon) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `kit-server` (HTTP REST API + static Vue SPA serving) and `kit-daemon` (binary TCP remote protocol) as two C++ binaries that link `kit_core`.

**Architecture:** `server/` directory with its own `CMakeLists.txt`. `kit-server` uses cpp-httplib (header-only). `kit-daemon` uses POSIX sockets (Linux/macOS) / Winsock (Windows). Both binaries link `kit_core` from `cpp/`. Bearer token auth for REST, pubkey for daemon.

**Tech Stack:** C++17, cpp-httplib 0.14.x (header-only, fetched via CMake), OpenSSL (from kit_core), nlohmann/json 3.11.x (header-only), POSIX sockets / Winsock2

**Prerequisite:** Phase 0 complete — `cpp/` layout must exist with `kit_core` static lib.

---

## File Map

### Created
```
server/CMakeLists.txt
server/src/rest/server.hpp
server/src/rest/server.cpp
server/src/rest/router.hpp
server/src/rest/router.cpp
server/src/rest/handlers/repos.cpp
server/src/rest/handlers/commits.cpp
server/src/rest/handlers/branches.cpp
server/src/rest/handlers/pulls.cpp
server/src/rest/handlers/issues.cpp
server/src/rest/handlers/static_files.cpp
server/src/auth/tokens.hpp
server/src/auth/tokens.cpp
server/src/daemon/tcp_server.hpp
server/src/daemon/tcp_server.cpp
server/src/daemon/protocol.hpp
server/src/daemon/protocol.cpp
server/src/daemon/handlers/push.cpp
server/src/daemon/handlers/fetch.cpp
server/src/daemon/handlers/clone.cpp
server/src/kit_server_main.cpp
server/src/kit_daemon_main.cpp
server/data/repos/                ← runtime repo storage root
server/tests/test_rest_api.cpp
server/tests/test_protocol.cpp
```

### Modified
```
CMakeLists.txt   ← uncomment add_subdirectory(server)
```

---

### Task 1: CMake setup for server/

**Files:**
- Create: `server/CMakeLists.txt`
- Modify: `CMakeLists.txt` (root)

- [ ] **Step 1: Create server/CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.14)
project(kit-server-project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch cpp-httplib
include(FetchContent)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(httplib nlohmann_json)

find_package(OpenSSL REQUIRED)

# kit_core comes from the cpp/ subdirectory
# When built from root CMakeLists, kit_core target already exists.

set(REST_SOURCES
    src/rest/server.cpp
    src/rest/router.cpp
    src/rest/handlers/repos.cpp
    src/rest/handlers/commits.cpp
    src/rest/handlers/branches.cpp
    src/rest/handlers/pulls.cpp
    src/rest/handlers/issues.cpp
    src/rest/handlers/static_files.cpp
    src/auth/tokens.cpp
)

set(DAEMON_SOURCES
    src/daemon/tcp_server.cpp
    src/daemon/protocol.cpp
    src/daemon/handlers/push.cpp
    src/daemon/handlers/fetch.cpp
    src/daemon/handlers/clone.cpp
)

add_executable(kit-server ${REST_SOURCES} src/kit_server_main.cpp)
target_include_directories(kit-server PRIVATE src ${CMAKE_SOURCE_DIR}/cpp/src)
target_link_libraries(kit-server PRIVATE kit_core httplib::httplib nlohmann_json::nlohmann_json OpenSSL::Crypto)

add_executable(kit-daemon ${DAEMON_SOURCES} src/kit_daemon_main.cpp)
target_include_directories(kit-daemon PRIVATE src ${CMAKE_SOURCE_DIR}/cpp/src)
target_link_libraries(kit-daemon PRIVATE kit_core nlohmann_json::nlohmann_json OpenSSL::Crypto)

# Tests
include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip)
FetchContent_MakeAvailable(googletest)

add_executable(test_server tests/test_rest_api.cpp tests/test_protocol.cpp ${REST_SOURCES} ${DAEMON_SOURCES})
target_include_directories(test_server PRIVATE src ${CMAKE_SOURCE_DIR}/cpp/src)
target_link_libraries(test_server PRIVATE kit_core httplib::httplib nlohmann_json::nlohmann_json OpenSSL::Crypto gtest gtest_main)
add_test(NAME server COMMAND test_server)
```

- [ ] **Step 2: Uncomment server in root CMakeLists.txt**

Change line in `CMakeLists.txt`:
```cmake
# add_subdirectory(server)  # uncomment in Phase 3
```
To:
```cmake
add_subdirectory(server)
```

- [ ] **Step 3: Create minimal main files to verify build**

`server/src/kit_server_main.cpp`:
```cpp
#include <iostream>
int main() { std::cout << "kit-server v2.0.0\n"; return 0; }
```

`server/src/kit_daemon_main.cpp`:
```cpp
#include <iostream>
int main() { std::cout << "kit-daemon v2.0.0\n"; return 0; }
```

- [ ] **Step 4: Verify root build**

```bash
mkdir -p build && cd build
cmake ..
make kit-server kit-daemon -j$(nproc)
```

Expected: both binaries build.

- [ ] **Step 5: Commit**

```bash
git add server/ CMakeLists.txt
git commit -m "feat(server): add CMake setup for kit-server and kit-daemon"
```

---

### Task 2: Auth — bearer tokens

**Files:**
- Create: `server/src/auth/tokens.hpp`
- Create: `server/src/auth/tokens.cpp`

- [ ] **Step 1: Write failing test**

`server/tests/test_rest_api.cpp` (auth section):

```cpp
#include <gtest/gtest.h>
#include "auth/tokens.hpp"

TEST(Auth, GenerateAndValidateToken) {
    auth::TokenStore store;
    std::string token = store.generate("alice");
    EXPECT_EQ(token.size(), 64u);  // 32 random bytes hex-encoded
    EXPECT_EQ(store.validate(token), "alice");
}

TEST(Auth, InvalidTokenReturnsEmpty) {
    auth::TokenStore store;
    EXPECT_EQ(store.validate("bad_token"), "");
}
```

- [ ] **Step 2: Run — expect fail**

```bash
cd build && make test_server 2>&1 | head -20
```

- [ ] **Step 3: Implement TokenStore**

`server/src/auth/tokens.hpp`:
```cpp
#pragma once
#include <string>
#include <unordered_map>

namespace auth {

class TokenStore {
public:
    std::string generate(const std::string& user);
    std::string validate(const std::string& token) const;  // returns username or ""
    void        revoke(const std::string& token);

private:
    std::unordered_map<std::string, std::string> tokens_;  // token → username
};

} // namespace auth
```

`server/src/auth/tokens.cpp`:
```cpp
#include "auth/tokens.hpp"
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

namespace auth {

static std::string random_hex(size_t bytes) {
    std::vector<unsigned char> buf(bytes);
    RAND_bytes(buf.data(), static_cast<int>(bytes));
    std::ostringstream oss;
    for (auto b : buf) oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}

std::string TokenStore::generate(const std::string& user) {
    std::string token = random_hex(32);
    tokens_[token] = user;
    return token;
}

std::string TokenStore::validate(const std::string& token) const {
    auto it = tokens_.find(token);
    return it != tokens_.end() ? it->second : "";
}

void TokenStore::revoke(const std::string& token) {
    tokens_.erase(token);
}

} // namespace auth
```

- [ ] **Step 4: Run — expect pass**

```bash
cd build && make test_server && ./test_server --gtest_filter=Auth.*
```

- [ ] **Step 5: Commit**

```bash
git add server/src/auth/ server/tests/
git commit -m "feat(server): add bearer token auth"
```

---

### Task 3: REST server skeleton + JSON responses

**Files:**
- Create: `server/src/rest/server.hpp`
- Create: `server/src/rest/server.cpp`
- Create: `server/src/rest/router.hpp`
- Create: `server/src/rest/router.cpp`

- [ ] **Step 1: Implement RestServer**

`server/src/rest/server.hpp`:
```cpp
#pragma once
#include <string>
#include <filesystem>

namespace rest {

class RestServer {
public:
    RestServer(const std::filesystem::path& repos_root, int port = 8080);
    void start();   // blocks
    void stop();

private:
    std::filesystem::path repos_root_;
    int port_;
};

} // namespace rest
```

`server/src/rest/server.cpp`:
```cpp
#include "rest/server.hpp"
#include "rest/router.hpp"
#include <httplib.h>
#include <iostream>

namespace rest {

RestServer::RestServer(const std::filesystem::path& repos_root, int port)
    : repos_root_(repos_root), port_(port) {}

void RestServer::start() {
    httplib::Server svr;
    rest::register_routes(svr, repos_root_);
    std::cout << "kit-server listening on :" << port_ << "\n";
    svr.listen("0.0.0.0", port_);
}

} // namespace rest
```

- [ ] **Step 2: Implement router with JSON helper**

`server/src/rest/router.hpp`:
```cpp
#pragma once
#include <httplib.h>
#include <filesystem>

namespace rest {
void register_routes(httplib::Server& svr, const std::filesystem::path& repos_root);
} // namespace rest
```

`server/src/rest/router.cpp`:
```cpp
#include "rest/router.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void json_response(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

namespace rest {

void register_routes(httplib::Server& svr, const std::filesystem::path& repos_root) {
    // Health check
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        json_response(res, 200, {{"status", "ok"}});
    });

    // Repo routes — implemented in handlers/repos.cpp
    extern void register_repo_routes(httplib::Server&, const std::filesystem::path&);
    register_repo_routes(svr, repos_root);

    // Commits, branches, pulls, issues
    extern void register_commit_routes(httplib::Server&, const std::filesystem::path&);
    extern void register_branch_routes(httplib::Server&, const std::filesystem::path&);
    extern void register_pull_routes(httplib::Server&, const std::filesystem::path&);
    extern void register_issue_routes(httplib::Server&, const std::filesystem::path&);
    register_commit_routes(svr, repos_root);
    register_branch_routes(svr, repos_root);
    register_pull_routes(svr, repos_root);
    register_issue_routes(svr, repos_root);
}

} // namespace rest
```

- [ ] **Step 3: Commit**

```bash
git add server/src/rest/
git commit -m "feat(server): add REST server skeleton with routing"
```

---

### Task 4: REST handlers — repos + commits

**Files:**
- Create: `server/src/rest/handlers/repos.cpp`
- Create: `server/src/rest/handlers/commits.cpp`

- [ ] **Step 1: Write REST handler test**

```cpp
// server/tests/test_rest_api.cpp — add to existing file
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "rest/router.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <filesystem>

using json = nlohmann::json;

class RestApiTest : public ::testing::Test {
protected:
    std::filesystem::path repos_root;
    httplib::Server svr;
    std::thread server_thread;

    void SetUp() override {
        repos_root = std::filesystem::temp_directory_path() / "kit-server-test";
        std::filesystem::create_directories(repos_root);
        rest::register_routes(svr, repos_root);
        server_thread = std::thread([this]{ svr.listen("127.0.0.1", 18080); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void TearDown() override {
        svr.stop();
        server_thread.join();
        std::filesystem::remove_all(repos_root);
    }
};

TEST_F(RestApiTest, HealthCheckReturnsOk) {
    httplib::Client cli("127.0.0.1", 18080);
    auto res = cli.Get("/health");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["status"], "ok");
}

TEST_F(RestApiTest, CreateAndGetRepo) {
    httplib::Client cli("127.0.0.1", 18080);
    json payload = {{"name", "myrepo"}};
    auto res = cli.Post("/repos", payload.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 201);

    auto get_res = cli.Get("/repos/myrepo");
    ASSERT_TRUE(get_res);
    EXPECT_EQ(get_res->status, 200);
    auto body = json::parse(get_res->body);
    EXPECT_EQ(body["name"], "myrepo");
}
```

- [ ] **Step 2: Implement repos handler**

`server/src/rest/handlers/repos.cpp`:
```cpp
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "core/repository.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

static void json_response(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void register_repo_routes(httplib::Server& svr, const fs::path& repos_root) {
    // POST /repos — create repo
    svr.Post("/repos", [repos_root](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("name")) {
            json_response(res, 400, {{"error", "name required"}});
            return;
        }
        std::string name = body["name"];
        fs::path repo_path = repos_root / name;
        if (fs::exists(repo_path)) {
            json_response(res, 409, {{"error", "repo already exists"}});
            return;
        }
        fs::create_directories(repo_path);
        auto result = kit::Repository::init(repo_path);
        if (!result) {
            json_response(res, 500, {{"error", "init failed"}});
            return;
        }
        json_response(res, 201, {{"name", name}, {"path", repo_path.string()}});
    });

    // GET /repos/:name — repo info
    svr.Get(R"(/repos/([^/]+))", [repos_root](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        fs::path repo_path = repos_root / name;
        if (!fs::exists(repo_path)) {
            json_response(res, 404, {{"error", "repo not found"}});
            return;
        }
        json_response(res, 200, {{"name", name}});
    });

    // GET /repos — list all repos
    svr.Get("/repos", [repos_root](const httplib::Request&, httplib::Response& res) {
        json repos = json::array();
        if (fs::exists(repos_root)) {
            for (const auto& entry : fs::directory_iterator(repos_root)) {
                if (entry.is_directory()) {
                    repos.push_back(entry.path().filename().string());
                }
            }
        }
        json_response(res, 200, {{"repos", repos}});
    });
}
```

- [ ] **Step 3: Implement commits handler**

`server/src/rest/handlers/commits.cpp`:
```cpp
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "core/repository.hpp"
#include "core/objects/commit.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

static void json_response(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void register_commit_routes(httplib::Server& svr, const fs::path& repos_root) {
    svr.Get(R"(/repos/([^/]+)/commits)", [repos_root](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        fs::path repo_path = repos_root / name;
        if (!fs::exists(repo_path)) {
            json_response(res, 404, {{"error", "repo not found"}});
            return;
        }
        kit::Repository repo(repo_path);
        auto head = repo.refs().head_commit();
        json commits = json::array();
        std::string current = head;
        while (!current.empty()) {
            auto raw = repo.read_object(current);
            if (!raw) break;
            auto commit = kit::Commit::deserialize(*raw);
            commits.push_back({
                {"hash", current},
                {"author", commit.author},
                {"message", commit.message},
                {"timestamp", commit.timestamp}
            });
            current = commit.parent;
        }
        json_response(res, 200, {{"commits", commits}});
    });
}
```

- [ ] **Step 4: Stub branches, pulls, issues handlers**

For each of `branches.cpp`, `pulls.cpp`, `issues.cpp`:
```cpp
#include <httplib.h>
void register_branch_routes(httplib::Server&, const std::filesystem::path&) {}
void register_pull_routes(httplib::Server&, const std::filesystem::path&) {}
void register_issue_routes(httplib::Server&, const std::filesystem::path&) {}
```

- [ ] **Step 5: Implement static file serving**

`server/src/rest/handlers/static_files.cpp`:
```cpp
// Serves built Vue SPA from web/dist/ when it exists.
// Falls back to JSON 404 if not built yet.
#include <httplib.h>
#include <filesystem>

void register_static_routes(httplib::Server& svr) {
    namespace fs = std::filesystem;
    fs::path dist = fs::path(__FILE__).parent_path().parent_path().parent_path().parent_path() / "web" / "dist";
    if (fs::exists(dist)) {
        svr.set_mount_point("/", dist.string());
    }
}
```

- [ ] **Step 6: Run tests**

```bash
cd build && make test_server && ./test_server
```

Expected: auth + REST API tests PASS.

- [ ] **Step 7: Wire up main and smoke test**

`server/src/kit_server_main.cpp`:
```cpp
#include "rest/server.hpp"
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 8080;
    std::filesystem::path repos_root = "server/data/repos";
    std::filesystem::create_directories(repos_root);
    if (argc > 1) port = std::stoi(argv[1]);
    rest::RestServer server(repos_root, port);
    server.start();
    return 0;
}
```

```bash
cd build && ./kit-server &
curl http://localhost:8080/health
# expected: {"status":"ok"}
curl -X POST http://localhost:8080/repos -d '{"name":"test"}' -H "Content-Type: application/json"
# expected: {"name":"test",...}
kill %1
```

- [ ] **Step 8: Commit**

```bash
git add server/src/ server/tests/
git commit -m "feat(server): implement REST API repos + commits endpoints"
```

---

### Task 5: Binary TCP protocol (kit-daemon)

**Files:**
- Create: `server/src/daemon/protocol.hpp`
- Create: `server/src/daemon/protocol.cpp`
- Create: `server/src/daemon/tcp_server.hpp`
- Create: `server/src/daemon/tcp_server.cpp`
- Create: `server/src/daemon/handlers/clone.cpp`
- Create: `server/src/daemon/handlers/fetch.cpp`
- Create: `server/src/daemon/handlers/push.cpp`

- [ ] **Step 1: Write protocol test**

```cpp
// server/tests/test_protocol.cpp
#include <gtest/gtest.h>
#include "daemon/protocol.hpp"

TEST(Protocol, EncodeDecodePacket) {
    daemon::Packet pkt;
    pkt.command = daemon::CMD_CLONE;
    pkt.payload = {'m', 'y', 'r', 'e', 'p', 'o'};
    auto encoded = daemon::encode_packet(pkt);
    // [4 bytes length][1 byte command][payload]
    EXPECT_EQ(encoded.size(), 4 + 1 + 6);
    EXPECT_EQ(encoded[4], daemon::CMD_CLONE);

    auto decoded = daemon::decode_packet(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->command, daemon::CMD_CLONE);
    EXPECT_EQ(decoded->payload, pkt.payload);
}
```

- [ ] **Step 2: Run — expect fail**

```bash
cd build && make test_server 2>&1 | grep "error:" | head -10
```

- [ ] **Step 3: Implement protocol**

`server/src/daemon/protocol.hpp`:
```cpp
#pragma once
#include <vector>
#include <cstdint>
#include <optional>

namespace daemon {

constexpr uint8_t CMD_CLONE = 0x01;
constexpr uint8_t CMD_FETCH = 0x02;
constexpr uint8_t CMD_PUSH  = 0x03;

struct Packet {
    uint8_t              command;
    std::vector<uint8_t> payload;
};

std::vector<uint8_t>   encode_packet(const Packet& pkt);
std::optional<Packet>  decode_packet(const std::vector<uint8_t>& data);

} // namespace daemon
```

`server/src/daemon/protocol.cpp`:
```cpp
#include "daemon/protocol.hpp"
#include <cstring>

namespace daemon {

std::vector<uint8_t> encode_packet(const Packet& pkt) {
    uint32_t len = static_cast<uint32_t>(pkt.payload.size());
    // network byte order (big-endian)
    std::vector<uint8_t> out(4 + 1 + len);
    out[0] = (len >> 24) & 0xFF;
    out[1] = (len >> 16) & 0xFF;
    out[2] = (len >>  8) & 0xFF;
    out[3] = (len      ) & 0xFF;
    out[4] = pkt.command;
    std::copy(pkt.payload.begin(), pkt.payload.end(), out.begin() + 5);
    return out;
}

std::optional<Packet> decode_packet(const std::vector<uint8_t>& data) {
    if (data.size() < 5) return std::nullopt;
    uint32_t len = (static_cast<uint32_t>(data[0]) << 24)
                 | (static_cast<uint32_t>(data[1]) << 16)
                 | (static_cast<uint32_t>(data[2]) <<  8)
                 | (static_cast<uint32_t>(data[3])      );
    if (data.size() < 5 + len) return std::nullopt;
    Packet pkt;
    pkt.command = data[4];
    pkt.payload.assign(data.begin() + 5, data.begin() + 5 + len);
    return pkt;
}

} // namespace daemon
```

- [ ] **Step 4: Run — expect pass**

```bash
cd build && make test_server && ./test_server --gtest_filter=Protocol.*
```

- [ ] **Step 5: Implement TCP server skeleton**

`server/src/daemon/tcp_server.hpp`:
```cpp
#pragma once
#include <filesystem>

namespace daemon {

class TcpServer {
public:
    TcpServer(const std::filesystem::path& repos_root, int port = 9418);
    void start();  // blocks, accepts connections
    void stop();

private:
    std::filesystem::path repos_root_;
    int port_;
    int server_fd_ = -1;
    void handle_client(int client_fd);
};

} // namespace daemon
```

`server/src/daemon/tcp_server.cpp`:
```cpp
#include "daemon/tcp_server.hpp"
#include "daemon/protocol.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <vector>

namespace daemon {

TcpServer::TcpServer(const std::filesystem::path& repos_root, int port)
    : repos_root_(repos_root), port_(port) {}

void TcpServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd_, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd_, 10);
    std::cout << "kit-daemon listening on :" << port_ << "\n";

    while (true) {
        int client = accept(server_fd_, nullptr, nullptr);
        if (client < 0) break;
        std::thread([this, client]{ handle_client(client); }).detach();
    }
}

void TcpServer::stop() {
    if (server_fd_ >= 0) { close(server_fd_); server_fd_ = -1; }
}

void TcpServer::handle_client(int client_fd) {
    // Read packet
    uint8_t header[5];
    if (recv(client_fd, header, 5, MSG_WAITALL) != 5) { close(client_fd); return; }

    uint32_t len = (header[0]<<24)|(header[1]<<16)|(header[2]<<8)|header[3];
    uint8_t  cmd = header[4];

    std::vector<uint8_t> payload(len);
    if (len > 0) recv(client_fd, payload.data(), len, MSG_WAITALL);

    std::string response = "ERR unknown command";
    std::string repo_name(payload.begin(), payload.end());

    extern std::string handle_clone(const std::filesystem::path&, const std::string&);
    extern std::string handle_fetch(const std::filesystem::path&, const std::string&);
    extern std::string handle_push(const std::filesystem::path&, const std::string&, const std::vector<uint8_t>&);

    if      (cmd == CMD_CLONE) response = handle_clone(repos_root_, repo_name);
    else if (cmd == CMD_FETCH) response = handle_fetch(repos_root_, repo_name);
    else if (cmd == CMD_PUSH)  response = handle_push(repos_root_, repo_name, payload);

    send(client_fd, response.data(), response.size(), 0);
    close(client_fd);
}

} // namespace daemon
```

- [ ] **Step 6: Stub daemon handlers**

`server/src/daemon/handlers/clone.cpp`:
```cpp
#include <filesystem>
#include <string>
std::string handle_clone(const std::filesystem::path& root, const std::string& name) {
    // TODO Phase 4: serialize all objects for repo `name`
    return "OK clone:" + name;
}
```

`server/src/daemon/handlers/fetch.cpp`:
```cpp
#include <filesystem>
#include <string>
std::string handle_fetch(const std::filesystem::path& root, const std::string& name) {
    return "OK fetch:" + name;
}
```

`server/src/daemon/handlers/push.cpp`:
```cpp
#include <filesystem>
#include <string>
#include <vector>
std::string handle_push(const std::filesystem::path& root, const std::string& name, const std::vector<uint8_t>& data) {
    return "OK push:" + name;
}
```

- [ ] **Step 7: Wire daemon main**

`server/src/kit_daemon_main.cpp`:
```cpp
#include "daemon/tcp_server.hpp"
#include <filesystem>

int main(int argc, char* argv[]) {
    int port = 9418;
    std::filesystem::path repos_root = "server/data/repos";
    std::filesystem::create_directories(repos_root);
    if (argc > 1) port = std::stoi(argv[1]);
    daemon::TcpServer server(repos_root, port);
    server.start();
    return 0;
}
```

- [ ] **Step 8: Build and verify**

```bash
cd build && make kit-daemon
./kit-daemon &
sleep 0.5
# Send a test packet
python3 -c "
import socket, struct
s = socket.socket()
s.connect(('127.0.0.1', 9418))
name = b'testrepo'
pkt = struct.pack('>IB', len(name), 0x01) + name
s.send(pkt)
print(s.recv(1024))
s.close()
"
kill %1
```

Expected: `b'OK clone:testrepo'`

- [ ] **Step 9: Commit**

```bash
git add server/src/daemon/ server/tests/test_protocol.cpp
git commit -m "feat(server): implement TCP daemon with binary protocol"
```
