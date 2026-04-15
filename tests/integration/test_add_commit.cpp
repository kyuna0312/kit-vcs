#include "helpers.hpp"
#include <fstream>
#include <sstream>

class AddCommitTest : public KitTest {};

TEST_F(AddCommitTest, AddStagedFileAppearsInIndex) {
    kit({"init"});
    write_file("hello.txt", "hello\n");
    auto r = kit({"add", "hello.txt"});
    EXPECT_EQ(r.exit_code, 0) << r.output;

    std::ifstream idx(tmp_dir / ".kit" / "index");
    std::ostringstream ss;
    ss << idx.rdbuf();
    EXPECT_NE(ss.str().find("hello.txt"), std::string::npos);
}

TEST_F(AddCommitTest, CommitClearsIndex) {
    kit({"init"});
    write_file("a.txt", "content");
    kit({"add", "a.txt"});
    kit({"commit", "-m", "first"});

    std::ifstream idx(tmp_dir / ".kit" / "index");
    std::ostringstream ss;
    ss << idx.rdbuf();
    EXPECT_TRUE(ss.str().empty());
}

TEST_F(AddCommitTest, LogShowsCommitMessage) {
    kit({"init"});
    write_file("f.txt", "x");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "my commit"});
    auto r = kit({"log"});
    EXPECT_EQ(r.exit_code, 0) << r.output;
    EXPECT_NE(r.output.find("my commit"), std::string::npos);
}

TEST_F(AddCommitTest, CommitWithoutStagedFiles) {
    kit({"init"});
    auto r = kit({"commit", "-m", "empty"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.output.find("Nothing to commit"), std::string::npos);
}

TEST_F(AddCommitTest, StatusShowsCleanAfterCommit) {
    kit({"init"});
    write_file("f.txt", "hello");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    auto r = kit({"status"});
    EXPECT_NE(r.output.find("clean"), std::string::npos);
}
