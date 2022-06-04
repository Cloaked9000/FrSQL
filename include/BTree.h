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
#include <map>
#include <cassert>
#include <memory>
#include "filesystem/Filesystem.h"

static constexpr auto NodeOrder = 5;
struct DiskNode;
class NodeStore;

template<typename T>
class NodePtr
{
public:
    NodePtr()
    : NodePtr(nullptr, nullptr)
    {

    }

    NodePtr(T *store, DiskNode *node)
    : store(store), node(node)
    {

    }

    NodePtr(const NodePtr&)=delete;
    void operator=(const NodePtr&)=delete;
    NodePtr(NodePtr&&o) noexcept
            : store(o.store),
              node(o.node)
    {
        o.store = nullptr;
    }

    NodePtr &operator=(NodePtr&& o) noexcept
    {
        release();
        store = o.store;
        node = o.node;
        o.store = nullptr;
        return *this;
    }

    ~NodePtr()
    {
        release();
    }

    void release()
    {
        if(store)
        {
            store->free(node);
        }
    }

    DiskNode *operator->()
    {
        return node;
    }

    bool valid()
    {
        return node != nullptr;
    }

private:
    T *store;
    DiskNode *node;
};

struct NodeStoreHeader
{
    uint64_t root = 0;
    uint64_t height = 0;
    uint64_t node_count = 1;
} __attribute__((packed));

struct DiskNode
{
    uint64_t id = 0;
    uint64_t list[NodeOrder - 1]{};
    uint64_t children[NodeOrder]{};
    uint64_t count = 0;

    bool operator==(const DiskNode &other) const
    {
        if(other.count != count)
            return false;
        if(!std::equal(std::begin(other.list), std::begin(other.list) + count, std::begin(list)))
            return false;
        if(!std::equal(std::begin(other.children), std::begin(other.children) + count + 1, std::begin(children)))
            return false;
        return true;
    }

    void erase(size_t position)
    {
        assert(position < count);
        for(size_t a = position; a < NodeOrder - 1; a++)
        {
            list[a] = list[a + 1];
        }
        for(size_t a = position; a < NodeOrder - 2; a++)
        {
            children[a + 1] = children[a + 2];
        }

        count--;
    }

    void insert(uint64_t val, uint64_t right_child, size_t insert_pos)
    {
        assert(insert_pos < NodeOrder);
        size_t index;
        for(index = count; index > insert_pos; index--)
        {
            list[index] = list[index - 1];
            children[index + 1] = children[index];
        }

        list[index] = val;
        children[index + 1] = right_child;
        count++;
    }


    [[nodiscard]] bool is_full() const
    {
        return count == NodeOrder - 1;
    }

    bool search(uint64_t val, size_t &location)
    {
        location = 0;
        while(location < count && val > list[location])
        {
            location++;
        }

        return location < count && val == list[location];
    }

    //Note: Overwrites last child
    void merge_right(NodePtr<NodeStore> &right)
    {
        assert(right->count);
        for(size_t a = 0; a < right->count; a++)
        {
            list[count + a] = right->list[a];
        }
        for(size_t a = 0; a < right->count + 1; a++)
        {
            children[count + a] = right->children[a];
        }

        count += right->count;
    }
} __attribute__((packed));

class NodeStore
{
public:
    ~NodeStore()
    {
        close();
    }

    bool create(Filesystem::Handle file_)
    {
        file = std::move(file_);
        file.seek(0);
        file.write(reinterpret_cast<char *>(&header), sizeof(header));
        return true;
    }

    bool open(Filesystem::Handle file_)
    {
        file = std::move(file_);
        return true;
    }

    void close()
    {
        if(file.is_open())
        {
            file.seek(0);
            file.write(reinterpret_cast<char *>(&header), sizeof(header));
        }
    }

    NodePtr<NodeStore> alloc()
    {
        uint64_t id = header.node_count++;
        auto iter = nodes.emplace(id, DiskNode());
        iter.first->second.id= id;
        return {this, &iter.first->second};
    }

    void free(DiskNode *node)
    {

    }

    NodePtr<NodeStore> load_root()
    {
        return load(header.root);
    }

    NodePtr<NodeStore> load(uint64_t id)
    {
        auto const iter = nodes.find(id);
        return iter == nodes.end() ? NodePtr<NodeStore>(nullptr, nullptr) : NodePtr<NodeStore>(this, &iter->second);
    }

    void set_root(NodePtr<NodeStore> &node)
    {
        header.root = node->id;
    }

    uint64_t &height()
    {
        return header.height;
    }

private:
    NodeStoreHeader header;
    Filesystem::Handle file;
    std::unordered_map<uint64_t, DiskNode> nodes;
};

template<typename T, size_t Order>
class Tree
{
public:
    Tree()=default;

    bool create(Filesystem::Handle file)
    {
        return store.create(std::move(file));
    }

    bool open(Filesystem::Handle file)
    {
        return store.open(std::move(file));
    }

    bool search(const uint64_t val)
    {
        auto node = store.load_root();
        while(node->count)
        {
            size_t location;
            if(node->search(val, location))
            {
                return true;
            }

            node = store.load(node->children[location]);
        }

        return false;
    }

    template<typename ...Args>
    void insert(T val, const Args&... args)
    {
        insert(val);
        insert(args...);
    }

