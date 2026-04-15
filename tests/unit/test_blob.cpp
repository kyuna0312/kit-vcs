#include <gtest/gtest.h>
#include "core/objects/blob.hpp"

TEST(BlobTest, SerializeFormat) {
    kit::Blob b{"hello"};
    EXPECT_EQ(b.serialize(), "blob 5\nhello");
}

TEST(BlobTest, HashIsConsistent) {
    kit::Blob b{"hello"};
    EXPECT_EQ(b.hash(), b.hash());
    EXPECT_EQ(b.hash().size(), 40u);
}

TEST(BlobTest, RoundTrip) {
    kit::Blob original{"some content\nwith newlines\n"};
    auto restored = kit::Blob::deserialize(original.serialize());
    EXPECT_EQ(restored.content, original.content);
}

TEST(BlobTest, EmptyContent) {
    kit::Blob b{""};
    EXPECT_EQ(b.serialize(), "blob 0\n");
    auto restored = kit::Blob::deserialize(b.serialize());
    EXPECT_EQ(restored.content, "");
}
