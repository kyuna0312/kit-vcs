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

TEST_F(BranchCheckoutTest, CheckoutRestoresFile) {
    kit({"init"});
    write_file("hello.txt", "from master");
    kit({"add", "hello.txt"});
    kit({"commit", "-m", "master commit"});

    kit({"branch", "dev"});
    kit({"checkout", "dev"});

    // Read hello.txt - should have "from master" content
    std::ifstream f(tmp_dir / "hello.txt");
    std::string content;
    std::getline(f, content);
    EXPECT_EQ(content, "from master");
}
