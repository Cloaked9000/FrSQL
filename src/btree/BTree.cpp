//
// Created by fred on 31/07/2021.
//

#include "btree/Node.h"
#include "btree/NodeStore.h"
#include "btree/BTree.h"

Tree::Tree()
{

}

bool Tree::create(Filesystem::Handle file)
{
    return store.create(std::move(file));
}

bool Tree::open(Filesystem::Handle file)
{
    return store.open(std::move(file));
}


std::optional<uint64_t> Tree::search(const uint64_t key)
{
    auto node = store.load_root();
    if(!node.valid())
    {
        return {};
    }

    while(node->count)
    {
        size_t location;
        if(node->search(key, location))
        {
            return true;
        }

        node = store.load(node->children[location]);
    }

    return false;
}

void Tree::insert(uint64_t key, uint64_t val)
{
    bool is_taller = false;
    uint64_t median;

    NodePtr right_child;
    auto root = store.load_root();
    insert_btree(root, key, val, median, right_child, is_taller);

    if(is_taller)
    {
        auto left = store.load_root().valid() ? store.load_root()->id : 0;
        auto right = right_child.valid() ? right_child->id : 0;

        auto temp_root = store.alloc();
        temp_root->count = 1;
        temp_root->list[0] = median;
        temp_root->children[0] = left;
        temp_root->children[1] = right;
        store.set_root(temp_root);
        store.height()++;
    }
}

void Tree::in_order()
{
    std::map<size_t, std::vector<uint64_t>> levels;
    auto root = store.load_root();
    recurse(root, 0, levels);

    for(auto iter = levels.begin(); iter != levels.end(); iter++)
    {
        for(auto &l : iter->second)
        {
            std::cout << l << ", ";
        }
        std::cout << std::endl;
    }
}

bool Tree::erase(uint64_t key)
{
    size_t position = 0;
    size_t depth = 1;
    bool doDelete = false;
    auto root = store.load_root();
    if(!erase(root, depth, key, doDelete, position))
    {
        return false;
    }

    //Tree is shrinking
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "UnreachableCode"
    if(doDelete)
    {
        // rebalance_node(root, position, doDelete);
        if(doDelete)
        {
            auto new_root = store.load(store.load_root()->children[0]);
            store.set_root(new_root);
            store.height()--;
        }
    }
#pragma clang diagnostic pop
    return true;
}

void Tree::rebalance_node(NodePtr &node, size_t position, bool &doDelete)
{
    //Check each sibling node to see if either has more than the minimum
    if(position > 0 && store.load(node->children[position - 1])->count > NodeOrder / 2)
    {
        //Left sibling has enough. Move a key from left sibling into us, and one of our keys into right sibling
        auto left_child = store.load(node->children[position - 1]);
        auto right_child = store.load(node->children[position]);

        //Extract highest key from left child
        uint64_t left_val = left_child->list[left_child->count - 1];
        uint64_t left_val2 = left_child->values[left_child->count - 1];
        left_child->erase(left_child->count - 1);

        //Move our key into right child
        right_child->insert(node->list[position - 1], node->values[position - 1], 0, 0);

        //Replace moved key with left
        node->list[position - 1] = left_val;
        node->values[position - 1] = left_val2;

        //Now we're good
        doDelete = false;
    }
    else if(node->children[position + 1] && store.load(node->children[position + 1])->count > NodeOrder / 2)
    {
        //Right has enough
        auto left_child = store.load(node->children[position]);
        auto right_child = store.load(node->children[position + 1]);

        //Extract lowest key from right child
        uint64_t right_val = right_child->list[0];
        uint64_t right_val2 = right_child->values[0];
        right_child->erase(0);

        //Move our key into left child
        left_child->insert(node->list[position], node->values[position], 0, right_child->count);

        //Replace moved key with right
        node->list[position] = right_val;
        node->values[position] = right_val2;

        doDelete = false;
    }
    else
    {
        //Else neither has enough, so we need to merge two siblings
        size_t median_pos;
        NodePtr left, right;
        if(node->children[position + 1])
        {
            //Merge with right sibling
            left = store.load(node->children[position]);
            right = store.load(node->children[position + 1]);
            median_pos = position;
        }
        else
        {
            //Merge with left sibling
            left = store.load(node->children[position - 1]);
            right = store.load(node->children[position]);
            median_pos = position - 1;
        }

        left->insert(node->list[median_pos], node->values[median_pos], 0, left->count);
        left->merge_right(right);

        node->erase(median_pos);
        doDelete = node->count < NodeOrder / 2;
    }
}