    void insert(T val)
    {
        bool is_taller = false;
        T median;

        NodePtr<NodeStore> right_child;
        auto root = store.load_root();
        insert_btree(root, std::move(val), median, right_child, is_taller);

        if(is_taller)
        {
            auto temp_root = store.alloc();
            temp_root->count = 1;
            temp_root->list[0] = median;
            temp_root->children[0] = store.load_root().valid() ? store.load_root()->id : 0;
            temp_root->children[1] = right_child.valid() ? right_child->id : 0;
            store.set_root(temp_root);
            store.height()++;
        }
    }

    void in_order()
    {
        std::map<size_t, std::vector<T>> levels;
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

    bool erase(const T &val)
    {
        size_t position = 0;
        size_t depth = 1;
        bool doDelete = false;
        if(!erase(store.load_root(), depth, val, doDelete, position))
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

private:
public:

    void rebalance_node(NodePtr<NodeStore> &node, size_t position, bool &doDelete)
    {

        //Check each sibling node to see if either has more than the minimum
        if(position > 0 && store.load(node->children[position - 1])->count > Order / 2)
        {
            //Left sibling has enough. Move a key from left sibling into us, and one of our keys into right sibling
            auto left_child = store.load(node->children[position - 1]);
            auto right_child = store.load(node->children[position]);

            //Extract highest key from left child
            T left_val = left_child->list[left_child->count - 1];
            left_child->erase(left_child->count - 1);

            //Move our key into right child
            right_child->insert(node->list[position - 1], 0, 0);

            //Replace moved key with left
            node->list[position - 1] = left_val;

            //Now we're good
            doDelete = false;
        }
        else if(node->children[position + 1] && store.load(node->children[position + 1])->count > Order / 2)
        {
            //Right has enough
            auto left_child = store.load(node->children[position]);
            auto right_child = store.load(node->children[position + 1]);

            //Extract lowest key from right child
            T right_val = right_child->list[0];
            right_child->erase(0);

            //Move our key into left child
            left_child->insert(node->list[position], 0, right_child->count);

            //Replace moved key with right
            node->list[position] = right_val;

            doDelete = false;
        }
        else
        {
            //Else neither has enough, so we need to merge two siblings
            size_t median_pos;
            NodePtr<NodeStore> left, right;
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

            left->insert(node->list[median_pos], 0, left->count);
            left->merge_right(right);

            node->erase(median_pos);
            doDelete = node->count < Order / 2;
        }
    }

    bool erase(DiskNode *node, size_t depth, const T &val, bool &doDelete, size_t position)
    {
        if(!node)
        {
            return false;
        }

        bool found = node->search(val, position);
        if(!found)
        {
            // Keep searching
            bool erased = erase(node->children[position], ++depth, val, doDelete, position);

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
                bool erased = erase(leaf, ++depth, val, doDelete, leaf->count - 1);
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
        if(node->count >= Order / 2)
        {
            return true;
        }

        // Else we need to re-balance
        doDelete = true;
        return true;
    }

    void recurse(NodePtr<NodeStore> &node, size_t level, std::map<size_t, std::vector<T>> &levels)
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

    void insert_btree(NodePtr<NodeStore> &node, T val, T &median, NodePtr<NodeStore> &right_child, bool &is_taller)
    {
        if(!node.valid())
        {
            //B-tree is empty or search ends at empty subtree
            median = val;
            is_taller = true;
            return;
        }

        size_t location;
        bool found = node->search(val, location);
        if(found)
        {
            throw std::logic_error("Item already in tree!");
        }

        auto parent = store.load(node->children[location]);
        insert_btree(parent, val, median, right_child, is_taller);
        if(is_taller)
        {
            if(node->is_full())
            {
                NodePtr<NodeStore> right;
                split_node(node, median, right_child, location, right, median);
                if(right->count)
                {
                    right_child = std::move(right);
                }
            }
            else
            {
                node->insert(median, right_child.valid() ? right_child->id : 0, location);
                is_taller = false;
            }
        }

    }

    void split_node(NodePtr<NodeStore> &node, T val, NodePtr<NodeStore> &right_child, size_t insert_pos, NodePtr<NodeStore> &right_node, T &median)
    {
        right_node = store.alloc();
        size_t mid = (Order - 1) / 2;

        // Insert into left side
        if(insert_pos <= mid)
        {
            //Move things on left side of mid into new node
            size_t index = 0;
            size_t i = mid;
            while(i < Order - 1)
            {
                right_node->list[index] = node->list[i];
                right_node->children[index + 1] = node->children[i + 1];
                index++;
                i++;
            }

            node->count = mid;
            node->insert(std::move(val), right_child.valid() ? right_child->id : 0, insert_pos);
            node->count--;
            median = node->list[node->count];

            right_node->count = index;
            right_node->children[0] = node->children[node->count + 1];
        }
        else //insert into right side
        {
            size_t index = 0;
            size_t i = mid + 1;
            while(i < Order - 1)
            {
                right_node->list[index] = node->list[i];
                right_node->children[index + 1] = node->children[i + 1];
                index++;
                i++;
            }

            node->count = mid;
            right_node->count = index;

            median = node->list[mid];
            right_node->insert(val, right_child.valid() ? right_child->id : 0, insert_pos - mid - 1);
            right_node->children[0] = node->children[node->count + 1];
        }
    }


    NodeStore store;
};

#endif //TESTDB_BTREE_H
