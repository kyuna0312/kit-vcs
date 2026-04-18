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
