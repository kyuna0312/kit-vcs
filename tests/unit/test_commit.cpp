#include <gtest/gtest.h>
#include "core/objects/commit.hpp"

TEST(CommitTest, SerializeRoundTrip) {
    kit::Commit c;
    c.tree_hash   = "treehash123";
    c.parent_hash = "parenthash456";
    c.author      = "Alice";
    c.timestamp   = 1700000000;
    c.message     = "Initial commit";

    auto restored = kit::Commit::deserialize(c.serialize());
    EXPECT_EQ(restored.tree_hash,   "treehash123");
    EXPECT_EQ(restored.parent_hash, "parenthash456");
    EXPECT_EQ(restored.author,      "Alice");
    EXPECT_EQ(restored.timestamp,   1700000000);
    EXPECT_EQ(restored.message,     "Initial commit");
}

TEST(CommitTest, NoParent) {
    kit::Commit c;
    c.tree_hash = "treehash";
    c.message   = "root";
    auto restored = kit::Commit::deserialize(c.serialize());
    EXPECT_TRUE(restored.parent_hash.empty());
}

TEST(CommitTest, HashConsistent) {
    kit::Commit c;
    c.tree_hash = "abc"; c.message = "msg";
    EXPECT_EQ(c.hash(), c.hash());
    EXPECT_EQ(c.hash().size(), 40u);
}
