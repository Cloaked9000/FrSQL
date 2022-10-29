//
// Created by fred on 31/07/2021.
//

#ifndef TESTDB_BTREE_H
#define TESTDB_BTREE_H
#include <array>
#include <algorithm>
#include <vector>
#include <limits>
#include <iostream>
#include <optional>
#include <map>
#include <cassert>
#include <memory>
#include "filesystem/Filesystem.h"
#include "btree/Node.h"
#include "btree/NodeStore.h"

class Tree
{
public:
    Tree();
    bool create(Filesystem::Handle file);
    bool open(Filesystem::Handle file);
    std::optional<uint64_t> search(uint64_t val);
    void insert(uint64_t key, uint64_t val);
    void in_order();
    bool erase(uint64_t key);

private:
    void rebalance_node(NodePtr &node, size_t position, bool &doDelete);
    bool erase(NodePtr &node, size_t depth, uint64_t key, bool &doDelete, size_t position);
    void recurse(NodePtr &node, size_t level, std::map<size_t, std::vector<uint64_t>> &levels);
    void insert_btree(NodePtr &node, uint64_t key, uint64_t val, uint64_t &median, NodePtr &right_child, bool &is_taller);
    void split_node(NodePtr &node, uint64_t key, uint64_t val, NodePtr &right_child, size_t insert_pos, NodePtr &right_node, uint64_t &median);

    NodeStore store;
};

#endif //TESTDB_BTREE_H
