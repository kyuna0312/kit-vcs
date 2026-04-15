#include "helpers.hpp"
#include <filesystem>

class MergeTest : public KitTest {};

TEST_F(MergeTest, MergeNewFileFromBranch) {
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
    EXPECT_EQ(r.exit_code, 0) << r.output;
    EXPECT_TRUE(fs::exists(tmp_dir / "feature.txt"));
}

TEST_F(MergeTest, MergeSelfIsNoOp) {
    kit({"init"});
    write_file("f.txt", "x");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    kit({"branch", "same"});
    // checkout same and merge master (pointing to same commit)
    kit({"checkout", "same"});
    auto r = kit({"merge", "master"});
    EXPECT_EQ(r.exit_code, 0) << r.output;
    EXPECT_NE(r.output.find("up to date"), std::string::npos);
}
