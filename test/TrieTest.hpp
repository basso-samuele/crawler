#pragma once
#include "Test.hpp"

#include <trie.h>
#include <utils.h>

namespace Trie
{

void InsertAndQuery() {
    CrawlerTrieNode* root = NULL;
    crawler_trie_node_init(root);
    crawler_trie_insert(&root, "abcd", 4, 1, 2);
    CrawlerCharacterReference output;
    CRAWLER_ASSERT_TRUE(crawler_trie_query(root, "abcd", 4, &output));
    CRAWLER_ASSERT_EQ(output.first, 1);
    CRAWLER_ASSERT_EQ(output.second, 2);    
    crawler_trie_destroy(&root);
}

void CommonNodes() {
    CrawlerTrieNode* root = NULL;
    crawler_trie_node_init(root);
    crawler_trie_insert(&root, "abcd", 4, 1, 2);
    crawler_trie_insert(&root, "abce", 4, 3, 4);
    crawler_trie_insert(&root, "abcf", 4, 5, 6);
    CrawlerCharacterReference output;
    CRAWLER_ASSERT_TRUE(crawler_trie_query(root, "abce", 4, &output));
    CRAWLER_ASSERT_EQ(output.first, 3);
    CRAWLER_ASSERT_EQ(output.second, 4);

    CRAWLER_ASSERT_TRUE(crawler_trie_bft_serialize("test_ser", root));
    CrawlerStaticTrie t;
    CRAWLER_ASSERT_TRUE(crawler_static_trie_deserialize("test_ser", &t));

    std::cout << "Deserialized " << t.node_count << " nodes." << std::endl;
    for (size_t i = 0; i < t.node_count; i++) {
        if (t.data[i].is_terminal) std::cout << "Terminal ";
        else std::cout << "Non terminal ";
        std::cout << "node at offset: " << i << std::endl;
        std::cout << "\tCharacter reference: " << t.data[i].char_ref.first << " " << t.data[i].char_ref.second << std::endl;
        std::cout << "\tWith children." << std::endl;
        for (size_t j = 0; j < CRAWLER_KEY_SPACE_SIZE; j++) {
            if (t.data[i].children_offsets[j] != 0)
                std::cout << "\t\t" << t.data[i].children_offsets[j] << std::endl;
        }
    }

    CRAWLER_ASSERT_TRUE(crawler_static_trie_query(&t, "abce", 4, &output));
    CRAWLER_ASSERT_EQ(output.first, 3);
    CRAWLER_ASSERT_EQ(output.second, 4);
    CRAWLER_ASSERT_TRUE(crawler_static_trie_query(&t, "abcd", 4, &output));
    CRAWLER_ASSERT_EQ(output.first, 1);
    CRAWLER_ASSERT_EQ(output.second, 2);
    CRAWLER_ASSERT_TRUE(crawler_static_trie_query(&t, "abcf", 4, &output));
    CRAWLER_ASSERT_EQ(output.first, 5);
    CRAWLER_ASSERT_EQ(output.second, 6);
    CRAWLER_ASSERT_FALSE(crawler_static_trie_query(&t, "abcg", 4, &output));
    CRAWLER_ASSERT_FALSE(crawler_static_trie_query(&t, "abcde", 5, &output));

    _crawler_free(t.data);
    crawler_trie_destroy(&root);
}

void Test() {
    InsertAndQuery();
    CommonNodes();
}

}