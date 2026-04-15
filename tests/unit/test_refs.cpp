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
