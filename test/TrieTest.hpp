#pragma once
#include "Test.hpp"
#include "gtest/gtest.h"

#include <trie.h>
#include <utils.h>

namespace
{

class Trie : public testing::Test
{
protected:
    void SetUp() override {
        root = NULL;
    }

    void TearDown() override {
        crawler_trie_destroy(&root);
    }

    CrawlerTrieNode* root = NULL;
};

TEST_F(Trie, InsertQuery) {
    ASSERT_TRUE(crawler_trie_insert(&root, "abcd", 4, 1, 2));
    CrawlerCharacterReference output;
    ASSERT_TRUE(crawler_trie_query(root, "abcd", 4, &output));
    ASSERT_EQ(output.first, 1);
    ASSERT_EQ(output.second, 2);
}

TEST_F(Trie, InsertQueryChain) {
    ASSERT_TRUE(crawler_trie_insert(&root, "abcd", 4, 1, 2));
    ASSERT_TRUE(crawler_trie_insert(&root, "abce", 4, 3, 4));
    ASSERT_TRUE(crawler_trie_insert(&root, "abcf", 4, 5, 6));

    CrawlerCharacterReference output;
    ASSERT_TRUE(crawler_trie_query(root, "abcd", 4, &output));
    ASSERT_EQ(output.first, 1);
    ASSERT_EQ(output.second, 2);

    ASSERT_TRUE(crawler_trie_query(root, "abce", 4, &output));
    ASSERT_EQ(output.first, 3);
    ASSERT_EQ(output.second, 4);

    ASSERT_TRUE(crawler_trie_query(root, "abcf", 4, &output));
    ASSERT_EQ(output.first, 5);
    ASSERT_EQ(output.second, 6);
}

TEST_F(Trie, Static) {
    ASSERT_TRUE(crawler_trie_insert(&root, "abcd", 4, 1, 2));
    ASSERT_TRUE(crawler_trie_insert(&root, "abce", 4, 3, 4));
    ASSERT_TRUE(crawler_trie_insert(&root, "abcf", 4, 5, 6));

    auto path = Crawler::Test::GenerateUniquePath();
    ASSERT_TRUE(crawler_trie_bft_serialize(path.c_str(), root));

    CrawlerStaticTrie st;
    ASSERT_TRUE(crawler_static_trie_deserialize(path.c_str(), &st));


    CrawlerCharacterReference output;
    ASSERT_TRUE(crawler_static_trie_query(&st, "abcd", 4, &output));
    ASSERT_EQ(output.first, 1);
    ASSERT_EQ(output.second, 2);

    ASSERT_TRUE(crawler_static_trie_query(&st, "abce", 4, &output));
    ASSERT_EQ(output.first, 3);
    ASSERT_EQ(output.second, 4);

    ASSERT_TRUE(crawler_static_trie_query(&st, "abcf", 4, &output));
    ASSERT_EQ(output.first, 5);
    ASSERT_EQ(output.second, 6);

    ASSERT_FALSE(crawler_static_trie_query(&st, "abcg", 4, &output));
    ASSERT_FALSE(crawler_static_trie_query(&st, "abcde", 5, &output));

    _crawler_free(st.data);
}

}