bool Tree::erase(NodePtr &node, size_t depth, uint64_t key, bool &doDelete, size_t position)
{
    if(!node.valid())
    {
        return false;
    }

    bool found = node->search(key, position);
    if(!found)
    {
        // Keep searching
        auto child = store.load(node->children[position]);
        bool erased = erase(child, ++depth, key, doDelete, position);

        // If the recursive erase succeeded, and we're in a re-balance state, re-balance the child
        if(doDelete)
        {
            rebalance_node(node, position, doDelete);
        }

        return erased;
    }

    // We've found it, found is true
    {
        // If we're not in a leaf, swap with child & erase from child instead
        if(depth != store.height())
        {
            auto leaf = store.load(node->children[position]);
            std::swap(node->list[position], leaf->list[leaf->count - 1]);
            bool erased = erase(leaf, ++depth, key, doDelete, leaf->count - 1);
            if(doDelete)
            {
                rebalance_node(node, position, doDelete);
            }
            return erased;
        }
    }

    //We've found the element & we're in a leaf node. Delete the item.
    node->erase(position);

    //If this node is still an acceptable size, we're done
    if(node->count >= NodeOrder / 2)
    {
        return true;
    }

    // Else we need to re-balance
    doDelete = true;
    return true;
}

void Tree::recurse(NodePtr &node, size_t level, std::map<size_t, std::vector<uint64_t>> &levels)
{
    if(!node.valid())
    {
        return;
    }

    auto c1 = store.load(node->children[0]);
    recurse(c1, level + 1, levels);
    for(size_t a = 0; a < node->count; a++)
    {
        levels[level].emplace_back((uint64_t)node->list[a]);
        auto c2 = store.load(node->children[a + 1]);
        recurse(c2, level + 1, levels);
    }
    levels[level].emplace_back();
}

void Tree::insert_btree(NodePtr &node, uint64_t key, uint64_t val, uint64_t &median, NodePtr &right_child, bool &is_taller)
{
    if(!node.valid())
    {
        //B-tree is empty or search ends at empty subtree
        median = key;
        is_taller = true;
        return;
    }

    size_t location;
    bool found = node->search(key, location);
    if(found)
    {
        throw std::logic_error("Item already in tree!");
    }

    auto parent = store.load(node->children[location]);
    insert_btree(parent, key, val, median, right_child, is_taller);
    if(is_taller)
    {
        if(node->is_full())
        {
            NodePtr right;
            split_node(node, median, val, right_child, location, right, median);
            if(right->count)
            {
                right_child = std::move(right);
            }
        }
        else
        {
            node->insert(median, val, right_child.valid() ? right_child->id : 0, location);
            is_taller = false;
        }
    }

}

void Tree::split_node(NodePtr &node, uint64_t key, uint64_t val, NodePtr &right_child, size_t insert_pos, NodePtr &right_node, uint64_t &median)
{
    right_node = store.alloc();
    size_t mid = (NodeOrder - 1) / 2;

    // Insert into left side
    if(insert_pos <= mid)
    {
        //Move things on left side of mid into new node
        size_t index = 0;
        size_t i = mid;
        while(i < NodeOrder - 1)
        {
            right_node->list[index] = node->list[i];
            right_node->children[index + 1] = node->children[i + 1];
            index++;
            i++;
        }

        node->count = mid;
        node->insert(key, val, right_child.valid() ? right_child->id : 0, insert_pos);
        node->count--;
        median = node->list[node->count];

        right_node->count = index;
        right_node->children[0] = node->children[node->count + 1];
    }
    else //insert into right side
    {
        size_t index = 0;
        size_t i = mid + 1;
        while(i < NodeOrder - 1)
        {
            right_node->list[index] = node->list[i];
            right_node->children[index + 1] = node->children[i + 1];
            index++;
            i++;
        }

        node->count = mid;
        right_node->count = index;

        median = node->list[mid];
        right_node->insert(key, val, right_child.valid() ? right_child->id : 0, insert_pos - mid - 1);
        right_node->children[0] = node->children[node->count + 1];
    }
}