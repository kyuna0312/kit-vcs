#include "helpers.hpp"

class DiffTest : public KitTest {};

TEST_F(DiffTest, NoDiffOnCleanRepo) {
    kit({"init"});
    write_file("f.txt", "hello\n");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    auto r = kit({"diff"});
    EXPECT_EQ(r.exit_code, 0) << r.output;
    EXPECT_NE(r.output.find("No differences"), std::string::npos);
}

TEST_F(DiffTest, DetectsModifiedFile) {
    kit({"init"});
    write_file("f.txt", "line1\nline2\n");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    write_file("f.txt", "line1\nline2\nline3\n");
    auto r = kit({"diff"});
    EXPECT_EQ(r.exit_code, 0) << r.output;
    EXPECT_NE(r.output.find("+line3"), std::string::npos);
}

TEST_F(DiffTest, ShowsRemovedLine) {
    kit({"init"});
    write_file("f.txt", "line1\nline2\n");
    kit({"add", "f.txt"});
    kit({"commit", "-m", "init"});
    write_file("f.txt", "line1\n");
    auto r = kit({"diff"});
    EXPECT_NE(r.output.find("-line2"), std::string::npos);
}
