#include "helpers.hpp"
#include <fstream>

TEST_F(KitTest, InitCreatesKitDirectory) {
    auto r = kit({"init"});
    EXPECT_EQ(r.exit_code, 0) << r.output;
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "HEAD"));
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "objects"));
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "refs" / "heads"));
    EXPECT_TRUE(fs::exists(tmp_dir / ".kit" / "index"));
}

TEST_F(KitTest, InitTwiceFails) {
    kit({"init"});
    auto r = kit({"init"});
    EXPECT_NE(r.exit_code, 0) << r.output;
}

TEST_F(KitTest, HeadPointsToMaster) {
    kit({"init"});
    std::ifstream head(tmp_dir / ".kit" / "HEAD");
    std::string content;
    std::getline(head, content);
    EXPECT_EQ(content, "ref: refs/heads/master");
}
