#pragma once
#include <gtest/gtest.h>
#include <filesystem>
#include <array>
#include <chrono>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <vector>

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
    for (const auto& a : args) cmd += " \"" + a + "\"";
    cmd += " 2>&1";

    RunResult r;
    std::array<char, 512> buf;
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) throw std::runtime_error("popen failed: " + cmd);
    while (fgets(buf.data(), (int)buf.size(), p))
        out += buf.data();
    int status = pclose(p);
    r.exit_code = WEXITSTATUS(status);
    r.output = out;
    return r;
}

class KitTest : public ::testing::Test {
protected:
    fs::path tmp_dir;
    fs::path orig_dir;

    void SetUp() override {
        orig_dir = fs::current_path();
        auto ns  = std::chrono::system_clock::now().time_since_epoch().count();
        tmp_dir  = fs::temp_directory_path() / ("kit_itest_" + std::to_string(ns));
        fs::create_directories(tmp_dir);
        fs::current_path(tmp_dir);
    }

    void TearDown() override {
        fs::current_path(orig_dir);
        fs::remove_all(tmp_dir);
    }

    RunResult kit(const std::vector<std::string>& args) { return run_kit(args); }

    void write_file(const std::string& name, const std::string& content) {
        std::ofstream f(tmp_dir / name);
        f << content;
    }

    std::string read_file(const std::string& name) {
        std::ifstream f(tmp_dir / name);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
};
