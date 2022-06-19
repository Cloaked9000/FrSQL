//
// Created by fred on 01/08/2021.
//
/*
#include "TestUtils.h"
#include "BTree.h"

using tree_t = Tree<int, 5>;
using node_t = NodePtr<int, 5>;

tree_t::node_t *make(const std::vector<int>& num, const std::vector<Tree<int, 5>::node_t*> &children = {})
{
    auto n = new Tree<int, 5>::node_t();
    n->count = num.size();
    for(int a = 0; a < num.size(); a++)
    {
        n->list[a] = num[a];
    }

    for(int a = 0; a < children.size(); a++)
    {
        n->children[a] = children[a];
    }

    return n;
}
tree_t make_tree()
{
    auto nodes = make({60}, {
            make({10, 25}, {
                    make({1, 3, 5}),
                    make({15, 18, 20}),
                    make({30, 32})
            }),
            make({70, 90},
                 {make({62, 68}),
                  make({75, 80}),
                  make({92, 95})})
    });

    tree_t tree;
    tree.root = nodes;
    tree.height = 3;
    return tree;
}

bool assert_node(tree_t::node_t *node, const std::vector<int> &items)
{
    return items.size() == node->count && std::equal(std::begin(items), std::end(items), std::begin(node->list));
}

TEST(BTreeTest, test_erase_node_beginning)
{
    node_t actual(4, {1, 2, 3, 4}, {(node_t*)0x1, (node_t*)0x2, (node_t*)0x3, (node_t*)0x4, (node_t*)0x5});
    node_t expected(3, {2, 3, 4}, {(node_t*)0x1, (node_t*)0x3, (node_t*)0x4, (node_t*)0x5});
    actual.erase(0);
    ASSERT_EQ(actual, expected);
}

TEST(BTreeTest, test_erase_node_end)
{
    node_t actual(4, {1, 2, 3, 4}, {(node_t*)0x1, (node_t*)0x2, (node_t*)0x3, (node_t*)0x4, (node_t*)0x5});
    node_t expected(3, {1, 2, 3}, {(node_t*)0x1, (node_t*)0x2, (node_t*)0x3, (node_t*)0x4});
    actual.erase(3);
    ASSERT_EQ(actual, expected);

}

TEST(BTreeTest, test_erase_node_multi)
{
    node_t actual(4, {1, 2, 3, 4}, {(node_t*)0x1, (node_t*)0x2, (node_t*)0x3, (node_t*)0x4, (node_t*)0x5});
    node_t expected(2, {2, 3}, {(node_t*)0x1, (node_t*)0x3, (node_t*)0x4});
    actual.erase(0);
    actual.erase(2);
    ASSERT_EQ(actual, expected);
}

TEST(BTreeTest, test_erase_leaf_no_cascade)
{
    tree_t tree = make_tree();
    tree.erase(18);
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[1], {15, 20}));
}

TEST(BTreeTest, test_erase_left_shift)
{
    tree_t tree = make_tree();
    tree.erase(30);
    ASSERT_TRUE(assert_node(tree.root->children[0], {10, 20}));
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[0], {1, 3, 5}));
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[1], {15, 18}));
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[2], {25, 32}));
}

TEST(BTreeTest, test_erase_right_shift)
{
    tree_t tree = make_tree();
    tree.erase(3);
    tree.erase(5);
    ASSERT_TRUE(assert_node(tree.root->children[0], {15, 25}));
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[0], {1, 10}));
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[1], {18, 20}));
    ASSERT_TRUE(assert_node(tree.root->children[0]->children[2], {30, 32}));
}

TEST(BTreeTest, test_erase_left_and_right_merge)
{
    tree_t tree = make_tree();
    tree.erase(68);
    ASSERT_TRUE(assert_node(tree.root, {10, 25, 60, 90}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {1, 3, 5}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {15, 18, 20}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {30, 32}));
    ASSERT_TRUE(assert_node(tree.root->children[3], {62, 70, 75, 80}));
    ASSERT_TRUE(assert_node(tree.root->children[4], {92, 95}));
}

TEST(BTreeTest, test_erase_right_merge)
{
    tree_t tree = make_tree();
    tree.erase(70);
    ASSERT_TRUE(assert_node(tree.root, {10, 25, 60, 90}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {1, 3, 5}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {15, 18, 20}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {30, 32}));
    ASSERT_TRUE(assert_node(tree.root->children[3], {62, 68, 75, 80}));
    ASSERT_TRUE(assert_node(tree.root->children[4], {92, 95}));
}

TEST(BTreeTest, test_erase_shrink)
{
    tree_t tree = make_tree();
    ASSERT_EQ(tree.height, 3);

    tree.erase(70);
    ASSERT_EQ(tree.height, 2);

    std::vector<int> nums{1, 3, 5, 15, 18, 20, 30, 32, 62, 68, 75, 80, 92, 95};
    for(auto num : nums)
    {
        tree.erase(num);
    }
    ASSERT_EQ(tree.height, 1);

    nums = {10, 25, 60, 90};
    for(auto num : nums)
    {
        tree.erase(num);
    }

    ASSERT_EQ(tree.height, 0);
}

TEST(BTreeTest, test_erase_existant)
{
    tree_t tree = make_tree();
    ASSERT_TRUE(tree.erase(60));
}

TEST(BTreeTest, test_erase_none_existant)
{
    tree_t tree = make_tree();
    ASSERT_FALSE(tree.erase(1000));
}

TEST(BTreeTest, test_insert)
{
    tree_t tree;
    tree.insert(40, 30, 70, 5);
    ASSERT_TRUE(assert_node(tree.root, {5, 30, 40, 70}));

    tree.insert(16);
    ASSERT_TRUE(assert_node(tree.root, {30}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {40, 70}));

    tree.insert(82, 95);
    ASSERT_TRUE(assert_node(tree.root, {30}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {40, 70, 82, 95}));

    tree.insert(100);
    ASSERT_TRUE(assert_node(tree.root, {30, 82}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {40, 70}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {95, 100}));

    tree.insert(73, 54, 98);
    ASSERT_TRUE(assert_node(tree.root, {30, 82}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {40, 54, 70, 73}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {95, 98, 100}));

    tree.insert(37);
    ASSERT_TRUE(assert_node(tree.root, {30, 54, 82}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {37, 40}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {70, 73}));
    ASSERT_TRUE(assert_node(tree.root->children[3], {95, 98, 100}));

    tree.insert(25, 62, 81, 150);
    ASSERT_TRUE(assert_node(tree.root, {30, 54, 82}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16, 25}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {37, 40}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {62, 70, 73, 81}));
    ASSERT_TRUE(assert_node(tree.root->children[3], {95, 98, 100, 150}));

    tree.insert(79);
    ASSERT_TRUE(assert_node(tree.root, {30, 54, 73, 82}));
    ASSERT_TRUE(assert_node(tree.root->children[0], {5, 16, 25}));
    ASSERT_TRUE(assert_node(tree.root->children[1], {37, 40}));
    ASSERT_TRUE(assert_node(tree.root->children[2], {62, 70}));
    ASSERT_TRUE(assert_node(tree.root->children[3], {79, 81}));
    ASSERT_TRUE(assert_node(tree.root->children[4], {95, 98, 100, 150}));
}
 */