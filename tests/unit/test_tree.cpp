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
